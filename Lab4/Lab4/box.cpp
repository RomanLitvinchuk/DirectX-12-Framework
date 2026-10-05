#include "DX12App.h"

struct BoxVertex {
	Vector3 pos;
};

void DX12App::BuildBoxGeometry() {
    BoxVertex vertices[] = {
    { XMFLOAT3(-0.5f, -0.5f, -0.5f) }, 
    { XMFLOAT3(-0.5f,  0.5f, -0.5f) }, 
    { XMFLOAT3(0.5f,  0.5f, -0.5f) },
    { XMFLOAT3(0.5f, -0.5f, -0.5f) }, 
    { XMFLOAT3(-0.5f, -0.5f,  0.5f) }, 
    { XMFLOAT3(-0.5f,  0.5f,  0.5f) }, 
    { XMFLOAT3(0.5f,  0.5f,  0.5f) }, 
    { XMFLOAT3(0.5f, -0.5f,  0.5f) }  
    };

    uint16_t indices[] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };

    ThrowIfFailed(currentFrameResource->cmdListAlloc->Reset());
    ThrowIfFailed(commandList->Reset(currentFrameResource->cmdListAlloc.Get(), nullptr));

    const UINT vbByteSize = sizeof(vertices);
    const UINT ibByteSize = sizeof(indices);

    wireframeVertexBuffer = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(), vertices, vbByteSize, vertexBufferUploader);
    wireframeIndexBuffer = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(), indices, ibByteSize, vertexBufferUploader);

    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* cmdLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    FlushCommandQueue();

    wireframeVertexBufferView.BufferLocation = wireframeVertexBuffer->GetGPUVirtualAddress();
    wireframeVertexBufferView.StrideInBytes = sizeof(BoxVertex);
    wireframeVertexBufferView.SizeInBytes = vbByteSize;

    wireframeIndexBufferView.BufferLocation = wireframeIndexBuffer->GetGPUVirtualAddress();
    wireframeIndexBufferView.SizeInBytes = ibByteSize;
    wireframeIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
}