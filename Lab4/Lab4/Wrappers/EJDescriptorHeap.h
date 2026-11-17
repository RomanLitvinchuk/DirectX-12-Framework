#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class EJDescriptorHeap
{
public:
	EJDescriptorHeap() = default;
	void Create(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorsCount);
	ID3D12DescriptorHeap* operator->() const
	{
		return heap.Get();
	}
	ID3D12DescriptorHeap* Get() const
	{
		return heap.Get();
	}

	void Reset()
	{
		cpuOffset = 0;
		gpuOffset = 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetFreeCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFreeGPUHandle();

private:
	ComPtr<ID3D12DescriptorHeap> heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ComPtr<ID3D12Device> device = nullptr;
	uint32_t cpuOffset = 0;
	uint32_t gpuOffset = 0;
};

struct EJDescriptorHeaps
{
	EJDescriptorHeap rtvHeap;
	EJDescriptorHeap srvHeap;
	EJDescriptorHeap dsvHeap;
};