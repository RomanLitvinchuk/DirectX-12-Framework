#include "DX12App.h"

void DX12App::EmitParticles() {
	commandList->SetPipelineState(renderSystem->particlesEmitPSO_.Get());
	commandList->SetComputeRootSignature(renderSystem->particlesEmitRS_.Get());
	ID3D12DescriptorHeap* heaps[] = { uavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	CD3DX12_RESOURCE_BARRIER toUAV = CD3DX12_RESOURCE_BARRIER::Transition(RWParticleBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	D3D12_RESOURCE_BARRIER resourceBarrier = { toUAV };
	commandList->ResourceBarrier(1, &resourceBarrier);
	commandList->SetComputeRootUnorderedAccessView(0, RWParticleBuffer->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(
		uavHeap->GetGPUDescriptorHandleForHeapStart(),
		0,
		cbvDescriptorSize);
	commandList->SetComputeRootDescriptorTable(1, uavHandle);
	commandList->Dispatch(4, 1, 1);
	CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(RWParticleBuffer.Get());
	commandList->ResourceBarrier(1, &uavBarrier);
}

void DX12App::ComputeParticles() {
	commandList->SetPipelineState(renderSystem->particlesUpdatePSO_.Get());
	commandList->SetComputeRootSignature(renderSystem->particlesUpdateRS_.Get());
	commandList->SetComputeRootConstantBufferView(0, currentFrameResource->particleConstantsBuffer->Resource()->GetGPUVirtualAddress());
	commandList->SetComputeRootUnorderedAccessView(1, RWParticleBuffer->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(
		uavHeap->GetGPUDescriptorHandleForHeapStart(),
		0,
		cbvDescriptorSize);
	commandList->SetComputeRootDescriptorTable(2, uavHandle);

	CD3DX12_RESOURCE_BARRIER sortCounterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(sortParticlesCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
	D3D12_RESOURCE_BARRIER barriers[] = { sortCounterBarrier };
	commandList->ResourceBarrier(_countof(barriers), barriers);
	commandList->CopyResource(sortParticlesCounterBuffer.Get(), currentFrameResource->sortParticlesCounterUpload->Resource());
	CD3DX12_RESOURCE_BARRIER sortToUAV = CD3DX12_RESOURCE_BARRIER::Transition(sortParticlesCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->Dispatch(ceil(PARTICLE_COUNT / 256.0f), 1, 1);
	CD3DX12_RESOURCE_BARRIER toCopy = CD3DX12_RESOURCE_BARRIER::Transition(RWParticleBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	D3D12_RESOURCE_BARRIER resourceBarrier = { toCopy };
	commandList->ResourceBarrier(1, &resourceBarrier);
}

void DX12App::DrawParticles() {
	commandList->SetPipelineState(renderSystem->particlePSO_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->post_process->GetHdrTextureA().rtvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->g_buffer->GetDepthTex().dsvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, &dsv);

	commandList->SetGraphicsRootSignature(renderSystem->particleRS_.Get());
	
	commandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->matricesBuffer->Resource()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootShaderResourceView(1, RWParticleBuffer->GetGPUVirtualAddress());

	commandList->IASetVertexBuffers(0, 0, nullptr);
	commandList->IASetIndexBuffer(nullptr);

	commandList->DrawInstanced(1, PARTICLE_COUNT, 0, 0);
}

void DX12App::InitEmitter() {
	emitter.Position = DirectX::SimpleMath::Vector3(-599.0f, 0.0f, 0.0f);
	float MaxVerticalDist = 50.0f * 12.0f; 
	float MaxHorizontalDist = 1.0f + (1.0f * 12.0f); 

	emitter.bounds.Center = emitter.Position + DirectX::SimpleMath::Vector3(0, MaxVerticalDist / 2.0f, 0);
	emitter.bounds.Extents = DirectX::SimpleMath::Vector3(MaxHorizontalDist, MaxVerticalDist / 2.0f, MaxHorizontalDist);
}