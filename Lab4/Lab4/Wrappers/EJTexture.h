#pragma once
#include <d3d12.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

struct EJTexture {
	ComPtr<ID3D12Resource> Resource = nullptr;
	D3D12_RESOURCE_STATES currentState;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle;
};