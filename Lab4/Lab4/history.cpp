#include "history.h"
#include "singletone_device.h"
#include "throw_if_failed.h"
#include "d3dx12.h"
#include "d3dUtil.h"

History::History(int screenWidth, int screenHeight)
{
	device = SingletonDevice::GetDevice();
	CreateDescriptorHeaps();
	CreateTextures(screenWidth, screenHeight);
	CreateSRV();
	CreateRTV();
	CreateUAV();
	CreateSamplers();
}


void History::OnResize(int screenWidth, int screenHeight)
{
	ResetTextures();
	CreateTextures(screenWidth, screenHeight);
	CreateSRV();
	CreateRTV();
	CreateUAV();
}

MyTexture& History::GetHistoryA()
{
	return historyA; 
}

MyTexture& History::GetLightTexture()
{
	return lightTexture;
}

MyTexture& History::GetCurrentBlurredTexture()
{
	return currentFrameBlurred;
}

MyTexture& History::GetTempBlurredTexture()
{
	return tempFrameBlurred;
}

MyTexture& History::GetMotionTextureA()
{
	return motionTextureA;
}

MyTexture& History::GetMotionTextureB()
{
	return motionTextureB;
}

ID3D12DescriptorHeap* History::GetSrvHeap()
{
	return srvHeap.Get();
}

ID3D12DescriptorHeap* History::GetSamplerHeap()
{
	return samplerHeap.Get();
}

void History::LightBarrier(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
	barriers.reserve(1);

	CD3DX12_RESOURCE_BARRIER tempBarrier;
	if (d3dUtil::Barrier(&lightTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, tempBarrier))
	{
		barriers.push_back(tempBarrier);
	}

	if (!barriers.empty())
	{
		commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}
}

void History::MotionBarrier(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
	barriers.reserve(1);

	CD3DX12_RESOURCE_BARRIER tempBarrier;
	if (d3dUtil::Barrier(&motionTextureA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, tempBarrier))
	{
		barriers.push_back(tempBarrier);
	}

	if (!barriers.empty())
	{
		commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}
}

void History::ResetTextures()
{
	historyA.Resource.Reset();
	lightTexture.Resource.Reset();
	motionTextureA.Resource.Reset();
	motionTextureB.Resource.Reset();
	tempFrameBlurred.Resource.Reset();
	currentFrameBlurred.Resource.Reset();
}

