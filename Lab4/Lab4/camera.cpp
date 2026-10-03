#include "camera.h"
#include "game_timer.h"
#include <algorithm>
#include "input_handler.h"

void Camera::MoveBy(const Vector3& delta) {
	mCameraPos += delta;
}

void Camera::UpdateCameraTarget(WPARAM btnState, int dx, int dy) {
	if ((btnState & MK_LBUTTON) != 0)
	{
		mCameraYaw += static_cast<float>(dx) * mCameraRotationSpeed * 0.01f * -1.0f;
		mCameraPitch += static_cast<float>(dy) * mCameraRotationSpeed * 0.01f * -1.0f;

		const float limit = XM_PIDIV2 - 0.01f;
		mCameraPitch = std::clamp(mCameraPitch, -limit, limit);

		mCameraTarget.x = cos(mCameraPitch) * sin(mCameraYaw);
		mCameraTarget.y = sin(mCameraPitch);
		mCameraTarget.z = cos(mCameraPitch) * cos(mCameraYaw);
		mCameraTarget.Normalize();

	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		mRadius_ += 0.005f * static_cast<float>(dx - dy);

		mRadius_ = mRadius_ < 3.0f ? 3.0f : (mRadius_ > 15.0f ? 15.0f : mRadius_);
	}
}

void Camera::UpdateFrustumCullingState() {
	bIsFrustumCullingEnabled = !bIsFrustumCullingEnabled;
}

void Camera::UpdateViewMatrix()
{
	mView_ = Matrix::CreateLookAt(mCameraPos, mCameraPos + mCameraTarget, mCameraUp);
}
