#include "ssao.h"
#include "DX12App.h"
#include "throw_if_failed.h"
#include "singletone_device.h"

SSAO::SSAO(int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture) {
	device = SingletonDevice::GetDevice();
	CreateHeaps();
	CreateTexture(width, height);
	CreateRTV();
	CreateSRV(depthTexture, normalTexture, noiseTexture);
	CreateSamplers();
}

MyTexture& SSAO::GetTextureA() 
{
	return SSAOTexture_A;
}

MyTexture& SSAO::GetTextureB() 
{
	return SSAOTexture_B;
}

ComPtr<ID3D12DescriptorHeap> SSAO::GetSrvHeap() 
{
	return srvHeap;
}

ComPtr<ID3D12DescriptorHeap> SSAO::GetSamplerHeap() 
{
	return samplerHeap;
}

ComPtr<ID3D12DescriptorHeap> SSAO::GetRtvHeap() 
{
	return rtvHeap;
}

void SSAO::CreateHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 5;
	srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC samplerDesc;
	samplerDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	samplerDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerDesc.NumDescriptors = 2;
	samplerDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&samplerDesc, IID_PPV_ARGS(&samplerHeap)));
}

void SSAO::CreateTexture(int width, int height) {
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R32_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&SSAOTexture_A.Resource));
	SSAOTexture_A.Resource->SetName(L"SSAO Texture A");
	SSAOTexture_A.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&SSAOTexture_B.Resource));
	SSAOTexture_B.Resource->SetName(L"SSAO Texture B");
	SSAOTexture_B.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void SSAO::CreateRTV() {
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	auto rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	SSAOTexture_A.rtvHandle = handle;
	device->CreateRenderTargetView(SSAOTexture_A.Resource.Get(), &rtvDesc, handle);

	handle.Offset(1, rtvSize);
	SSAOTexture_B.rtvHandle = handle;
	device->CreateRenderTargetView(SSAOTexture_B.Resource.Get(), &rtvDesc, handle);
}

void SSAO::CreateSRV(ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(srvHeap->GetCPUDescriptorHandleForHeapStart());
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	SSAOTexture_A.srvHandle = handle;
	device->CreateShaderResourceView(SSAOTexture_A.Resource.Get(), &srvDesc, handle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(srvHeap->GetGPUDescriptorHandleForHeapStart());
	SSAOTexture_A.srvGpuHandle = gpuHandle;

	handle.Offset(1, size);
	gpuHandle.Offset(1, size);
	SSAOTexture_B.srvHandle = handle;
	SSAOTexture_B.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(SSAOTexture_B.Resource.Get(), &srvDesc, handle);

	handle.Offset(1, size);
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	device->CreateShaderResourceView(depthTexture, &srvDesc, handle);

	handle.Offset(1, size);
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	device->CreateShaderResourceView(normalTexture, &srvDesc, handle);

	handle.Offset(1, size);
	D3D12_RESOURCE_DESC noiseDesc = noiseTexture->GetDesc();
	srvDesc.Format = noiseDesc.Format;
	device->CreateShaderResourceView(noiseTexture, &srvDesc, handle);
}

void SSAO::CreateSamplers()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE sampHandle(samplerHeap->GetCPUDescriptorHandleForHeapStart());
	auto sampSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	D3D12_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	device->CreateSampler(&sampDesc, sampHandle);

	sampHandle.Offset(1, sampSize);
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	device->CreateSampler(&sampDesc, sampHandle);
}

void SSAO::OnResize(int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture)
{
	ResetTextures();
	CreateTexture(width, height);
	CreateRTV();
	CreateSRV(depthTexture, normalTexture, noiseTexture);
}

void SSAO::ResetTextures() {
	SSAOTexture_A.Resource.Reset();
	SSAOTexture_B.Resource.Reset();
}

void SSAO::ClearSSAO(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &SSAOTexture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &SSAOTexture_B, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(SSAOTexture_A.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(SSAOTexture_B.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	BarriersToDefault(commandList);
}

void SSAO::BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &SSAOTexture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &SSAOTexture_B, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DX12App::DrawSSAO() {
	commandList->SetPipelineState(renderSystem->SsaoPSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->SsaoRS_.Get());

	D3D12_VIEWPORT ssaoViewport = {};
	ssaoViewport.TopLeftX = 0.0f;
	ssaoViewport.TopLeftY = 0.0f;
	ssaoViewport.Width = static_cast<float>(clientWidth / 2);
	ssaoViewport.Height = static_cast<float>(clientHeight / 2);
	ssaoViewport.MinDepth = 0.0f;
	ssaoViewport.MaxDepth = 1.0f;

	D3D12_RECT ssaoScissorRect = { 0, 0, clientWidth / 2, clientHeight / 2 };

	commandList->RSSetViewports(1, &ssaoViewport);
	commandList->RSSetScissorRects(1, &ssaoScissorRect);

	auto handle = renderSystem->ssao->GetTextureA().rtvHandle;
	commandList->OMSetRenderTargets(1, &handle, true, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->ssao->GetSrvHeap().Get(), renderSystem->ssao->GetSamplerHeap().Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(renderSystem->ssao->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
	auto srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandle.Offset(2, srvSize);
	commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

	commandList->SetGraphicsRootDescriptorTable(1, renderSystem->ssao->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart());

	commandList->SetGraphicsRootConstantBufferView(2, ssaoBuffer->Resource()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(3, cameraBuffer->Resource()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(4, matricesBuffer->Resource()->GetGPUVirtualAddress());

	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12App::BlurSSAO()
{
	MyTexture* ssaoWriteTexture = &renderSystem->ssao->GetTextureA();
	MyTexture* ssaoReadTexture = &renderSystem->ssao->GetTextureB();

	d3dUtil::SwapTextures(commandList, ssaoWriteTexture, ssaoReadTexture);

	commandList->SetPipelineState(renderSystem->SsaoBlurPSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->SsaoBlurRS_.Get());

	auto rtv = ssaoWriteTexture->rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);

	commandList->SetGraphicsRootDescriptorTable(0, ssaoReadTexture->srvGpuHandle);

	CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(renderSystem->ssao->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	normalHandle.Offset(3, size);
	commandList->SetGraphicsRootDescriptorTable(1, normalHandle);

	commandList->SetGraphicsRootDescriptorTable(2, renderSystem->ssao->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart());

	BlurConstants blurConst;
	blurConst.screenWidth = clientWidth / 2;
	blurConst.screenHeight = clientHeight / 2;
	blurConst.blurType = 0.0f;
	blurConst.padding = 0.0f;
	ssaoBlurBuffer->CopyData(0, blurConst);

	commandList->SetGraphicsRootConstantBufferView(3, ssaoBlurBuffer->Resource()->GetGPUVirtualAddress());

	commandList->DrawInstanced(3, 1, 0, 0);

	d3dUtil::SwapTextures(commandList, ssaoWriteTexture, ssaoReadTexture);
	rtv = ssaoWriteTexture->rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);
	commandList->SetGraphicsRootDescriptorTable(0, ssaoReadTexture->srvGpuHandle);

	blurConst.blurType = 1.0f;
	ssaoBlurBuffer->CopyData(0, blurConst);
	commandList->SetGraphicsRootConstantBufferView(3, ssaoBlurBuffer->Resource()->GetGPUVirtualAddress());

	commandList->DrawInstanced(3, 1, 0, 0);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}
