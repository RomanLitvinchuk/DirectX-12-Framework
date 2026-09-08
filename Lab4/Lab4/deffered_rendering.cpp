#include "DX12App.h"
#include <numeric>

#define ALIGN_256(size) ((size + 255) & ~255)

void DX12App::InitRenderSystem() {
	ID3D12Resource* noiseTexResource = nullptr;
	auto iter = sceneData.textures.find(L"noise");
	if (iter != sceneData.textures.end()) {
		noiseTexResource = iter->second->Resource.Get();
	}
	renderSystem = new RenderingSystem(clientWidth, clientHeight, noiseTexResource);

	CreateStructuredBuffersSRV();
}

void DX12App::sortLODs(){
	for (auto& sm : sceneData.submeshes) {
		sm.sorted_lod0.clear();
		sm.sorted_lod1.clear();
		sm.sorted_billboards.clear();
		sm.sorted_lod0.reserve(sm.InstanceCount);
		if (sm.hasLOD1) sm.sorted_lod1.reserve(sm.InstanceCount);

		for (int i = 0; i < sm.InstanceCount; i++) {
			Vector3 instancePos(sm.instances[i].World_._41, sm.instances[i].World_._42, sm.instances[i].World_._43);
			float distSq = Vector3::DistanceSquared(camera.mCameraPos, instancePos);

			if (sm.hasLOD1 && distSq > BILLBOARD_DISTANCE) {
				sm.sorted_billboards.push_back(sm.instances[i]);
			}
			else if (sm.hasLOD1 && distSq > LOD_DISTANCE)
			{
				sm.sorted_lod1.push_back(sm.instances[i]);
			}
			else {
				sm.sorted_lod0.push_back(sm.instances[i]);
			}
		}
	}
}

