#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "pbf_constants.h"
#include "upload_buffer.h"

using Microsoft::WRL::ComPtr;

class PBFSimulator {
private:
	ComPtr<ID3D12Resource> particleBuffer[2];
	ComPtr<ID3D12Resource> particleBufferUpload;
	D3D12_GPU_VIRTUAL_ADDRESS particleGPUAddress[2];

	ComPtr<ID3D12Resource> neighborBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS neighborGPUAddress;

	ComPtr<ID3D12Resource> hashGridBuffer;
	ComPtr<ID3D12Resource> hashGridAtomicBuffer;

	std::unique_ptr<UploadBuffer<PBFConstants>> ConstBuffer = nullptr;

	uint32_t numParticles;
	uint32_t currentBuffer;

};
