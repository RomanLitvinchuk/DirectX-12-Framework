#include "DX12App.h"
#include "d3dx12.h"
#include <iostream>
#include <string>
#include <SimpleMath.h>
#include "d3dUtil.h"
#include "vertex.h"
#include <filesystem>
#include "DDSTextureLoader.h"
#include "model_parser.h"
#include "throw_if_failed.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "singletone_device.h"
#include <unordered_map>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
	ID3D12DescriptorHeap* g_ImGuiHeap = nullptr;
	UINT g_ImGuiDescriptorSize = 0;

	UINT g_ImGuiNextDescriptor = 0;

	constexpr UINT IMGUI_DESCRIPTOR_COUNT = 64;

	void ImGuiSrvDescriptorAlloc(
		ImGui_ImplDX12_InitInfo*,
		D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
	{
		UINT index = g_ImGuiNextDescriptor++;

		IM_ASSERT(index < IMGUI_DESCRIPTOR_COUNT);

		cpuHandle->ptr =
			g_ImGuiHeap->GetCPUDescriptorHandleForHeapStart().ptr +
			index * g_ImGuiDescriptorSize;

		gpuHandle->ptr =
			g_ImGuiHeap->GetGPUDescriptorHandleForHeapStart().ptr +
			index * g_ImGuiDescriptorSize;
	}

	void ImGuiSrvDescriptorFree(
		ImGui_ImplDX12_InitInfo*,
		D3D12_CPU_DESCRIPTOR_HANDLE,
		D3D12_GPU_DESCRIPTOR_HANDLE)
	{

	}
}

void DX12App::EnableDebug() {
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif
}


void DX12App::CreateImGuiDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = IMGUI_DESCRIPTOR_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imGuiSrvHeap)));

	g_ImGuiHeap = imGuiSrvHeap.Get();
	g_ImGuiDescriptorSize =
		device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
}

void DX12App::InitImGui(HWND hwnd) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = device.Get();
	initInfo.CommandQueue = commandQueue.Get();
	initInfo.NumFramesInFlight = 2;
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
	initInfo.SrvDescriptorHeap = imGuiSrvHeap.Get();
	initInfo.SrvDescriptorAllocFn = ImGuiSrvDescriptorAlloc;
	initInfo.SrvDescriptorFreeFn = ImGuiSrvDescriptorFree;

	ThrowIfFailed(ImGui_ImplDX12_Init(&initInfo) ? S_OK : E_FAIL);
}

void DX12App::NewImGuiFrame(){
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
}

void DX12App::BuildImGui() {
	//static bool showDemoWindow = true;
	//if (showDemoWindow) 
	//{
	//	ImGui::ShowDemoWindow(&showDemoWindow);
	//}
	ImGui::Begin("Framework");
	ImGui::Text("DirectX 12");
	ImGui::Separator();
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::Checkbox("Frustum culling", &camera.bIsFrustumCullingEnabled);
	ImGui::End();
}

void DX12App::RenderImGui() {
	ImGui::Render();
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = GetBackBuffer();
	commandList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);
	ID3D12DescriptorHeap* heaps[] =
	{
		imGuiSrvHeap.Get()
	};
	commandList->SetDescriptorHeaps(1, heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void DX12App::ShutdownImGui() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	g_ImGuiHeap = nullptr;
	g_ImGuiDescriptorSize = 0;
	g_ImGuiNextDescriptor = 0;
	imGuiSrvHeap.Reset();
}

void DX12App::InitializeDevice() {
	EnableDebug();
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&DXGIFactory)));

	device = SingletonDevice::GetDevice();

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	std::cout << "FENCE CREATED" << std::endl;

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	std::cout << "RTV size: " << std::to_string(rtvDescriptorSize) << "\n"
		<< "DSV size: " << std::to_string(dsvDescriptorSize) << "\n"
		<< "CbvSrvUav size:" << std::to_string(cbvDescriptorSize) << std::endl;

	msQualityLevels_.Format = backBufferFormat;
	msQualityLevels_.SampleCount = 4;
	msQualityLevels_.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels_.NumQualityLevels = 0;

	ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels_, sizeof(msQualityLevels_)));
	if (msQualityLevels_.NumQualityLevels > 0) { std::cout << "MSAA 4x is supported" << std::endl;} 
	else { std::cout << "WARNING! MSAA 4x is NOT supported" << std::endl; }
}