void History::ClearTextures(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> clearBarriers;
	clearBarriers.reserve(2);

	CD3DX12_RESOURCE_BARRIER tempBarrier;

	if (d3dUtil::Barrier(&lightTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, tempBarrier))
	{
		clearBarriers.push_back(tempBarrier);
	}

	if (d3dUtil::Barrier(&motionTextureA, D3D12_RESOURCE_STATE_RENDER_TARGET, tempBarrier))
	{
		clearBarriers.push_back(tempBarrier);
	}

	if (!clearBarriers.empty())
	{
		commandList->ResourceBarrier(static_cast<UINT>(clearBarriers.size()), clearBarriers.data());
	}

	commandList->ClearRenderTargetView(lightTexture.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(motionTextureA.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);

	DefaultBarriers(commandList);
}

void History::DefaultBarriers(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> defaultBarriers;
	defaultBarriers.reserve(2);

	CD3DX12_RESOURCE_BARRIER tempBarrier;

	if (d3dUtil::Barrier(&tempFrameBlurred, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tempBarrier))
	{
		defaultBarriers.push_back(tempBarrier);
	}

	if (d3dUtil::Barrier(&currentFrameBlurred, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, tempBarrier))
	{
		defaultBarriers.push_back(tempBarrier);
	}

	if (!defaultBarriers.empty())
	{
		commandList->ResourceBarrier(static_cast<UINT>(defaultBarriers.size()), defaultBarriers.data());
	}
}

void History::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.NumDescriptors = 8;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NodeMask = 0;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));

	heapDesc.NumDescriptors = 5;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap)));

	heapDesc.NumDescriptors = 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&samplerHeap)));
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
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&historyA.Resource)));
	historyA.Resource->SetName(L"HistoryA");
	historyA.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&lightTexture.Resource)));
	lightTexture.Resource->SetName(L"Light texture");
	lightTexture.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&currentFrameBlurred.Resource)));
	currentFrameBlurred.Resource->SetName(L"Current Frame Blurred");
	currentFrameBlurred.currentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&tempFrameBlurred.Resource)));
	tempFrameBlurred.Resource->SetName(L"Temporal Frame Blured");
	tempFrameBlurred.currentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	clearValue.Format = DXGI_FORMAT_R16G16_FLOAT;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&motionTextureA.Resource)));
	motionTextureA.Resource->SetName(L"Motion texture A");
	motionTextureA.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&motionTextureB.Resource)));
	motionTextureB.Resource->SetName(L"Motion texture B");
	motionTextureB.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
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
	lightTexture.srvHandle = cpuHandle;
	lightTexture.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(lightTexture.Resource.Get(), &srvDesc, cpuHandle);

	cpuHandle.Offset(1, srvIncSize);
	gpuHandle.Offset(1, srvIncSize);
	motionTextureA.srvHandle = cpuHandle;
	motionTextureA.srvGpuHandle = gpuHandle;
	srvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	device->CreateShaderResourceView(motionTextureA.Resource.Get(), &srvDesc, cpuHandle);

	cpuHandle.Offset(1, srvIncSize);
	gpuHandle.Offset(1, srvIncSize);
	motionTextureB.srvHandle = cpuHandle;
	motionTextureB.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(motionTextureB.Resource.Get(), &srvDesc, cpuHandle);

	cpuHandle.Offset(1, srvIncSize);
	gpuHandle.Offset(1, srvIncSize);
	currentFrameBlurred.srvHandle = cpuHandle;
	currentFrameBlurred.srvGpuHandle = gpuHandle;
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	device->CreateShaderResourceView(currentFrameBlurred.Resource.Get(), &srvDesc, cpuHandle);

	cpuHandle.Offset(1, srvIncSize);
	gpuHandle.Offset(1, srvIncSize);
	tempFrameBlurred.srvHandle = cpuHandle;
	tempFrameBlurred.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(tempFrameBlurred.Resource.Get(), &srvDesc, cpuHandle);
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
	lightTexture.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(lightTexture.Resource.Get(), &rtvDesc, rtvHandle);

	rtvHandle.Offset(1, rtvIncSize);
	motionTextureA.rtvHandle = rtvHandle;
	rtvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	device->CreateRenderTargetView(motionTextureA.Resource.Get(), &rtvDesc, rtvHandle);

	rtvHandle.Offset(1, rtvIncSize);
	motionTextureB.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(motionTextureB.Resource.Get(), &rtvDesc, rtvHandle);
}

void History::CreateUAV()
{
	auto uavIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvHeap->GetGPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(6, uavIncSize);
	gpuHandle.Offset(6, uavIncSize);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	currentFrameBlurred.uavHandle = cpuHandle;
	currentFrameBlurred.uavGpuHandle = gpuHandle;
	device->CreateUnorderedAccessView(currentFrameBlurred.Resource.Get(), nullptr, &uavDesc, cpuHandle);

	cpuHandle.Offset(1, uavIncSize);
	gpuHandle.Offset(1, uavIncSize);
	tempFrameBlurred.uavHandle = cpuHandle;
	tempFrameBlurred.uavGpuHandle = gpuHandle;
	device->CreateUnorderedAccessView(tempFrameBlurred.Resource.Get(), nullptr, &uavDesc, cpuHandle);
}

void History::CreateSamplers()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(samplerHeap->GetCPUDescriptorHandleForHeapStart());
	auto samplerIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	device->CreateSampler(&samplerDesc, cpuHandle);

	cpuHandle.Offset(1, samplerIncSize);
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	device->CreateSampler(&samplerDesc, cpuHandle);
}

