#include "DX12App.h"

void DX12App::InitUploadBuffers() {
	objectsUploadBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device.Get(), 1, true);
	matricesBuffer = std::make_unique<UploadBuffer<Matrices>>(device.Get(), 1, true);
	particleConstantsBuffer = std::make_unique<UploadBuffer<ParticleConstants>>(device.Get(), 1, true);
	materialBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device.Get(), 300, true);
	cameraBuffer = std::make_unique<UploadBuffer<CameraConstants>>(device.Get(), 1, true);
	lightBuffer = std::make_unique<UploadBuffer<LightConstants>>(device.Get(), 1000, false);
	instanceBuffer = std::make_unique<UploadBuffer<MeshInstanceData>>(device.Get(), 1000, false);
	hullBuffer = std::make_unique<UploadBuffer<HullBuffer>>(device.Get(), 1, true);
	wireframeInstanceBuffer = std::make_unique<UploadBuffer<WireframeInstanceData>>(device.Get(), 1000, false);
	shadowBuffer = std::make_unique<UploadBuffer<ShadowConstants>>(device.Get(), 3, true);
	ssaoBuffer = std::make_unique<UploadBuffer<SsaoConstants>>(device.Get(), 1, true);
	ssaoBlurBuffer = std::make_unique<UploadBuffer<BlurConstants>>(device.Get(), 1, true);

	deadParticlesListUpload = std::make_unique<UploadBuffer<uint32_t>>(device.Get(), PARTICLE_COUNT, false);
	deadParticlesCounterUpload = std::make_unique<UploadBuffer<uint32_t>>(device.Get(), 1, false);
	sortParticlesCounterUpload = std::make_unique<UploadBuffer<uint32_t>>(device.Get(), 1, false);

	FillUploadBuffers();
}

void DX12App::FillUploadBuffers()
{
	SsaoConstants ssaoConst;
	ssaoConst.screenWidth = clientWidth / 2.0f;
	ssaoConst.screenHeight = clientHeight / 2.0f;
	ssaoConst.randomTextureSize = 64;
	ssaoConst.sampleRadius = 1.0f;
	ssaoConst.ssaoScale = 1.0f;
	ssaoConst.ssaoBias = 0.1f;
	ssaoConst.ssaoIntensity = 2.0f;
	ssaoConst.padding = 0.0f;
	ssaoBuffer->CopyData(0, ssaoConst);

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		deadParticlesListUpload->CopyData(i, i);
	}
	deadParticlesCounterUpload->CopyData(0, PARTICLE_COUNT);
	sortParticlesCounterUpload->CopyData(0, 0);

	for (int i = 0; i < sceneData.materials.size(); ++i) {
		materialBuffer->CopyData(i, sceneData.materials[i]);
	}
}