void DX12App::DrawShadows() {

	UINT totalInstances = 0;
	for (auto& sm : sceneData.submeshes) {
		if (sm.sorted_lod0.empty()) continue;

		for (size_t i = 0; i < sm.sorted_lod0.size(); ++i) {
			instanceBuffer->CopyData(totalInstances + i, sm.sorted_lod0[i]);
		}

		sm.shadowInstanceOffset = totalInstances;
		totalInstances += static_cast<UINT>(sm.sorted_lod0.size());
	}

	if (totalInstances == 0) return;

	commandList->SetPipelineState(renderSystem->shadowPSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->shadowRS_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBuffers[0]);
	commandList->IASetIndexBuffer(&indexBufferView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvHeap.Get(), renderSystem->samplerHeap.Get() };
	commandList->SetDescriptorHeaps(2, descriptorHeaps);

	CD3DX12_RESOURCE_BARRIER barriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(shadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE),
	};
	commandList->ResourceBarrier(_countof(barriers), barriers);

	D3D12_VIEWPORT vp = shadowMap->Viewport();
	commandList->RSSetViewports(1, &vp);
	D3D12_RECT rect = shadowMap->ScissorRect();
	commandList->RSSetScissorRects(1, &rect);

	commandList->SetGraphicsRootShaderResourceView(1, instanceBuffer->Resource()->GetGPUVirtualAddress());

	const D3D12_GPU_VIRTUAL_ADDRESS shadowCbBaseAddress = shadowBuffer->Resource()->GetGPUVirtualAddress();
	const UINT shadowElementSize = ALIGN_256(sizeof(ShadowConstants));
	const D3D12_GPU_VIRTUAL_ADDRESS matBaseAddress = materialBuffer->Resource()->GetGPUVirtualAddress();
	const UINT matBufferSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
	const CD3DX12_GPU_DESCRIPTOR_HANDLE srvHeapStart(cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

	const int numCascades = shadowMap->GetNumCascades();

	for (int i = 0; i < numCascades; ++i) {
		commandList->ClearDepthStencilView(shadowMap->Dsv(i), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = shadowMap->Dsv(i);
		commandList->OMSetRenderTargets(0, nullptr, false, &dsv);

		commandList->SetGraphicsRootConstantBufferView(0, shadowCbBaseAddress + i * shadowElementSize);

		for (auto& sm : sceneData.submeshes) {
			if (sm.sorted_lod0.empty()) continue;

			commandList->DrawIndexedInstanced(
				sm.indexCount,
				static_cast<UINT>(sm.sorted_lod0.size()),
				sm.startIndiceIndex,
				sm.startVerticeIndex,
				sm.shadowInstanceOffset
			);
		}
	}

	CD3DX12_RESOURCE_BARRIER backBarriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(shadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	commandList->ResourceBarrier(_countof(backBarriers), backBarriers);
}

void DX12App::DrawToGBuffer() {
	GetVisibleObjects();
	commandList->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		renderSystem->g_buffer->GetDiffuseTex().rtvHandle, renderSystem->g_buffer->GetNormalTex().rtvHandle
	};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->g_buffer->GetDepthTex().dsvHandle;
	commandList->OMSetRenderTargets(2, rtvs, true, &dsv);

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvHeap.Get(), samplerHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootSignature(renderSystem->opaqueRS_.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetVertexBuffers(0, 1, &vertexBuffers[0]);
	commandList->IASetIndexBuffer(&indexBufferView);

	UINT currentInstanceOffset = 0;
	Vector3 cameraPos = camera.mCameraPos;
	for (UINT idx : visibleIndices)
	{
		auto& sm = sceneData.submeshes[idx];

		commandList->SetGraphicsRootSignature(renderSystem->opaqueRS_.Get());
		commandList->SetPipelineState(renderSystem->opaquePSO_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootDescriptorTable(0, cbvHandle); 
		commandList->SetGraphicsRootDescriptorTable(2, samplerHandle); 
		commandList->SetGraphicsRootConstantBufferView(6, hullBuffer->Resource()->GetGPUVirtualAddress());

		UINT matIndex = sm.materialIndex;
		UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
		D3D12_GPU_VIRTUAL_ADDRESS matAddress = materialBuffer->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
		commandList->SetGraphicsRootConstantBufferView(3, matAddress);

		treeIsVisible = sceneData.materials[matIndex].isTree == 1;
		int texHeapIndex = sceneData.materials[matIndex].diffuseTextureIndex + 1;

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
			cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			texHeapIndex,
			cbvDescriptorSize);

		commandList->SetGraphicsRootDescriptorTable(1, srvHandle);

		int normHeapIndex = sceneData.materials[matIndex].normalTextureIndex + 1;

		CD3DX12_GPU_DESCRIPTOR_HANDLE normHandle(
			cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			normHeapIndex,
			cbvDescriptorSize);

		int dispHeapIndex = sceneData.materials[matIndex].displacementTextureIndex + 1;

		CD3DX12_GPU_DESCRIPTOR_HANDLE dispHandle(
			cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			dispHeapIndex,
			cbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(5, dispHandle);

		commandList->SetGraphicsRootDescriptorTable(4, normHandle);

		DrawLOD0ToGBuffer(currentInstanceOffset, sm);
		DrawLOD1ToGBuffer(currentInstanceOffset, sm);
		DrawBillboardsToGBuffer(currentInstanceOffset, sm);
	}
}

void DX12App::GetVisibleObjects() {
	visibleIndices.clear();
	if (camera.isFrustumCullingEnabled)
	{
		octree.GetVisibleObjects(camera.frustum, sceneData.submeshes, visibleIndices);
	}
	else
	{
		visibleIndices.resize(sceneData.submeshes.size());
		std::iota(visibleIndices.begin(), visibleIndices.end(), 0);
	}
}

void DX12App::DrawLOD0ToGBuffer(UINT& currentInstanceOffset, Submesh& sm) {
	if (!sm.sorted_lod0.empty()) {
		for (size_t i = 0; i < sm.sorted_lod0.size(); i++) {
			instanceBuffer->CopyData(currentInstanceOffset + i, sm.sorted_lod0[i]);
		}
		commandList->SetGraphicsRootShaderResourceView(7, instanceBuffer->Resource()->GetGPUVirtualAddress());

		if (sm.name_.find("Sketchfab") != std::string::npos) {
			DrawBakedMeshToGBuffer(currentInstanceOffset, sm);
		}
		else {
			commandList->DrawIndexedInstanced(
				sm.indexCount,
				static_cast<UINT>(sm.sorted_lod0.size()),
				sm.startIndiceIndex,
				sm.startVerticeIndex,
				currentInstanceOffset);
		}
		currentInstanceOffset += static_cast<UINT>(sm.sorted_lod0.size());
	}
}

void DX12App::DrawBakedMeshToGBuffer(UINT& currentInstanceOffset, Submesh& sm) {
	commandList->SetPipelineState(renderSystem->bakedPSO_.Get());
	D3D12_VERTEX_BUFFER_VIEW bakedVbv;
	bakedVbv.BufferLocation = streamOutputBuffer->GetGPUVirtualAddress();
	bakedVbv.StrideInBytes = sizeof(BakedVertex);
	bakedVbv.SizeInBytes = streamOutputMesh.SOVertexCount * sizeof(BakedVertex);

	commandList->IASetVertexBuffers(0, 1, &bakedVbv);
	commandList->DrawInstanced(
		streamOutputMesh.SOVertexCount,
		sm.InstanceCount,
		0,
		currentInstanceOffset);
	commandList->IASetVertexBuffers(0, 1, &vertexBuffers[0]);
}

void DX12App::DrawLOD1ToGBuffer(UINT& currentInstanceOffset, Submesh& sm) {
	if (!sm.sorted_lod1.empty()) {
		for (size_t i = 0; i < sm.sorted_lod1.size(); i++) {
			instanceBuffer->CopyData(currentInstanceOffset + i, sm.sorted_lod1[i]);
		}
		commandList->SetGraphicsRootShaderResourceView(7, instanceBuffer->Resource()->GetGPUVirtualAddress());

		commandList->DrawIndexedInstanced(
			sm.indexCountLOD1,
			static_cast<UINT>(sm.sorted_lod1.size()),
			sm.startIndiceIndexLOD1,
			sm.startVerticeIndexLOD1,
			currentInstanceOffset);

		currentInstanceOffset += static_cast<UINT>(sm.sorted_lod1.size());
	}
}

void DX12App::DrawBillboardsToGBuffer(UINT& currentInstanceOffset, Submesh& sm) {
	if (!sm.sorted_billboards.empty()) {
		commandList->SetGraphicsRootSignature(renderSystem->billboardRS_.Get());
		commandList->SetPipelineState(renderSystem->billboardPSO_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		for (size_t i = 0; i < sm.sorted_billboards.size(); i++) {
			instanceBuffer->CopyData(currentInstanceOffset + i, sm.sorted_billboards[i]);
		}
		commandList->SetGraphicsRootShaderResourceView(0, instanceBuffer->Resource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(1, cameraBuffer->Resource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(2, objectsUploadBuffer->Resource()->GetGPUVirtualAddress());
		int matIndex = sm.materialIndex;
		int bTextureIndex = sceneData.materials[matIndex].billboardTextureIndex + 1;
		CD3DX12_GPU_DESCRIPTOR_HANDLE bHandle(cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), bTextureIndex, cbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(3, bHandle);
		commandList->SetGraphicsRootDescriptorTable(4, samplerHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->DrawInstanced(
			4,
			static_cast<UINT>(sm.sorted_billboards.size()),
			0,
			currentInstanceOffset
		);

		currentInstanceOffset += static_cast<UINT>(sm.sorted_billboards.size());
	}
}

void DX12App::DrawLights() {
	commandList->SetPipelineState(renderSystem->lightPSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->lightRS_.Get());

	for (int i = 0; i < renderSystem->sceneLights_.size(); ++i) {
		lightBuffer->CopyData(i, renderSystem->sceneLights_[i]);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->post_process->GetHdrTextureA().rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);
	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->g_buffer->GetSrvHeap().Get(), renderSystem->samplerHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootConstantBufferView(0, cameraBuffer->Resource()->GetGPUVirtualAddress());
	UINT count = (UINT)renderSystem->sceneLights_.size();
	commandList->SetGraphicsRoot32BitConstant(1, count, 0);
	commandList->SetGraphicsRootDescriptorTable(2, renderSystem->g_buffer->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(3, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(4, shadowMap->Srv());
	commandList->SetGraphicsRootConstantBufferView(5, shadowBuffer->Resource()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(6, matricesBuffer->Resource()->GetGPUVirtualAddress());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
	CD3DX12_RESOURCE_BARRIER backBarriers[] = { CD3DX12_RESOURCE_BARRIER::Transition(shadowMap->Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ)};
	commandList->ResourceBarrier(_countof(backBarriers), backBarriers);
}

void DX12App::DrawNYBalls()
{
	commandList->SetPipelineState(renderSystem->bulbPSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->bulbRS_.Get());

	auto dsv = renderSystem->g_buffer->GetDepthTex().dsvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->post_process->GetHdrTextureA().rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, &dsv);

	commandList->SetGraphicsRootConstantBufferView(0, objectsUploadBuffer->Resource()->GetGPUVirtualAddress());

	auto handle = renderSystem->g_buffer->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart();
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightSrvHandle(handle, 3, size);
	commandList->SetGraphicsRootDescriptorTable(1, lightSrvHandle);

	commandList->IASetVertexBuffers(0, 1, &sphereVertexBufferView);
	commandList->IASetIndexBuffer(&sphereIndexBufferView);
	commandList->DrawIndexedInstanced(sphereIndexCount, 500, 0, 0, 0);
}



void DX12App::Draw()
{
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), renderSystem->opaquePSO_.Get()));
	sortLODs();
	DrawShadows();
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	renderSystem->g_buffer->TransitToOpaqueRenderingState(commandList);

	renderSystem->g_buffer->ClearGBuffer(commandList);
	renderSystem->post_process->ClearPostProcess(commandList);
	renderSystem->ssao->ClearSSAO(commandList);


	MyTexture* ppWriteTexture = &renderSystem->post_process->GetHdrTextureA();
	MyTexture* ppReadTexture = &renderSystem->post_process->GetHdrTextureB();

	treeIsVisible = false;

	if (isFirstFrame) {
		DrawToStreamOutput();
		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), renderSystem->opaquePSO_.Get()));
		isFirstFrame = false;
	}
	DrawToGBuffer();
	renderSystem->g_buffer->TransitToLightsRenderingState(commandList);
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrier);
	DrawLights();

	if (treeIsVisible) DrawNYBalls();

	bool isEmitterInside = true;
	if (camera.isFrustumCullingEnabled) {
		ContainmentType type = camera.frustum.Contains(emitter.bounds);
		if (type == ContainmentType::DISJOINT) {
			isEmitterInside = false;
		}
	}
	DrawWireframe();
	EmitParticles();
	ComputeParticles();
	if (isEmitterInside) {
		DrawParticles();
	}

	//Здесь будут шейдеры до тонмаппинга

	d3dUtil::SwapTextures(commandList, ppWriteTexture, ppReadTexture);
	DrawPPTonemap(ppReadTexture);
	ppReadTexture = &renderSystem->post_process->GetLdrTextureB();
	ppWriteTexture = &renderSystem->post_process->GetLdrTextureA();
	d3dUtil::SwapTextures(commandList, ppWriteTexture, ppReadTexture);

	DrawPPVignette(ppReadTexture, ppWriteTexture);
	d3dUtil::SwapTextures(commandList, ppWriteTexture, ppReadTexture);
	DrawPPOutput(ppReadTexture);

	CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrierBack);
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(swapChain->Present(0, 0));
	currentBackBuffer = (currentBackBuffer + 1) % 2;

	FlushCommandQueue();
}
