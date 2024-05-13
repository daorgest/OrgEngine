//
// Created by Orgest on 4/23/2024.
//
#include "InputHandler.h"
#include "../Core/Logger.h"


// TODO: INTEGRATEING THIS
void Input::updateUsingController() {
	// Check if any Xbox controller button is pressed or held
	usingController = false;
	for (const auto& button : xboxButtons) {
		if (button.pressed || button.held) {
			usingController = true;
			break;
		}
	}

	// If no controller input detected, check if any keyboard or mouse button is pressed or held
	if (!usingController) {
		for (const auto& key : keyboard) {
			if (key.pressed || key.held) {
				return;
			}
		}

		if (lMouseButton.pressed || lMouseButton.held ||
			rMouseButton.pressed || rMouseButton.held ||
			mouse4Button.pressed || mouse4Button.held ||
			mouse5Button.pressed || mouse5Button.held) {
			return;
			}

		usingController = false;
	}
}

void processInputAfter(Input &input)
{
	for (int i = 0; i < Button::BUTTONS_COUNT; i++)
	{
		input.keyboard[i].pressed = 0;
		input.keyboard[i].released = 0;
		input.keyboard[i].altWasDown = 0;
	}

	for (auto& button : input.xboxButtons)
	{
		button.pressed = 0;
		button.released = 0;
		button.altWasDown = 0;
	}

	input.lMouseButton.pressed = 0;
	input.lMouseButton.released = 0;
	input.lMouseButton.altWasDown = 0;

	input.rMouseButton.pressed = 0;
	input.rMouseButton.released = 0;
	input.rMouseButton.altWasDown = 0;


	ZeroMemory(input.typedInput, sizeof(input.typedInput));
}

void resetInput(Input &input)
{
	input.lMouseButton = {};
	input.rMouseButton = {};

	ZeroMemory(input.keyboard.data(), input.keyboard.size() * sizeof(Button));
	ZeroMemory(input.xboxButtons.data(), input.xboxButtons.size() * sizeof(Button));
	ZeroMemory(input.typedInput, sizeof(input.typedInput));
}

//newState == 1 means pressed else released
void processEventButton(Button &b, bool newState)
{
	//LOG(INFO, "Processing Event: ", (newState ? "Pressed" : "Released"));
	if (newState)
	{
		if (!b.held)
		{
			//LOG(INFO, "Button state changed to Pressed");
			b.pressed = true;
			b.held = true;
			b.released = false;
		}
	}
	else
	{
		//LOG(INFO, "Button state changed to Held");
		b.pressed = false;
		b.held = false;
		b.released = true;
	}
}
