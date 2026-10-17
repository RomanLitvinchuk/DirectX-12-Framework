#pragma once
#include <d3d12.h>
#include "wrl.h"
#include "throw_if_failed.h"

using Microsoft::WRL::ComPtr;

class SingletonDevice {
private:
	static ComPtr<ID3D12Device> device;

	SingletonDevice() = delete;

public:
	SingletonDevice(const SingletonDevice&) = delete;

	static ComPtr<ID3D12Device> GetDevice() {
		if (device == nullptr) {
			ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
		}
		return device;
	}
};