void DX12App::InitializeCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	std::cout << "Command queue is created" << std::endl;
	ThrowIfFailed(device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&commandAllocator)));
	std::cout << "Command allocator is created" << std::endl;
	ThrowIfFailed(device->CreateCommandList(0, queueDesc.Type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	std::cout << "Command list is created" << std::endl;
	ThrowIfFailed(commandList->Close());
}

void DX12App::CreateSwapChain(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC swDesc = {};
	swDesc.BufferDesc.Width = clientWidth;
	swDesc.BufferDesc.Height = clientHeight;
	swDesc.BufferDesc.RefreshRate.Numerator = 60;
	swDesc.BufferDesc.RefreshRate.Denominator = 1;
	swDesc.BufferDesc.Format = backBufferFormat;
	swDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swDesc.SampleDesc.Count = 1;
	swDesc.SampleDesc.Quality = 0;
	swDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swDesc.BufferCount = 2;
	swDesc.OutputWindow = hWnd;
	swDesc.Windowed = true;
	swDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ThrowIfFailed(DXGIFactory->CreateSwapChain(commandQueue.Get(), &swDesc, &swapChain));
	std::cout << "Swap chain is created" << std::endl;
	
}

void DX12App::CreateRTVAndDSVDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc;
	RTVHeapDesc.NumDescriptors = 4;
	RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RTVHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&rtvHeap)));
	std::cout << "RTV heap is created" << std::endl;

	D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc;
	DSVHeapDesc.NumDescriptors = 4;
	DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DSVHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&dsvHeap)));
	std::cout << "DSV heap is created" << std::endl;
}


D3D12_CPU_DESCRIPTOR_HANDLE DX12App::GetBackBuffer() const {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBuffer, rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12App::GetDSV() const {
	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* DX12App::CurrentBackBuffer() const {
	return swapChainBuffer[currentBackBuffer].Get();
}


void DX12App::CreateRTV() {
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTV_heap_handle_(rtvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	for (UINT i = 0; i < 2; i++) {
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer[i])));
		device->CreateRenderTargetView(swapChainBuffer[i].Get(), &rtvDesc, RTV_heap_handle_);
		RTV_heap_handle_.Offset(1, rtvDescriptorSize);
	}
	std::cout << "RTV is created" << std::endl;
}

void DX12App::CreateDSV() {
	D3D12_RESOURCE_DESC dsDesc;
	dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsDesc.Alignment = 0;
	dsDesc.Width = clientWidth;
	dsDesc.Height = clientHeight;
	dsDesc.DepthOrArraySize = 1;
	dsDesc.MipLevels = 1;
	dsDesc.Format = depthStencilFormat;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clrValue;
	clrValue.Format = depthStencilFormat;
	clrValue.DepthStencil.Depth = 1.0f;
	clrValue.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clrValue, IID_PPV_ARGS(&dsvBuffer)));
	std::cout << "DSV is created" << std::endl;
	device->CreateDepthStencilView(dsvBuffer.Get(), nullptr, GetDSV());
}



