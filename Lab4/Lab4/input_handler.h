#pragma once
#include <memory>
#include "command.h"

struct Camera;

class InputHandler {
public: 
	InputHandler();
	~InputHandler() = default;


	void handleInput(const bool* keys, Camera& camera, float speed);


private:
	std::unique_ptr<Command> buttonW;
	std::unique_ptr<Command> buttonA;
	std::unique_ptr<Command> buttonS;
	std::unique_ptr<Command> buttonD;
};

