#ifndef COMMAND_H_
#define COMMAND_H_
#include "camera.h"

class Command {
public:
	virtual ~Command() {}
	virtual void Execute(Camera& camera, float speed) = 0;
};

class MoveForwardOrBackwardCommand : public Command {
public:
	virtual void Execute(Camera& camera, float speed) {
		camera.MoveBy(camera.mCameraTarget * speed);
	}
};

class MoveLeftOrRightCommand : public Command {
	virtual void Execute(Camera& camera, float speed) {
		camera.MoveBy(camera.mCameraTarget.Cross(camera.mCameraUp) * speed);
	}
};


#endif //COMMAND_H_
