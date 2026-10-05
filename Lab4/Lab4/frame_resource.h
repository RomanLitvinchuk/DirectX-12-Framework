#pragma once
#include <Windows.h>
#include <memory>

#include "upload_buffer.h"
#include "object_constants.h"
#include "materials.h"
#include "light.h"
#include "particle.h"
#include "camera.h"
#include "instances.h"
#include "shadow_map.h"
#include "ssao.h"

class FrameResource {
public:
	FrameResource(UINT materialCount, UINT lightCount, UINT instanceCount);

	FrameResource(const FrameResource&) = delete;
	FrameResource& operator=(const FrameResource&) = delete;

	ComPtr<ID3D12CommandAllocator> cmdListAlloc;

	std::unique_ptr<UploadBuffer<ObjectConstants>> objectsUploadBuffer;
	std::unique_ptr<UploadBuffer<Matrices>> matricesBuffer;
	std::unique_ptr<UploadBuffer<ParticleConstants>> particleConstantsBuffer;
	std::unique_ptr<UploadBuffer<MaterialConstants>> materialBuffer;
	std::unique_ptr<UploadBuffer<LightConstants>> lightBuffer;
	std::unique_ptr<UploadBuffer<CameraConstants>> cameraBuffer;
	std::unique_ptr<UploadBuffer<HullBuffer>> hullBuffer;
	std::unique_ptr<UploadBuffer<MeshInstanceData>> instanceBuffer;
	std::unique_ptr<UploadBuffer<WireframeInstanceData>> wireframeInstanceBuffer;
	std::unique_ptr<UploadBuffer<ShadowConstants>> shadowBuffer;
	std::unique_ptr<UploadBuffer<SsaoConstants>> ssaoBuffer;
	std::unique_ptr<UploadBuffer<BlurConstants>> ssaoBlurBuffer;
	std::unique_ptr<UploadBuffer<uint32_t>> deadParticlesCounterUpload;
	std::unique_ptr<UploadBuffer<uint32_t>> sortParticlesCounterUpload;

	UINT64 Fence = 0;
};