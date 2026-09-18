#ifndef CAMERA_H_
#define CAMERA_H_

#include <Windows.h>
#include <SimpleMath.h>
#include <DirectXCollision.h>
#include "game_timer.h"


using namespace DirectX;
using namespace DirectX::SimpleMath;

struct HullBuffer {
	DirectX::SimpleMath::Vector3 CameraPos;
	float gMinTess;

	float gMaxTess;
	float gMinDist;
	float gMaxDist;
	float padding;
};

struct Camera {

	void MoveBy(const Vector3& delta);
	void UpdateCameraTarget(WPARAM btnState, int dx, int dy);
	void UpdateFrustumCullingState();
	void UpdateViewMatrix();

	float mCameraYaw;
	float mCameraPitch;
	Vector3 mCameraPos = { 15.0f, 20.0f, -90.0f };
	Vector3 mCameraTarget = { 0.0f, 0.0f, 1.0f };
	Vector3 mCameraUp = { 0.0f, 1.0f, 0.0f };
	float mCameraSpeed = 200.0f;
	float mCameraRotationSpeed = 0.3f;
	float mRadius_ = 5.0f;

	Matrix mWorld_ = Matrix::Identity;
	Matrix mView_ = Matrix::Identity;
	Matrix mProj_ = Matrix::Identity;
	XMMATRIX xmProj = XMMatrixIdentity();

	BoundingFrustum frustum;
	bool isFrustumCullingEnabled = true;
};

#endif //CAMERA_H_