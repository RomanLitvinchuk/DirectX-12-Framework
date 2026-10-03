#pragma once

struct Camera;

class Command {
public:
	virtual ~Command() {}
	virtual void Execute(Camera& camera, float speed) = 0;
};

class MoveForwardOrBackwardCommand : public Command {
public:
	void Execute(Camera& camera, float speed) override;
};

class MoveLeftOrRightCommand : public Command {
	void Execute(Camera& camera, float speed) override;
};

