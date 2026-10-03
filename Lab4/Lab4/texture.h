#pragma once
#include <string>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

struct Texture
{
	std::string name_;
	std::wstring filepath;
	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
	UINT srvHeapIndex = 0;
	bool isSRGB = false;
};

struct MyTexture {
	ComPtr<ID3D12Resource> Resource = nullptr;
	D3D12_RESOURCE_STATES currentState;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
};

