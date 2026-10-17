#include "DX12App.h"
#include "history.h"
#include "imgui.h"

#define ALIGN_256(size) ((size + 255) & ~255)

void DX12App::Draw()
{
	frameIndex++;
	WaitForCurrentFrameResource();
	ThrowIfFailed(currentFrameResource->cmdListAlloc->Reset());
	ThrowIfFailed(commandList->Reset(currentFrameResource->cmdListAlloc.Get(), renderSystem->opaquePSO.Get()));
	SortLODs();
	DrawShadows();
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	renderSystem->gBuffer->TransitToOpaqueRenderingState(commandList);

	renderSystem->gBuffer->ClearGBuffer(commandList);
	renderSystem->postProcess->ClearPostProcess(commandList);
	renderSystem->ssao->ClearSSAO(commandList);
	renderSystem->history->ClearLightTexture(commandList);


	MyTexture* ppWriteTexture = &renderSystem->postProcess->GetHdrTextureA();
	MyTexture* ppReadTexture = &renderSystem->postProcess->GetHdrTextureB();

	bIsTreeVisible = false;

	if (bIsFirstFrame) {
		DrawToStreamOutput();
		ThrowIfFailed(commandList->Reset(currentFrameResource->cmdListAlloc.Get(), renderSystem->opaquePSO.Get()));
		bIsFirstFrame = false;
	}
	DrawToGBuffer();
	renderSystem->gBuffer->TransitToLightsRenderingState(commandList);
	DrawLights();

	//if (bIsTreeVisible) DrawNYBalls();

	bool isEmitterInside = true;
	if (camera.bIsFrustumCullingEnabled) {
		ContainmentType type = camera.frustum.Contains(emitter.bounds);
		if (type == ContainmentType::DISJOINT) {
			isEmitterInside = false;
		}
	}
	EmitParticles();
	ComputeParticles();
	if (isEmitterInside) {
		DrawParticles();
	}

	bool skipPresent = !bHistoryValid;

	if (skipPresent)
	{
		InitHistoryTextureFromCurrentFrame(renderSystem->history->GetLightTexture());
		bHistoryValid = true;
		ImGui::EndFrame();
	}
	else
	{
		TAAResolve();
		DrawWireframe();

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);

		//Здесь будут шейдеры до тонмаппинга

		d3dUtil::SwapTextures(commandList, ppWriteTexture, ppReadTexture);
		DrawPPTonemap(ppReadTexture);
		ppReadTexture = &renderSystem->postProcess->GetLdrTextureB();
		ppWriteTexture = &renderSystem->postProcess->GetLdrTextureA();
		d3dUtil::SwapTextures(commandList, ppWriteTexture, ppReadTexture);

		if (renderSystem->bVignette)
		{
			DrawPPVignette(ppReadTexture, ppWriteTexture);
			d3dUtil::SwapTextures(commandList, ppWriteTexture, ppReadTexture);
		}
		DrawPPOutput(ppReadTexture);

		RenderImGui();

		CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrierBack);
	}

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	if (!skipPresent)
	{
		ThrowIfFailed(swapChain->Present(0, 0));
		currentBackBuffer = (currentBackBuffer + 1) % 2;
	}

	currentFence++;
	currentFrameResource->Fence = currentFence;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFence));
}



