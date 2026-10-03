#pragma once
#include <SimpleMath.h>

using namespace DirectX;
using namespace SimpleMath;

struct MaterialConstants
{

    Matrix MatTransform = Matrix::Identity;

    Vector4 DiffuseColor;
    Vector4 AmbientColor;
    Vector4 SpecularColor;
    Vector4 EmissiveColor;
    Vector4 TransparentColor;

    float Shininess;
    float Opacity;
    float RefractionIndex;
    int hasNormalTexture = 0;

    int isLion = 0;
    int isTree = 0;
    int diffuseTextureIndex = 0; 
    int normalTextureIndex = 0;

    int displacementTextureIndex = 0;
    int hasDisplacementTexture = 0;
    int shadowTextureIndex = 0;
    int hasShadowTexture = 0;

    int billboardTextureIndex = 0;
    int hasBillboardTexture = 0;
    int pad[2];
};

//static_assert(sizeof(MaterialConstants) == 160,
//    "MaterialConstants should be 160 bytes (16*5 + 4*3 + 4 + 64)");

