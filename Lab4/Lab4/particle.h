#pragma once
#include <SimpleMath.h>
#include <DirectXCollision.h>

using namespace DirectX::SimpleMath;

struct Particle {
	Vector4 Position;
	Vector3 Velocity;
	float padding;
	Vector4 Color;
	float currentAge = 0;
	float deadAge;
	uint32_t isAlive = 0;
	float pad;
};

struct SortParticle {
	uint32_t index;
	float distance;
};

struct ParticleConstants {
	Vector3 CameraPos;
	float deltaTime;
	int particlesCount;
};

struct Emitter {
	Vector3 Position;
	BoundingBox bounds;
};
