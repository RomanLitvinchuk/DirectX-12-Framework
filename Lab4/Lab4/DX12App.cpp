#include "DX12App.h"
#include "d3dx12.h"
#include <iostream>
#include <string>
#include <DirectXColors.h>
#include <SimpleMath.h>
#include "d3dUtil.h"
#include "vertex.h"
#include <filesystem>
#include <numeric>
#include <limits>

using namespace DirectX;
using namespace DirectX::SimpleMath;

void DX12App::EnableDebug() {
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif
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
 
