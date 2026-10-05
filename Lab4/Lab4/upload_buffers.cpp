#include "DX12App.h"

void DX12App::InitUploadBuffers() {

	deadParticlesListUpload = std::make_unique<UploadBuffer<uint32_t>>(device.Get(), PARTICLE_COUNT, false);

	FillUploadBuffers();
}

void DX12App::FillUploadBuffers()
{
	for (int i = 0; i < PARTICLE_COUNT; i++) {
		deadParticlesListUpload->CopyData(i, i);
	}
}
