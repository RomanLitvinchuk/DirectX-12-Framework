#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct SimulateParticle {
	Vector3 position;
	float padding1;
	Vector3 velocity;
	float padding2;
	Vector3 predictedPos;
	float padding3;
	float density;
	float lambda;
	uint32_t neighborOffset;
	uint32_t neighborCount;
};

struct CSParticle {
	Vector3 position;
	float density;
	Vector3 velocity;
	float lambda;
	Vector3 predictedPos;
	UINT neighborOffset;
	UINT neighborCount;
};
