#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

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
    int hasDisplacementTexture = 0;
    int hasShadowTexture = 0;


    int hasBillboardTexture = 0;
    int pad[3];
};

