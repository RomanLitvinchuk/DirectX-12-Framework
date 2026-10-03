#include "rendering_system.h"
#include "throw_if_failed.h"
#include "d3dUtil.h"
#include "vertex.h"
#include "d3dx12.h"
#include <random>
#include "g_buffer.h"
#include "post_process.h"
#include "ssao.h"
#include "singletone_device.h"

void RenderingSystem::CreateOpaqueRS() {
	CD3DX12_ROOT_PARAMETER slotRootParameter[8];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE diffuseTable;
	diffuseTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE normalTable;
	normalTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	CD3DX12_DESCRIPTOR_RANGE dispTable;
	dispTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[1].InitAsDescriptorTable(1, &diffuseTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[2].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[3].InitAsConstantBufferView(1);
	slotRootParameter[4].InitAsDescriptorTable(1, &normalTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[5].InitAsDescriptorTable(1, &dispTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[6].InitAsConstantBufferView(2);
	slotRootParameter[7].InitAsShaderResourceView(3);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&opaqueRS_)
	));
}

void RenderingSystem::CompileShaders() {

	opaqueVS_ = d3dUtil::CompileShader(L"shaders/gbuffer.hlsl", nullptr, "VS", "vs_5_0");
	opaquePS_ = d3dUtil::CompileShader(L"shaders/gbuffer.hlsl", nullptr, "PS", "ps_5_0");


	fullscreenTriangleVS_ = d3dUtil::CompileShader(L"shaders/light.hlsl", nullptr, "VS_FullScreenTriangle", "vs_5_0");
	lightPS_ = d3dUtil::CompileShader(L"shaders/light.hlsl", nullptr, "PS_DeferredLighting", "ps_5_0");


	bulbVS_ = d3dUtil::CompileShader(L"shaders/bulb.hlsl", nullptr, "VS", "vs_5_0");
	bulbPS_ = d3dUtil::CompileShader(L"shaders/bulb.hlsl", nullptr, "PS", "ps_5_0");


	HS_ = d3dUtil::CompileShader(L"shaders/HullShader.hlsl", nullptr, "main", "hs_5_0");


	DS_ = d3dUtil::CompileShader(L"shaders/DomainShader.hlsl", nullptr, "DS", "ds_5_0");

	tessVS_ = d3dUtil::CompileShader(L"shaders/tess.hlsl", nullptr, "TessVS", "vs_5_0");

	bakedVS_ = d3dUtil::CompileShader(L"shaders/baked.hlsl", nullptr, "BakedVS", "vs_5_0");

	wireframeVS_ = d3dUtil::CompileShader(L"shaders/wireframe.hlsl", nullptr, "VS", "vs_5_0");
	wireframePS_ = d3dUtil::CompileShader(L"shaders/wireframe.hlsl", nullptr, "PS", "ps_5_0");

	particleVS_ = d3dUtil::CompileShader(L"shaders/particles.hlsl", nullptr, "VS", "vs_5_0");
	particleGS_ = d3dUtil::CompileShader(L"shaders/particleGS.hlsl", nullptr, "GS", "gs_5_0");
	particlePS_ = d3dUtil::CompileShader(L"shaders/particles.hlsl", nullptr, "PS", "ps_5_0");
	particleUpdateCS_ = d3dUtil::CompileShader(L"shaders/particleUpdateCS.hlsl", nullptr, "UpdateCS", "cs_5_0");
	particleEmitCS_ = d3dUtil::CompileShader(L"shaders/particleEmitCS.hlsl", nullptr, "EmitCS", "cs_5_0");

	shadowVS_ = d3dUtil::CompileShader(L"shaders/shadowVS.hlsl", nullptr, "VS", "vs_5_0");

	SsaoPS_ = d3dUtil::CompileShader(L"shaders/ssao.hlsl", nullptr, "PS", "ps_5_0");
	SsaoBlurPS_ = d3dUtil::CompileShader(L"shaders/ssaoBlur.hlsl", nullptr, "PS", "ps_5_0");

	billboardVS_ = d3dUtil::CompileShader(L"shaders/billboard.hlsl", nullptr, "VS", "vs_5_0");
	billboardPS_ = d3dUtil::CompileShader(L"shaders/billboard.hlsl", nullptr, "PS", "ps_5_0");

	pp_tonemappingPS_ = d3dUtil::CompileShader(L"shaders/PPTonemapping.hlsl", nullptr, "PS", "ps_5_0");
	pp_vignettePS_ = d3dUtil::CompileShader(L"shaders/PPVignette.hlsl", nullptr, "PS", "ps_5_0");
	pp_outputPS_ = d3dUtil::CompileShader(L"shaders/PPOutput.hlsl", nullptr, "PS", "ps_5_0");
}

