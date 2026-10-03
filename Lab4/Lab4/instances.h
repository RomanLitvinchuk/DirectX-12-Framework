#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;


struct MeshInstanceData {
    Matrix World_;
    Matrix TexTransform_;
    Matrix InvTWorld_;
};

struct WireframeInstanceData {
    Vector3 center;
    Vector3 extents;
    Vector4 color;
};

