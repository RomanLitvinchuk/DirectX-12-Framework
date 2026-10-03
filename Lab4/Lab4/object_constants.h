#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;


struct ObjectConstants {
	Matrix View;
	Matrix Proj;
	float gTime;
	float pad[3];
};

struct Matrices {
	Matrix View;
	Matrix Proj;
	Matrix invView;
	Matrix invProj;
};