void RenderingSystem::CreateOpaquePSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = opaqueRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(opaqueVS_->GetBufferPointer()), opaqueVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(opaquePS_->GetBufferPointer()), opaquePS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = true;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&opaquePSO_)));
}

void RenderingSystem::CreateLightRS() {
	CD3DX12_ROOT_PARAMETER rootParameter[7];
	CD3DX12_DESCRIPTOR_RANGE srvTable[4];
	srvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	srvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	srvTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	srvTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	CD3DX12_DESCRIPTOR_RANGE smTable;
	smTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
	CD3DX12_DESCRIPTOR_RANGE samplerTable[2];
	samplerTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	samplerTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);

	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsConstants(1, 1);
	rootParameter[2].InitAsDescriptorTable(4, srvTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameter[3].InitAsDescriptorTable(2, samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[4].InitAsDescriptorTable(1, &smTable);
	rootParameter[5].InitAsConstantBufferView(2);
	rootParameter[6].InitAsConstantBufferView(3);


	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&lightRS_)));
}

void RenderingSystem::CreateLightPSO() {
	D3D12_RENDER_TARGET_BLEND_DESC RTBDesc;
	RTBDesc.BlendEnable = true;
	RTBDesc.LogicOpEnable = false;

	RTBDesc.SrcBlend = D3D12_BLEND_ONE;
	RTBDesc.DestBlend = D3D12_BLEND_ONE;
	RTBDesc.BlendOp = D3D12_BLEND_OP_ADD;

	RTBDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	RTBDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	RTBDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	RTBDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0] = RTBDesc;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.pRootSignature = lightRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(fullscreenTriangleVS_->GetBufferPointer()), fullscreenTriangleVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(lightPS_->GetBufferPointer()), lightPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = blendDesc;

	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&lightPSO_)));

}

void RenderingSystem::GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	for (int i = 0; i < count; ++i) {
		float u = dis(gen);
		float v = dis(gen);

		float h = treeHeight * (1.0f - std::sqrt(u)) + 50.0f;
		float r = treeBaseRadius * std::sqrt(u);
		float theta = 2.0f * XM_PI * v;

		Vector3 localPos(
			r * std::cos(theta),
			h,
			r * std::sin(theta)
		);

		LightConstants bulb = {};
		bulb.lightType = 1; // Point light
		bulb.lightPosition = treeBasePosition + localPos;
		bulb.lightRange = 100.0f;

		bulb.lightColor = { dis(gen), dis(gen), dis(gen) };

		lightsArray.push_back(bulb);
	}
}

void RenderingSystem::CreateBulbRS() {
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&bulbRS_)));
}

void RenderingSystem::CreateBulbPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = bulbRS_.Get(); 

	psoDesc.VS = { reinterpret_cast<BYTE*>(bulbVS_->GetBufferPointer()), bulbVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(bulbPS_->GetBufferPointer()), bulbPS_->GetBufferSize() };

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bulbPSO_)));
}

void RenderingSystem::CreateStreamOutputRS()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[8];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE diffuseTable;
	diffuseTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE normalTable;
	normalTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	CD3DX12_DESCRIPTOR_RANGE dispTable;
	dispTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[1].InitAsDescriptorTable(1, &diffuseTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[2].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[3].InitAsConstantBufferView(1);
	slotRootParameter[4].InitAsDescriptorTable(1, &normalTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[5].InitAsDescriptorTable(1, &dispTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[6].InitAsConstantBufferView(2);
	slotRootParameter[7].InitAsShaderResourceView(3);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&streamOutputRS_)
	));
}

