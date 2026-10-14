#include "history.h"
#include "singletone_device.h"
#include "throw_if_failed.h"
#include "d3dx12.h"

History::History(int screenWidth, int screenHeight)
{
	device = SingletonDevice::GetDevice();
	CreateDescriptorHeaps();
	CreateTextures(screenWidth, screenHeight);
	CreateSRV();
	CreateRTV();
}


void History::OnResize(int screenWidth, int screenHeight)
{
	ResetTextures();
	CreateTextures(screenWidth, screenHeight);
	CreateSRV();
	CreateRTV();
}

MyTexture& History::GetHistoryA()
{
	return historyA; 
}

MyTexture& History::GetHistoryB()
{
	return historyB;
}

void History::ResetTextures()
{
	historyA.Resource.Reset();
	historyB.Resource.Reset();
}

void History::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.NumDescriptors = 3;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NodeMask = 0;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap)));
}

void History::CreateTextures(int screenWidth, int screenHeight)
{
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, screenWidth, screenHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&historyA.Resource)));
	historyA.Resource->SetName(L"HistoryA");
	historyA.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&historyB.Resource)));
	historyB.Resource->SetName(L"HistoryB");
	historyB.currentState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&lightTexture.Resource)));
	lightTexture.Resource->SetName(L"Light texture");
	lightTexture.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

}

void History::CreateSRV()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	auto srvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvHeap->GetGPUDescriptorHandleForHeapStart());

	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	historyA.srvHandle = cpuHandle;
	historyA.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(historyA.Resource.Get(), &srvDesc, cpuHandle);

	cpuHandle.Offset(1, srvIncSize);
	gpuHandle.Offset(1, srvIncSize);
	historyB.srvHandle = cpuHandle;
	historyB.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(historyB.Resource.Get(), &srvDesc, cpuHandle);

	cpuHandle.Offset(1, srvIncSize);
	gpuHandle.Offset(1, srvIncSize);
	lightTexture.srvHandle = cpuHandle;
	lightTexture.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(lightTexture.Resource.Get(), &srvDesc, cpuHandle);
}

void History::CreateRTV()
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	auto rtvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	historyA.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(historyA.Resource.Get(), &rtvDesc, rtvHandle);

	rtvHandle.Offset(1, rtvIncSize);
	historyB.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(historyB.Resource.Get(), &rtvDesc, rtvHandle);

	rtvHandle.Offset(1, rtvIncSize);
	lightTexture.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(lightTexture.Resource.Get(), &rtvDesc, rtvHandle);
}

