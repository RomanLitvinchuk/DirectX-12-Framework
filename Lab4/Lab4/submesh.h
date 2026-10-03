#pragma once
#include <DirectXCollision.h>
#include <SimpleMath.h>
#include <string>
#include "instances.h"
using namespace DirectX;
using namespace SimpleMath;

struct Submesh
{
    std::string name_;
    UINT indexCount = 0;
    UINT startIndiceIndex = 0;
    UINT startVerticeIndex = 0;
    int materialIndex = -1;
    DirectX::BoundingBox box;
    UINT InstanceCount = 1;
    Matrix mWorld;
    Matrix mTexTransform = Matrix::Identity;
    std::vector<MeshInstanceData> instances;
    UINT SOVertexCount;
    int globalInstanceOffset;

    UINT indexCountLOD1 = 0;
    UINT startIndiceIndexLOD1 = 0;
    UINT startVerticeIndexLOD1 = 0;
    bool hasLOD1 = false;

    std::vector<MeshInstanceData> sorted_lod0;
    std::vector<MeshInstanceData> sorted_lod1;
    std::vector<MeshInstanceData> sorted_billboards;
    int shadowInstanceOffset = 0;
};