void RenderingSystem::CreateStreamOutputPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	D3D12_SO_DECLARATION_ENTRY soDecl[] =
	{
		{0, "WORLDPOS", 0, 0, 3, 0},
		{0, "NORMAL",   0, 0, 3, 0 },
		{0, "NORMAL",   1, 0, 3, 0 },
		{0, "TEXCOORD", 0, 0, 2, 0 },
		{0, "TANGENT",  0, 0, 3, 0 }
	};

	UINT stride[] = { sizeof(BakedVertex) };

	D3D12_STREAM_OUTPUT_DESC soDesc = {};
	soDesc.NumEntries = _countof(soDecl);
	soDesc.NumStrides = 1;
	soDesc.pBufferStrides = stride;
	soDesc.pSODeclaration = soDecl;
	soDesc.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC streamOutputPSODesc;
	ZeroMemory(&streamOutputPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	streamOutputPSODesc.InputLayout = { layout.data(), (UINT)layout.size() };
	streamOutputPSODesc.pRootSignature = streamOutputRS_.Get();
	streamOutputPSODesc.StreamOutput = soDesc;
	streamOutputPSODesc.VS = { reinterpret_cast<BYTE*>(tessVS_->GetBufferPointer()), tessVS_->GetBufferSize() };
	streamOutputPSODesc.HS = { reinterpret_cast<BYTE*>(HS_->GetBufferPointer()), HS_->GetBufferSize() };
	streamOutputPSODesc.DS = { reinterpret_cast<BYTE*>(DS_->GetBufferPointer()), DS_->GetBufferSize() };
	streamOutputPSODesc.PS = { nullptr, 0};
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.FrontCounterClockwise = true;
	streamOutputPSODesc.RasterizerState = rastDesc;
	streamOutputPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	streamOutputPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	streamOutputPSODesc.DepthStencilState.DepthEnable = FALSE;
	streamOutputPSODesc.DepthStencilState.StencilEnable = FALSE;
	streamOutputPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	streamOutputPSODesc.SampleMask = UINT_MAX;
	streamOutputPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	streamOutputPSODesc.NumRenderTargets = 0;
	streamOutputPSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	streamOutputPSODesc.SampleDesc.Count = 1;
	streamOutputPSODesc.SampleDesc.Quality = 0;
	streamOutputPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&streamOutputPSODesc, IID_PPV_ARGS(&streamOutputPSO_)));
}

void RenderingSystem::CreateBakedPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = opaqueRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(bakedVS_->GetBufferPointer()), bakedVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(opaquePS_->GetBufferPointer()), opaquePS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = true;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bakedPSO_)));
}

void RenderingSystem::CreateWireframeRS()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsShaderResourceView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(slotRootParameter), slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	
	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&wireframeRS_)));
}

void RenderingSystem::CreateWireframePSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = wireframeRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(wireframeVS_->GetBufferPointer()), wireframeVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(wireframePS_->GetBufferPointer()), wireframePS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.FrontCounterClockwise = true;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&wireframePSO_)));
}

void RenderingSystem::CreateParticleRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsShaderResourceView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;
	
	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&particleRS_)));
}

void RenderingSystem::CreateParticlePSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = particleRS_.Get();
	psoDesc.VS = { particleVS_->GetBufferPointer(), particleVS_->GetBufferSize() };
	psoDesc.GS = { particleGS_->GetBufferPointer(), particleGS_->GetBufferSize() };
	psoDesc.PS = { particlePS_->GetBufferPointer(), particlePS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = true;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&particlePSO_)));
}

void RenderingSystem::CreateParticlesUpdateRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[3];
	CD3DX12_DESCRIPTOR_RANGE uavTable[2];
	uavTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
	uavTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsUnorderedAccessView(0);
	rootParameter[2].InitAsDescriptorTable(2, uavTable, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&particlesUpdateRS_)));
}

void RenderingSystem::CreateParticlesUpdatePSO()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC particleUpdatePSO = {};
	particleUpdatePSO.pRootSignature = particlesUpdateRS_.Get();
	particleUpdatePSO.CS = { particleUpdateCS_->GetBufferPointer(), particleUpdateCS_->GetBufferSize() };
	particleUpdatePSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->CreateComputePipelineState(&particleUpdatePSO, IID_PPV_ARGS(&particlesUpdatePSO_)));
}

void RenderingSystem::CreateParticlesEmitRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
	rootParameter[0].InitAsUnorderedAccessView(0);
	rootParameter[1].InitAsDescriptorTable(1, &uavTable, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&particlesEmitRS_)));
}

