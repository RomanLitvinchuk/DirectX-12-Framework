#include "input_handler.h"

InputHandler::InputHandler() :
	buttonW(std::make_unique<MoveForwardOrBackwardCommand>()),
	buttonA(std::make_unique<MoveLeftOrRightCommand>()),
	buttonS(std::make_unique<MoveForwardOrBackwardCommand>()),
	buttonD(std::make_unique<MoveLeftOrRightCommand>())
{}


void InputHandler::handleInput(const bool* keys, Camera& camera, float speed) {
	if (keys['W']) buttonW->Execute(camera, speed);
	if (keys['A']) buttonA->Execute(camera, -speed);
	if (keys['S']) buttonS->Execute(camera, -speed);
	if (keys['D']) buttonD->Execute(camera, speed);
}