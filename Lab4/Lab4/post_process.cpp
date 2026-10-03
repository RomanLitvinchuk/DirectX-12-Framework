#include "post_process.h"
#include "d3dUtil.h"
#include "singletone_device.h"
#include "throw_if_failed.h"
#include "d3dx12.h"

PostProcess::PostProcess(int width, int height) 
{
	device = SingletonDevice::GetDevice();
	CreateHeaps();
	CreateTextures(width, height);
	CreateSRV();
	CreateRTV();
}

MyTexture& PostProcess::GetHdrTextureA() 
{
	return hdrTextureA;
}

MyTexture& PostProcess::GetHdrTextureB() 
{
	return hdrTextureB;
}

MyTexture& PostProcess::GetLdrTextureA() 
{
	return ldrTextureA;
}

MyTexture& PostProcess::GetLdrTextureB() 
{
	return ldrTextureB;
}

ComPtr<ID3D12DescriptorHeap> PostProcess::GetSrvHeap() 
{
	return srvHeap;
}

ComPtr<ID3D12DescriptorHeap> PostProcess::GetRtvHeap() 
{
	return rtvHeap;
}

void PostProcess::CreateHeaps() 
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeap.NumDescriptors = 4;
	ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&srvHeap)));

	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&rtvHeap)));
}

void PostProcess::CreateTextures(int width, int height) 
{
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&hdrTextureA.Resource)));
	hdrTextureA.Resource->SetName(L"HDR Post Process Texture A");
	hdrTextureA.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&hdrTextureB.Resource)));
	hdrTextureB.Resource->SetName(L"HDR Post Process Texture B");
	hdrTextureB.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&ldrTextureA.Resource)));
	ldrTextureA.Resource->SetName(L"LDR Post Process Texture A");
	ldrTextureA.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&ldrTextureB.Resource)));
	ldrTextureB.Resource->SetName(L"LDR Post Process Texture B");
	ldrTextureB.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
} 

void PostProcess::CreateSRV() 
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	auto srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(srvHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(srvHeap->GetGPUDescriptorHandleForHeapStart());
	hdrTextureA.srvHandle = cpuHandle;
	hdrTextureA.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(hdrTextureA.Resource.Get(), &srvDesc, cpuHandle);
	cpuHandle.Offset(1, srvSize);
	gpuHandle.Offset(1, srvSize);
	hdrTextureB.srvHandle = cpuHandle;
	hdrTextureB.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(hdrTextureB.Resource.Get(), &srvDesc, cpuHandle);
	
	cpuHandle.Offset(1, srvSize);
	gpuHandle.Offset(1, srvSize);
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ldrTextureA.srvHandle = cpuHandle;
	ldrTextureA.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(ldrTextureA.Resource.Get(), &srvDesc, cpuHandle);
	cpuHandle.Offset(1, srvSize);
	gpuHandle.Offset(1, srvSize);
	ldrTextureB.srvHandle = cpuHandle;
	ldrTextureB.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(ldrTextureB.Resource.Get(), &srvDesc, cpuHandle);
}

void PostProcess::CreateRTV()
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	auto rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	hdrTextureA.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(hdrTextureA.Resource.Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvSize);
	hdrTextureB.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(hdrTextureB.Resource.Get(), &rtvDesc, rtvHandle);

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvHandle.Offset(1, rtvSize);
	ldrTextureA.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(ldrTextureA.Resource.Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvSize);
	ldrTextureB.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(ldrTextureB.Resource.Get(), &rtvDesc, rtvHandle);

}

void PostProcess::ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &hdrTextureA, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &hdrTextureB, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &ldrTextureA, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &ldrTextureB, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(hdrTextureA.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(hdrTextureB.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(ldrTextureA.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(ldrTextureB.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	BarriersToDefault(commandList);
}

void PostProcess::BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &hdrTextureA, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &hdrTextureB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	d3dUtil::Barrier(commandList, &ldrTextureA, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &ldrTextureB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcess::OnResize(int width, int height)
{
	ResetTextures();
	CreateTextures(width, height);
	CreateSRV();
	CreateRTV();
}

void PostProcess::ResetTextures()
{
	hdrTextureA.Resource.Reset();
	hdrTextureB.Resource.Reset();
	ldrTextureA.Resource.Reset();
	ldrTextureB.Resource.Reset();
}