void RenderingSystem::CreateParticlesEmitPSO()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC particleEmitPSO = {};
	particleEmitPSO.pRootSignature = particlesEmitRS_.Get();
	particleEmitPSO.CS = { particleEmitCS_->GetBufferPointer(), particleEmitCS_->GetBufferSize() };
	particleEmitPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->CreateComputePipelineState(&particleEmitPSO, IID_PPV_ARGS(&particlesEmitPSO_)));
}

void RenderingSystem::CreateShadowRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsShaderResourceView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&shadowRS_)));
}

void RenderingSystem::CreateShadowPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = shadowRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(shadowVS_->GetBufferPointer()), shadowVS_->GetBufferSize() };
	psoDesc.PS = { nullptr, 0 };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.FrontCounterClockwise = true;
	rastDesc.DepthBias = 0;
	rastDesc.SlopeScaledDepthBias = 1.0f;
	rastDesc.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&shadowPSO_)));
}

void RenderingSystem::CreateSSAORS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[5];
	CD3DX12_DESCRIPTOR_RANGE texTable[3];
	texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	texTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	rootParameter[0].InitAsDescriptorTable(3, texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	CD3DX12_DESCRIPTOR_RANGE sampleTable[2];
	sampleTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	sampleTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);
	rootParameter[1].InitAsDescriptorTable(2, sampleTable);
	rootParameter[2].InitAsConstantBufferView(0);
	rootParameter[3].InitAsConstantBufferView(1);
	rootParameter[4].InitAsConstantBufferView(2);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&SsaoRS_)));
}

void RenderingSystem::CreateSSAOPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = SsaoRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(fullscreenTriangleVS_->GetBufferPointer()), fullscreenTriangleVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(SsaoPS_->GetBufferPointer()), SsaoPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.FrontCounterClockwise = false;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	CD3DX12_DEPTH_STENCIL_DESC dsDesc(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = false;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.DepthStencilState = dsDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&SsaoPSO_)));
}

void RenderingSystem::CreateSSAOBlurRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[4];
	CD3DX12_DESCRIPTOR_RANGE ssaoTable;
	ssaoTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameter[0].InitAsDescriptorTable(1, &ssaoTable, D3D12_SHADER_VISIBILITY_PIXEL);
	CD3DX12_DESCRIPTOR_RANGE normalTable;
	normalTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	rootParameter[1].InitAsDescriptorTable(1, &normalTable, D3D12_SHADER_VISIBILITY_PIXEL);
	CD3DX12_DESCRIPTOR_RANGE sampleTable;
	sampleTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	rootParameter[2].InitAsDescriptorTable(1, &sampleTable, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[3].InitAsConstantBufferView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&SsaoBlurRS_)));
}

void RenderingSystem::CreateSSAOBlurPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = SsaoBlurRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(fullscreenTriangleVS_->GetBufferPointer()), fullscreenTriangleVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(SsaoBlurPS_->GetBufferPointer()), SsaoBlurPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.FrontCounterClockwise = false;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	CD3DX12_DEPTH_STENCIL_DESC dsDesc(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = false;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.DepthStencilState = dsDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&SsaoBlurPSO_)));
}

void RenderingSystem::CreateBillboardRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[5];
	rootParameter[0].InitAsShaderResourceView(0);
	rootParameter[1].InitAsConstantBufferView(0);
	rootParameter[2].InitAsConstantBufferView(1);
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	rootParameter[3].InitAsDescriptorTable(1, &texTable);
	CD3DX12_DESCRIPTOR_RANGE sampleTable;
	sampleTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	rootParameter[4].InitAsDescriptorTable(1, &sampleTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&billboardRS_)));
}

void RenderingSystem::CreateBillboardPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = billboardRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(billboardVS_->GetBufferPointer()), billboardVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(billboardPS_->GetBufferPointer()), billboardPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	rastDesc.FrontCounterClockwise = true;
	rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&billboardPSO_)));
}

void RenderingSystem::CreatePPDefaultRS()
{
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameter[0].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL);
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	rootParameter[1].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> errorBlob;
	ComPtr<ID3DBlob> serializedRootDesc;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootDesc.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(device->CreateRootSignature(0, serializedRootDesc->GetBufferPointer(), serializedRootDesc->GetBufferSize(), IID_PPV_ARGS(&pp_defaultRS_)));
}

