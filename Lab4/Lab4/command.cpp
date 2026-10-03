#include "command.h"
#include "camera.h"

void MoveForwardOrBackwardCommand::Execute(Camera& camera, float speed) {
	camera.MoveBy(camera.mCameraTarget * speed);
}

void MoveLeftOrRightCommand::Execute(Camera& camera, float speed) {
	camera.MoveBy(camera.mCameraTarget.Cross(camera.mCameraUp) * speed);
}
