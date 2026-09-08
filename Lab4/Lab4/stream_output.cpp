#include "DX12App.h"

void DX12App::CreateSOBuffers() {
	UINT maxTess = 3;
	UINT maxVerticesPerPatch = (maxTess * maxTess) * 3;
	UINT totalMaxVertices = 583680 * maxVerticesPerPatch;
	UINT64 soBufferSize = totalMaxVertices * sizeof(BakedVertex);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto SOBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(soBufferSize, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&streamOutputBuffer)));

	SOBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(8, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&filledSizeBuffer)));

	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuffer)));

	CD3DX12_RESOURCE_BARRIER soBarrier = CD3DX12_RESOURCE_BARRIER::Transition(streamOutputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_STREAM_OUT);
	CD3DX12_RESOURCE_BARRIER fsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_STREAM_OUT);
	D3D12_RESOURCE_BARRIER barriers[] = { soBarrier, fsBarrier };
	commandList->Reset(commandAllocator.Get(), nullptr);
	commandList->ResourceBarrier(2, barriers);
	commandList->Close();
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	streamOutputBufferView.BufferLocation = streamOutputBuffer->GetGPUVirtualAddress();
	streamOutputBufferView.SizeInBytes = soBufferSize;
	streamOutputBufferView.BufferFilledSizeLocation = filledSizeBuffer->GetGPUVirtualAddress();

}

void DX12App::DrawToStreamOutput()
{
	commandList->SetPipelineState(renderSystem->streamOutputPSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->streamOutputRS_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvHeap.Get(), samplerHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	commandList->IASetVertexBuffers(0, 1, &vertexBuffers[0]);
	commandList->IASetIndexBuffer(&indexBufferView);
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(
		cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

	commandList->SetGraphicsRootDescriptorTable(
		2,
		samplerHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->SetGraphicsRootConstantBufferView(6, hullBuffer->Resource()->GetGPUVirtualAddress());

	for (auto& sm : sceneData.submeshes) {
		if (sm.name_.find("Sketchfab") != std::string::npos) {
			streamOutputMesh = sm;
			break;
		}
	}
	UINT matIndex = streamOutputMesh.materialIndex;
	UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
	D3D12_GPU_VIRTUAL_ADDRESS matAddress = materialBuffer->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
	commandList->SetGraphicsRootConstantBufferView(3, matAddress);

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

	for (int i = 0; i < streamOutputMesh.InstanceCount; i++) {
		instanceBuffer->CopyData(i, streamOutputMesh.instances[i]);
	}

	commandList->SetGraphicsRootShaderResourceView(7, instanceBuffer->Resource()->GetGPUVirtualAddress());

	D3D12_STREAM_OUTPUT_BUFFER_VIEW soViews[] = { streamOutputBufferView };
	commandList->SOSetTargets(0, 1, soViews);
	commandList->DrawIndexedInstanced(streamOutputMesh.indexCount, 1, streamOutputMesh.startIndiceIndex, streamOutputMesh.startVerticeIndex, 0);
	commandList->SOSetTargets(0, 1, nullptr);
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->ResourceBarrier(1, &barrier);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		streamOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &barrier);
	commandList->CopyResource(readbackBuffer.Get(), filledSizeBuffer.Get());
	commandList->Close();
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	UINT64* mappedData = nullptr;
	readbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
	UINT64 filledBytes = *mappedData;
	readbackBuffer->Unmap(0, nullptr);

	streamOutputMesh.SOVertexCount = filledBytes / sizeof(BakedVertex);
}