void DX12App::CreateSamplerHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC sampHeapDesc = {};
	sampHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	sampHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	sampHeapDesc.NumDescriptors = 2;
	ThrowIfFailed(device->CreateDescriptorHeap(&sampHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&samplerHeap));

	D3D12_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	device->CreateSampler(&sampDesc, samplerHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX12App::SetViewport() {
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(clientWidth);
	viewport.Height = static_cast<float>(clientHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DX12App::SetScissor() {
	scissorRect = { 0, 0, clientWidth, clientHeight };
}

void DX12App::CalculateGameStats(HWND hWnd) {
	static int frameCnt= 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((gt.TotalTime() - timeElapsed) >= 1.0f) {
		float fps = (float)frameCnt;
		float mspf = 1000.0f / fps;
		std::wstring WindowString = L"WINDOW  fps: " + std::to_wstring(fps) + L"mspf: " + std::to_wstring(mspf);
		SetWindowText(hWnd, WindowString.c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DX12App::FlushCommandQueue()
{
	currentFence++;

	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFence));

	if (fence->GetCompletedValue() < currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}


void DX12App::InitProjectionMatrix() {
	float aspectRatio = static_cast<float>(clientWidth) / clientHeight;

	camera.mProj_ = Matrix::CreatePerspectiveFieldOfView(
		XMConvertToRadians(60.0f),  
		aspectRatio,                
		1.0f,                       
		100000.0f                    
	);
	float fovAngleY = 0.33f * XM_PI;
	camera.xmProj = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 1.0f, 100000.0f);
}


void DX12App::CreateVertexBuffer() {
	UINT vbByteSize = (UINT)(sceneData.vertices.size() * sizeof(Vertex));
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	vertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(), sceneData.vertices.data(), vbByteSize, vertexBufferUploader);
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
	vbv.SizeInBytes = vbByteSize;
	vbv.StrideInBytes = sizeof(Vertex);
	vertexBuffers[0] = { vbv };
}


void DX12App::CreateIndexBuffer() {
	UINT ibByteSize = (UINT)(sceneData.indices.size() * sizeof(std::uint32_t));
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	indexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(), sceneData.indices.data(), ibByteSize, indexBufferUploader);
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
	indexBufferView.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = ibByteSize;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void DX12App::OnResize() {
	FlushCommandQueue();
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	swapChainBuffer[0].Reset();
	swapChainBuffer[1].Reset();

	ThrowIfFailed(swapChain->ResizeBuffers(2, clientWidth, clientHeight, backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	InitProjectionMatrix();
	currentBackBuffer = 0;
	CreateRTV();
	CreateDSV();
	renderSystem->g_buffer->OnResize(clientWidth, clientHeight);
	renderSystem->post_process->OnResize(clientWidth, clientHeight);
	ID3D12Resource* noiseTexResource = nullptr;
	auto iter = sceneData.textures.find(L"noise");
	if (iter != sceneData.textures.end()) {
		noiseTexResource = iter->second->Resource.Get();
	}
	renderSystem->ssao->OnResize(clientWidth / 2, clientHeight / 2,
		renderSystem->g_buffer->GetDepthTex().Resource.Get(),
		renderSystem->g_buffer->GetNormalTex().Resource.Get(),
		noiseTexResource);
	SetViewport();
	SetScissor();
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
}

void DX12App::BuildOctree() {
	visibleIndices.reserve(sceneData.submeshes.size());
	octree.Build(sceneData.submeshes);
}

#define RESERVED_TEXTURES 1

void DX12App::LoadTextures()
{
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	UINT index = RESERVED_TEXTURES + 1;

	for (auto& entry : std::filesystem::directory_iterator(L"textures"))
	{
		auto path = entry.path();
		if (path.extension() != L".dds") continue;

		std::wstring name = path.stem().wstring();
		std::transform(name.begin(), name.end(), name.begin(), ::towlower);

		auto tex = std::make_unique<Texture>();
		tex->name_ = std::string(name.begin(), name.end());
		tex->filepath = path.wstring();
		if (tex->name_.find("noise") != std::string::npos) {
			tex->srvHeapIndex = 1;
			tex->isSRGB = false;
		}
		else tex->srvHeapIndex = index++;

		ThrowIfFailed(CreateDDSTextureFromFile12(
			device.Get(),
			commandList.Get(),
			tex->filepath.c_str(),
			tex->Resource,
			tex->UploadHeap));

		std::wcout << L"Loaded texture: [" << name << L"] to index: " << tex->srvHeapIndex << std::endl;

		sceneData.textures[name] = std::move(tex);
	}

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, lists);
	FlushCommandQueue();
}

void DX12App::Parsing() {
	ModelParser parser;
	parser.ParseFile("models/sponza.obj", Matrix::Identity, 1, sceneData);

	Matrix Transform = Matrix::CreateScale(0.2f) * Matrix::CreateRotationX(-3.14 / 2) * Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	parser.ParseFile("models/Christmas Tree Color mm.obj", Transform, 1, sceneData);

	Transform = Matrix::CreateScale(25.0f) * Matrix::CreateTranslation(100.0f, 500.0f, 0.0f);
	parser.ParseFile("models/Sketchfab.fbx", Transform, 1, sceneData);

	Transform = Matrix::CreateScale(30.0f) * Matrix::CreateTranslation(400.0f, 200.0f, 0.0f);
	parser.ParseFile("models/HydraMoonSimpleCube.fbx", Transform, 1, sceneData);

	Transform = Matrix::CreateScale(30.0f) * Matrix::CreateTranslation(700.0f, 0.0f, 0.0f);
	parser.ParseFile("models/Minecraft Tree.obj", Transform, 1, sceneData);
}

void DX12App::InitShadowMap() {
	shadowMap = std::make_unique<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	auto handle = renderSystem->g_buffer->GetSrvHeap()->GetCPUDescriptorHandleForHeapStart();
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE smHandle(handle, 4, size);

	auto gpuHandle = renderSystem->g_buffer->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart();
	CD3DX12_GPU_DESCRIPTOR_HANDLE smGpuHandle(gpuHandle, 4, size);

	auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE smDsvHandle(dsvHandle, 1, size);

	shadowMap->BuildDescriptors(smHandle, smGpuHandle, smDsvHandle);
}

 
