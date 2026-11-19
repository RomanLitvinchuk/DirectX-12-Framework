#pragma once
#include <d3d12.h>
#include <d3dx12.h>
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
		offset = 0;
	}

	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeHandles(const bool bIsNotResizable = false);

private:
	ComPtr<ID3D12DescriptorHeap> heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ComPtr<ID3D12Device> device = nullptr;
	std::vector<bool> staticsResources;
 	uint32_t offset = 0;
};

struct EJDescriptorHeaps
{
	EJDescriptorHeap rtvHeap;
	EJDescriptorHeap srvHeap;
	EJDescriptorHeap dsvHeap;
};