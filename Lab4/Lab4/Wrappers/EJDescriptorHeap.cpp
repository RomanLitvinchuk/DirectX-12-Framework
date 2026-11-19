#include "EJDescriptorHeap.h"
#include "singletone_device.h"
#include "throw_if_failed.h"
#include "d3dx12.h"

void EJDescriptorHeap::Create(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorsCount)
{
	device = SingletonDevice::GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = heapType;
	this->heapType = heapType;
	heapDesc.NumDescriptors = descriptorsCount;
	heapDesc.Flags = (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
	offset = 0;
	staticsResources.assign(descriptorsCount, false);
	switch (heapType)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			heap->SetName(L"RTV Heap");
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			heap->SetName(L"CBV SRV UAV Heap");
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			heap->SetName(L"DSV Heap");
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			heap->SetName(L"Sampler Heap");
			break;
		default:
			heap->SetName(L"Unknown Heap");
			break;
	}
}

std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> EJDescriptorHeap::GetFreeHandles(const bool bIsNotResizable) 
{
	while (staticsResources[offset])
	{
		offset++;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(offset, device->GetDescriptorHandleIncrementSize(heapType));
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	if (heapType != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		gpuHandle.ptr = 0;
	}
	else
	{
		gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.Offset(offset, device->GetDescriptorHandleIncrementSize(heapType));
	}
	staticsResources[offset] = bIsNotResizable;
	offset++;
	return std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>(cpuHandle, gpuHandle);
}