void RenderingSystem::CreatePPTonemappingPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = pp_defaultRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(fullscreenTriangleVS_->GetBufferPointer()), fullscreenTriangleVS_->GetBufferSize()};
	psoDesc.PS = { reinterpret_cast<BYTE*>(pp_tonemappingPS_->GetBufferPointer()), pp_tonemappingPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = false;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	CD3DX12_DEPTH_STENCIL_DESC dsDesc(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = false;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.DepthStencilState = dsDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pp_tonemappingPSO_)));
}

void RenderingSystem::CreatePPVignettePSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = pp_defaultRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(fullscreenTriangleVS_->GetBufferPointer()), fullscreenTriangleVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(pp_vignettePS_->GetBufferPointer()), pp_vignettePS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = false;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	CD3DX12_DEPTH_STENCIL_DESC dsDesc(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = false;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.DepthStencilState = dsDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pp_vignettePSO_)));
}

void RenderingSystem::CreatePPOutputPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.pRootSignature = pp_defaultRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(fullscreenTriangleVS_->GetBufferPointer()), fullscreenTriangleVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(pp_outputPS_->GetBufferPointer()), pp_outputPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = false;
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	CD3DX12_DEPTH_STENCIL_DESC dsDesc(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;
	dsDesc.StencilEnable = false;
	dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	psoDesc.DepthStencilState = dsDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pp_outputPSO_)));
}

RenderingSystem::RenderingSystem(int width, int height, ID3D12Resource* noiseTexture) {
	device = SingletonDevice::GetDevice();
	BuildLayouts();
	CreateOpaqueRS();
	CompileShaders();
	CreateOpaquePSO(inputLayout_);

	CreateStreamOutputRS();
	CreateStreamOutputPSO(inputLayout_);
	CreateBakedPSO(bakedLayout_);

	CreateLightRS();
	CreateLightPSO();

	CreateWireframeRS();
	CreateWireframePSO(wireframeLayout_);

	CreateParticleRS();
	CreateParticlePSO();

	CreateParticlesUpdateRS();
	CreateParticlesUpdatePSO();
	CreateParticlesEmitRS();
	CreateParticlesEmitPSO();

	CreateShadowRS();
	CreateShadowPSO(inputLayout_);

	CreateSSAORS();
	CreateSSAOPSO();

	CreateSSAOBlurRS();
	CreateSSAOBlurPSO();

	CreateBillboardRS();
	CreateBillboardPSO();

	g_buffer = std::make_unique<GBuffer>(width, height);
	post_process = std::make_unique<PostProcess>(width, height);
	ssao = std::make_unique<SSAO>(width / 2, height / 2, g_buffer->GetDepthTex().Resource.Get(), g_buffer->GetNormalTex().Resource.Get(), noiseTexture);

	LightConstants sun = {};
	sun.lightType = 0; // Directional
	sun.lightDirection = { 0.0f, -1.0f, 0.0f };
	sun.lightColor = { 1.0f, 0.9f, 0.8f };
	sceneLights_.push_back(sun);

	GenerateTreeLights(sceneLights_, { 10.0f, 0.0f, -60.0f }, 350.0f, 100.0f, 500);

	CreateBulbRS();
	CreateBulbPSO(inputLayout_);

	CreatePPDefaultRS();
	CreatePPTonemappingPSO();
	CreatePPVignettePSO();
	CreatePPOutputPSO();



	D3D12_DESCRIPTOR_HEAP_DESC sampHeapDesc = {};
	sampHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	sampHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	sampHeapDesc.NumDescriptors = 2;
	ThrowIfFailed(device->CreateDescriptorHeap(&sampHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&samplerHeap));

	D3D12_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	device->CreateSampler(&sampDesc, samplerHeap->GetCPUDescriptorHandleForHeapStart());

	sampDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;

	auto handle = samplerHeap->GetCPUDescriptorHandleForHeapStart();
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	CD3DX12_CPU_DESCRIPTOR_HANDLE smpHandle(handle, 1, size);
	device->CreateSampler(&sampDesc, smpHandle);
}


