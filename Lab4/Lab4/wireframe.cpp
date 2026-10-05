#include "DX12App.h"

void DX12App::DrawWireframe()
{
	commandList->SetPipelineState(renderSystem->wireframePSO_.Get());
	commandList->SetGraphicsRootSignature(renderSystem->wireframeRS_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	
	commandList->IASetVertexBuffers(0, 1, &wireframeVertexBufferView);
	commandList->IASetIndexBuffer(&wireframeIndexBufferView);

	commandList->SetGraphicsRootConstantBufferView(0, currentFrameResource->objectsUploadBuffer->Resource()->GetGPUVirtualAddress());

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderSystem->post_process->GetHdrTextureA().rtvHandle;
	auto dsv = renderSystem->g_buffer->GetDepthTex().dsvHandle;
	commandList->OMSetRenderTargets(1, &rtv, true, &dsv);

	std::vector<BVHNode*> allNodes;
	octree.GetAllNodes(allNodes);

	if (allNodes.empty()) return;

	UINT numInstances = (std::min)((UINT)allNodes.size(), 1000U);

	for (UINT i = 0; i < numInstances; i++) {
		WireframeInstanceData data;

		data.center = allNodes[i]->bounds.Center;
		data.extents = allNodes[i]->bounds.Extents;
		data.color = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
		currentFrameResource->wireframeInstanceBuffer->CopyData(i, data);
	}

	commandList->SetGraphicsRootShaderResourceView(1, currentFrameResource->wireframeInstanceBuffer->Resource()->GetGPUVirtualAddress());

	commandList->DrawIndexedInstanced(24, numInstances, 0, 0, 0);
}