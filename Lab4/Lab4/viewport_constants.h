#pragma once
#include <SimpleMath.h>

using DirectX::SimpleMath::Vector2;

struct ViewportConstants
{
	float screenWidth;
	float screenHeight;
	Vector2 jitter;
	Vector2 prevJitter;
};