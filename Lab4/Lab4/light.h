#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct LightConstants {
	Vector3 lightColor;
	int lightType;

	Vector3 lightPosition;
	float lightRange;

	Vector3 lightDirection;
	float SpotCosInner;

	float SpotCosOuter;
	Vector3 padding;
};

struct CameraConstants {
	Matrix invViewProj;

	Vector3 cameraPos;
	float padding;
};

