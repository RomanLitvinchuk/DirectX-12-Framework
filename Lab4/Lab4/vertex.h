#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Vertex
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;
    Vector3 tangent;
    Vector3 biNormal;
};

struct BakedVertex
{
    Vector3 worldPos;
    Vector3 normal;
    Vector3 normalW;
    Vector3 uv;
    Vector3 tangentW;
};
