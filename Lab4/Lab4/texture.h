#pragma once
#include <string>
#include <d3d12.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

struct Texture
{
	std::string name_;
	std::wstring filepath;
	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
	UINT srvHeapIndex = 0;
	bool isSRGB = false;
};
