#include "g_buffer.h"
#include "DX12App.h"
#include "singletone_device.h"
#include "throw_if_failed.h"
#include <DirectXColors.h>

GBuffer::GBuffer(int width, int height) 
{
	device = SingletonDevice::GetDevice();
	CreateHeaps();
	CreateTextures(width, height);
	CreateSRV();
	CreateRTVandDSV();
};

GBufferTexture& GBuffer::GetDiffuseTex() 
{
	return diffuseTex;
}

GBufferTexture& GBuffer::GetNormalTex() 
{
	return normalTex;
}

GBufferTexture& GBuffer::GetDepthTex() 
{
	return depthTex;
}

ComPtr<ID3D12DescriptorHeap> GBuffer::GetSrvHeap() 
{
	return srvDescriptorHeap;
}

ComPtr<ID3D12DescriptorHeap> GBuffer::GetRtvHeap() 
{
	return rtvDescpritorHeap;
}

ComPtr<ID3D12DescriptorHeap> GBuffer::GetDsvHeap() 
{
	return dsvDescriptorHeap;
}


void GBuffer::CreateHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NumDescriptors = 5;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvDescpritorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
	srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvDesc.NumDescriptors = 6;
	srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&srvDescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDesc.NumDescriptors = 1;
	dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvDescriptorHeap)));
}

void GBuffer::CreateTextures(int width, int height) 
{
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		&clearValue, IID_PPV_ARGS(&diffuseTex.Resource)));
	diffuseTex.Resource->SetName(L"Diffuse texture");

	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	resDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		&clearValue, IID_PPV_ARGS(&normalTex.Resource)));
	normalTex.Resource->SetName(L"Normal texture");

	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	resDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ,
		&clearValue, IID_PPV_ARGS(&depthTex.Resource)));
	depthTex.Resource->SetName(L"Depth texture");
}

void GBuffer::CreateRTVandDSV() 
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvTexDesc = {};
	rtvTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvTexDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvTexDesc.Texture2D.MipSlice = 0;
	rtvTexDesc.Texture2D.PlaneSlice = 0;
	auto RtvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescpritorHeap->GetCPUDescriptorHandleForHeapStart());
	diffuseTex.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(diffuseTex.Resource.Get(), &rtvTexDesc, diffuseTex.rtvHandle);
	rtvTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvHandle.Offset(1, RtvIncSize);
	normalTex.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(normalTex.Resource.Get(), &rtvTexDesc, normalTex.rtvHandle);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvTexDesc = {};
	dsvTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvTexDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvTexDesc.Texture2D.MipSlice = 0;
	auto DsvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	depthTex.dsvHandle = dsvHandle;
	device->CreateDepthStencilView(depthTex.Resource.Get(), &dsvTexDesc, depthTex.dsvHandle);
}

void GBuffer::CreateSRV() 
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvTexDesc = {};
	srvTexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvTexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvTexDesc.Texture2D.MipLevels = 1;
	auto SrvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	diffuseTex.srvHandle = srvHandle;
	device->CreateShaderResourceView(diffuseTex.Resource.Get(), &srvTexDesc, diffuseTex.srvHandle);

	srvTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvHandle.Offset(1, SrvIncSize);
	normalTex.srvHandle = srvHandle;
	device->CreateShaderResourceView(normalTex.Resource.Get(), &srvTexDesc, normalTex.srvHandle);

	srvTexDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvHandle.Offset(1, SrvIncSize);
	depthTex.srvHandle = srvHandle;
	device->CreateShaderResourceView(depthTex.Resource.Get(), &srvTexDesc, depthTex.srvHandle);
}

void GBuffer::TransitToOpaqueRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList) 
{
	CD3DX12_RESOURCE_BARRIER diffuseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(diffuseTex.Resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	CD3DX12_RESOURCE_BARRIER normalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(normalTex.Resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	CD3DX12_RESOURCE_BARRIER depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthTex.Resource.Get(), 
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	D3D12_RESOURCE_BARRIER barriers[] = { diffuseBarrier, normalBarrier, depthBarrier };
	commandList->ResourceBarrier(3, barriers);
}

void GBuffer::TransitToLightsRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList) 
{
	CD3DX12_RESOURCE_BARRIER diffuseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(diffuseTex.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	CD3DX12_RESOURCE_BARRIER normalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(normalTex.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	CD3DX12_RESOURCE_BARRIER depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthTex.Resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ);
	D3D12_RESOURCE_BARRIER barriers[] = { diffuseBarrier, normalBarrier, depthBarrier };
	commandList->ResourceBarrier(3, barriers);
}

void GBuffer::OnResize(int width, int height) 
{
	ResetTextures();
	CreateTextures(width, height);
	CreateRTVandDSV();
	CreateSRV();
}

void GBuffer::ResetTextures() 
{
	diffuseTex.Resource.Reset();
	normalTex.Resource.Reset();
	depthTex.Resource.Reset();
}

void GBuffer::ClearGBuffer(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->ClearRenderTargetView(diffuseTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(normalTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearDepthStencilView(depthTex.dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}
