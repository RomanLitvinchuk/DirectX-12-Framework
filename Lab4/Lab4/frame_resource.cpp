#include "frame_resource.h"
#include "singletone_device.h"

FrameResource::FrameResource(UINT materialCount, UINT lightCount, UINT instanceCount)
{
	auto device = SingletonDevice::GetDevice();
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdListAlloc)));

	objectsUploadBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device.Get(), 1, true);
	matricesBuffer = std::make_unique<UploadBuffer<Matrices>>(device.Get(), 1, true);
	particleConstantsBuffer = std::make_unique<UploadBuffer<ParticleConstants>>(device.Get(), 1, true);
	materialBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device.Get(), materialCount, true);
	cameraBuffer = std::make_unique<UploadBuffer<CameraConstants>>(device.Get(), 1, true);
	lightBuffer = std::make_unique<UploadBuffer<LightConstants>>(device.Get(), lightCount, false);
	instanceBuffer = std::make_unique<UploadBuffer<MeshInstanceData>>(device.Get(), instanceCount, false);
	hullBuffer = std::make_unique<UploadBuffer<HullBuffer>>(device.Get(), 1, true);
	wireframeInstanceBuffer = std::make_unique<UploadBuffer<WireframeInstanceData>>(device.Get(), 1000, false);
	shadowBuffer = std::make_unique<UploadBuffer<ShadowConstants>>(device.Get(), 3, true);
	ssaoBuffer = std::make_unique<UploadBuffer<SsaoConstants>>(device.Get(), 1, true);
	ssaoBlurBuffer = std::make_unique<UploadBuffer<BlurConstants>>(device.Get(), 1, true);
	deadParticlesCounterUpload = std::make_unique<UploadBuffer<uint32_t>>(device.Get(), 1, false);
	sortParticlesCounterUpload = std::make_unique<UploadBuffer<uint32_t>>(device.Get(), 1, false);
}