void DX12App::DrawShadows() {

	UINT totalInstances = 0;
	for (auto& sm : sceneData.submeshes) {
		if (sm.sorted_lod0.empty()) continue;

		for (size_t i = 0; i < sm.sorted_lod0.size(); ++i) {
			currentFrameResource->instanceBuffer->CopyData(totalInstances + i, sm.sorted_lod0[i]);
		}

		sm.shadowInstanceOffset = totalInstances;
		totalInstances += static_cast<UINT>(sm.sorted_lod0.size());
	}

	if (totalInstances == 0) return;

	commandList->SetPipelineState(renderSystem->shadowPSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->shadowRS.Get());
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

	commandList->SetGraphicsRootShaderResourceView(1, currentFrameResource->instanceBuffer->Resource()->GetGPUVirtualAddress());

	const D3D12_GPU_VIRTUAL_ADDRESS shadowCbBaseAddress = currentFrameResource->shadowBuffer->Resource()->GetGPUVirtualAddress();
	const UINT shadowElementSize = ALIGN_256(sizeof(ShadowConstants));
	const D3D12_GPU_VIRTUAL_ADDRESS matBaseAddress = currentFrameResource->materialBuffer->Resource()->GetGPUVirtualAddress();
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
	commandList->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		renderSystem->gBuffer->GetDiffuseTex().rtvHandle, renderSystem->gBuffer->GetNormalTex().rtvHandle
	};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->gBuffer->GetDepthTex().dsvHandle;
	commandList->OMSetRenderTargets(2, rtvs, true, &dsv);

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvHeap.Get(), samplerHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootSignature(renderSystem->opaqueRS.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetVertexBuffers(0, 1, &vertexBuffers[0]);
	commandList->IASetIndexBuffer(&indexBufferView);

	UINT currentInstanceOffset = 0;
	Vector3 cameraPos = camera.mCameraPos;
	for (UINT idx : visibleIndices)
	{
		auto& sm = sceneData.submeshes[idx];

		commandList->SetGraphicsRootSignature(renderSystem->opaqueRS.Get());
		commandList->SetPipelineState(renderSystem->opaquePSO.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->objectsUploadBuffer->Resource()->GetGPUVirtualAddress()); 
		commandList->SetGraphicsRootDescriptorTable(2, samplerHandle); 
		commandList->SetGraphicsRootConstantBufferView(6, currentFrameResource->hullBuffer->Resource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(8, currentFrameResource->viewportBuffer->Resource()->GetGPUVirtualAddress());

		UINT matIndex = sm.materialIndex;
		UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
		D3D12_GPU_VIRTUAL_ADDRESS matAddress = currentFrameResource->materialBuffer->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
		commandList->SetGraphicsRootConstantBufferView(3, matAddress);

		bIsTreeVisible = sceneData.materials[matIndex].isTree == 1;
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

void DX12App::DrawLOD0ToGBuffer(UINT& currentInstanceOffset, Submesh& sm) {
	if (!sm.sorted_lod0.empty()) {
		for (size_t i = 0; i < sm.sorted_lod0.size(); i++) {
			currentFrameResource->instanceBuffer->CopyData(currentInstanceOffset + i, sm.sorted_lod0[i]);
		}
		commandList->SetGraphicsRootShaderResourceView(7, currentFrameResource->instanceBuffer->Resource()->GetGPUVirtualAddress());

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
	commandList->SetPipelineState(renderSystem->bakedPSO.Get());
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
			currentFrameResource->instanceBuffer->CopyData(currentInstanceOffset + i, sm.sorted_lod1[i]);
		}
		commandList->SetGraphicsRootShaderResourceView(7, currentFrameResource->instanceBuffer->Resource()->GetGPUVirtualAddress());

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
		commandList->SetGraphicsRootSignature(renderSystem->billboardRS.Get());
		commandList->SetPipelineState(renderSystem->billboardPSO.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		for (size_t i = 0; i < sm.sorted_billboards.size(); i++) {
			currentFrameResource->instanceBuffer->CopyData(currentInstanceOffset + i, sm.sorted_billboards[i]);
		}
		commandList->SetGraphicsRootShaderResourceView(0, currentFrameResource->instanceBuffer->Resource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(1, currentFrameResource->cameraBuffer->Resource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(2, currentFrameResource->objectsUploadBuffer->Resource()->GetGPUVirtualAddress());
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
	commandList->SetPipelineState(renderSystem->lightPSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->lightRS.Get());

	for (int i = 0; i < renderSystem->sceneLights.size(); ++i) {
		currentFrameResource->lightBuffer->CopyData(i, renderSystem->sceneLights[i]);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->history->GetLightTexture().rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);
	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->gBuffer->GetSrvHeap().Get(), renderSystem->samplerHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->cameraBuffer->Resource()->GetGPUVirtualAddress());
	UINT count = (UINT)renderSystem->sceneLights.size();
	commandList->SetGraphicsRoot32BitConstant(1, count, 0);
	commandList->SetGraphicsRootDescriptorTable(2, renderSystem->gBuffer->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(3, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(4, shadowMap->Srv());
	commandList->SetGraphicsRootConstantBufferView(5, currentFrameResource->shadowBuffer->Resource()->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(6, currentFrameResource->matricesBuffer->Resource()->GetGPUVirtualAddress());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
	CD3DX12_RESOURCE_BARRIER backBarriers[] = { CD3DX12_RESOURCE_BARRIER::Transition(shadowMap->Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ)};
	commandList->ResourceBarrier(_countof(backBarriers), backBarriers);
}

void DX12App::TAAResolve()
{
	renderSystem->history->LightBarrier(commandList);
	commandList->SetPipelineState(renderSystem->TAAResolvePSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->TAAResolveRS.Get());

	auto rtv = renderSystem->postProcess->GetHdrTextureA().rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = {renderSystem->history->GetSrvHeap(), renderSystem->history->GetSamplerHeap()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootDescriptorTable(0, renderSystem->history->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(1, renderSystem->history->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);

	InitHistoryTextureFromCurrentFrame(renderSystem->postProcess->GetHdrTextureA());
}


void DX12App::DrawNYBalls()
{
	commandList->SetPipelineState(renderSystem->bulbPSO.Get());
	commandList->SetGraphicsRootSignature(renderSystem->bulbRS.Get());

	auto dsv = renderSystem->gBuffer->GetDepthTex().dsvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->postProcess->GetHdrTextureA().rtvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, &dsv);

	commandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->objectsUploadBuffer->Resource()->GetGPUVirtualAddress());

	auto handle = renderSystem->gBuffer->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart();
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightSrvHandle(handle, 3, size);
	commandList->SetGraphicsRootDescriptorTable(1, lightSrvHandle);

	commandList->IASetVertexBuffers(0, 1, &sphereVertexBufferView);
	commandList->IASetIndexBuffer(&sphereIndexBufferView);
	commandList->DrawIndexedInstanced(sphereIndexCount, 500, 0, 0, 0);
}

void DX12App::InitHistoryTextureFromCurrentFrame(MyTexture& copyTexture)
{
	auto& historyA = renderSystem->history->GetHistoryA();

	d3dUtil::Barrier(commandList, &copyTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d3dUtil::Barrier(commandList, &historyA, D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyResource(historyA.Resource.Get(), copyTexture.Resource.Get());

	d3dUtil::Barrier(commandList, &copyTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &historyA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}


