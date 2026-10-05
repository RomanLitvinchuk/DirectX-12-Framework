#include "DX12App.h"

struct BulbVertex {
    DirectX::XMFLOAT3 Pos;
};

void CreateSphereGeometry(float radius, uint32_t sliceCount, uint32_t stackCount,
    std::vector<BulbVertex>& vertices, std::vector<uint16_t>& indices) {
    vertices.clear();
    indices.clear();

    BulbVertex topVertex = { {0.0f, +radius, 0.0f} };
    BulbVertex bottomVertex = { {0.0f, -radius, 0.0f} };

    vertices.push_back(topVertex);
    float phiStep = DirectX::XM_PI / stackCount;
    float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;

    for (uint32_t i = 1; i <= stackCount - 1; ++i) {
        float phi = i * phiStep;
        for (uint32_t j = 0; j <= sliceCount; ++j) {
            float theta = j * thetaStep;
            BulbVertex v;
            v.Pos.x = radius * sinf(phi) * cosf(theta);
            v.Pos.y = radius * cosf(phi);
            v.Pos.z = radius * sinf(phi) * sinf(theta);
            vertices.push_back(v);
        }
    }
    vertices.push_back(bottomVertex);

    for (uint32_t i = 1; i <= sliceCount; ++i) {
        indices.push_back(0); indices.push_back(i + 1); indices.push_back(i);
    }
    uint32_t baseIndex = 1;
    uint32_t ringVertexCount = sliceCount + 1;
    for (uint32_t i = 0; i < stackCount - 2; ++i) {
        for (uint32_t j = 0; j < sliceCount; ++j) {
            indices.push_back(baseIndex + i * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }
    uint32_t southPoleIndex = (uint32_t)vertices.size() - 1;
    baseIndex = southPoleIndex - ringVertexCount;
    for (uint32_t i = 0; i < sliceCount; ++i) {
        indices.push_back(southPoleIndex); indices.push_back(baseIndex + i); indices.push_back(baseIndex + i + 1);
    }
}

void DX12App::BuildBulbGeometry() {
    std::vector<BulbVertex> vertices;
    std::vector<uint16_t> indices;

    ThrowIfFailed(currentFrameResource->cmdListAlloc->Reset());
    ThrowIfFailed(commandList->Reset(currentFrameResource->cmdListAlloc.Get(), nullptr));

    CreateSphereGeometry(10.0f, 16, 16, vertices, indices);
    sphereIndexCount = (UINT)indices.size();

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(BulbVertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

    sphereVertexBuffer = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(),
        vertices.data(), vbByteSize, vertexBufferUploader);
    sphereIndexBuffer = d3dUtil::CreateDefaultBuffer(device.Get(), commandList.Get(),
        indices.data(), ibByteSize, indexBufferUploader);
    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* cmdsLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    sphereVertexBufferView.BufferLocation = sphereVertexBuffer->GetGPUVirtualAddress();
    sphereVertexBufferView.StrideInBytes = sizeof(BulbVertex);
    sphereVertexBufferView.SizeInBytes = vbByteSize;

    sphereIndexBufferView.BufferLocation = sphereIndexBuffer->GetGPUVirtualAddress();
    sphereIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    sphereIndexBufferView.SizeInBytes = ibByteSize;
}