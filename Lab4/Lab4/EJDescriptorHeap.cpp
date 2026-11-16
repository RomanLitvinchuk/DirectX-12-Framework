#include "EJDescriptorHeap.h"
#include "singletone_device.h"
#include "throw_if_failed.h"

void EJDescriptorHeap::Create(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorsCount)
{
	device = SingletonDevice::GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = heapType;
	heapDesc.NumDescriptors = descriptorsCount;
	heapDesc.Flags = (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
	cpuOffset = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE EJDescriptorHeap::GetFreeCPUHandle() 
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(heap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(cpuOffset, device->GetDescriptorHandleIncrementSize(heapType));
	cpuOffset++;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE EJDescriptorHeap::GetFreeGPUHandle()
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(heap->GetGPUDescriptorHandleForHeapStart());
	handle.Offset(gpuOffset, device->GetDescriptorHandleIncrementSize(heapType));
	gpuOffset++;
	return handle;
}
