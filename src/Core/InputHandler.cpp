//
// Created by Orgest on 4/23/2024.
//
#include "InputHandler.h"

#include "Logger.h"

// Controller::Controller(u32 index)
// {
// }

void Controller::Update()
{
	PollInput();
}

void Controller::PollInput()
{
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));


	DWORD result = XInputGetState(gamepadIndex, &state) ;
	if (result == ERROR_SUCCESS)
	{
		currentState.Buttons =		state.Gamepad.wButtons;
		currentState.LeftStickX =	state.Gamepad.sThumbLX;
		currentState.LeftStickY =	state.Gamepad.sThumbLY;
		currentState.RightStickX =	state.Gamepad.sThumbRX;
		currentState.RightStickY =	state.Gamepad.sThumbRY;
		currentState.LeftTrigger =	state.Gamepad.bLeftTrigger;
		currentState.RightTrigger = state.Gamepad.bRightTrigger;
	}
	else {
		currentState = {};
	}
}

// u8 Controller::GetTriggerValues(Controller& controller) const
// {
// 	switch (controller)
// 	{
// 		case Trigger::LeftTrigger:	return currentState.LeftTrigger;
// 		case Trigger::RightTrigger: return currentState.RightTrigger;
// 		default:					return 0;
// 	}
// }
//
// i16 Controller::GetStickValues(Controller& controller) const
// {
// 	switch (controller)
// 	{
// 		case Stick::LeftStickX:		return currentState.LeftStickX;
// 		case Stick::LeftStickY:		return currentState.LeftStickY;
// 		case Stick::RightStickX:	return currentState.RightStickX;
// 		case Stick::RightStickY:	return currentState.RightStickY;
// 		default:					return 0;
// 	}
// }
//
// void Controller::ProcessControllerAfter(Controller& controller)
// {
// 	for (const int i : 14)
// 	{
//
// 	}
// }

void processInputAfter(Input& input)
{
	for (int i = 0; i < Button::BUTTONS_COUNT; i++)
	{
		input.keyboard[i].pressed = 0;
		input.keyboard[i].released = 0;
		input.keyboard[i].altWasDown = 0;
	}

	for (int i = 0; i < Controller::XboxButton::BUTTONS_CONUNT; i++)
	{
		input.xboxButtons[i].pressed = 0;
		input.xboxButtons[i].released = 0;
	}

	input.lMouseButton.pressed = 0;
	input.lMouseButton.released = 0;
	input.lMouseButton.altWasDown = 0;

	input.rMouseButton.pressed = 0;
	input.rMouseButton.released = 0;
	input.rMouseButton.altWasDown = 0;


	ZeroMemory(static_cast<void*>(input.typedInput), sizeof(input.typedInput));
}

void resetInput(Input& input)
{
	input.lMouseButton = {};
	input.rMouseButton = {};

	input.keyboard.fill(Button{});
	input.typedInput.fill(0);
}


//newState == 1 means pressed else released
void processEventButton(Button& b, bool newState)
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

