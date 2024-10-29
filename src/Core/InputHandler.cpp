//
// Created by Orgest on 4/23/2024.
//
#include "../Core/InputHandler.h"

// TODO: INTEGRATEING THIS

// void UpdateControllerInput(Input &input)
// {
// 	XINPUT_STATE state{};
//
// 	if (XInputGetState(0, &state) == ERROR_SUCCESS)
// 	{
// 		XINPUT_GAMEPAD& gamepad = state.Gamepad;
//
// 		processEventButton(input.xboxButtons[Xbox::Xbox_A], gamepad.wButtons & XINPUT_GAMEPAD_A);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_B], gamepad.wButtons & XINPUT_GAMEPAD_B);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_X], gamepad.wButtons & XINPUT_GAMEPAD_X);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_Y], gamepad.wButtons & XINPUT_GAMEPAD_Y);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_Start], gamepad.wButtons & XINPUT_GAMEPAD_START);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_Select], gamepad.wButtons & XINPUT_GAMEPAD_BACK);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_LeftShoulder], gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_RightShoulder], gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_DpadUp], gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_DpadDown], gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_DpadLeft], gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
// 		processEventButton(input.xboxButtons[Xbox::Xbox_DpadRight], gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
//
// 		input.thumbLeftX = gamepad.sThumbLX / 32767.0f;  // Normalize to [-1, 1]
// 		input.thumbLeftY = gamepad.sThumbLY / 32767.0f;
// 		input.thumbRightX = gamepad.sThumbRX / 32767.0f;
// 		input.thumbRightY = gamepad.sThumbRY / 32767.0f;
//
// 		// Handle trigger input (e.g., left and right triggers)
// 		input.leftTrigger = gamepad.bLeftTrigger / 255.0f;  // Normalize to [0, 1]
// 		input.rightTrigger = gamepad.bRightTrigger / 255.0f;
// 	}
// 	else
// 	{
// 		// Controller is not connected, reset the Xbox buttons to default
// 		// input.xboxButtons.reset();
// 	}
// }

void processInputAfter(Input &input)
{
	// Reset pressed and released states for all keyboard buttons
	for (auto& key : input.keyboard)
	{
		key.pressed = 0;
		key.released = 0;
	}

	// Reset pressed and released states for all Xbox buttons
	for (auto& button : input.xboxButtons)
	{
		button.pressed = 0;
		button.released = 0;
	}

	// Reset pressed and released states for mouse buttons
	for (auto& mouseButton : input.mouseButtons) {
		mouseButton.pressed = 0;
		mouseButton.released = 0;
	}

}

void resetInput(Input &input)
{
	input.keyboard.reset();
	input.mouseButtons.reset();
	input.xboxButtons.reset();
}

//newState == 1 means pressed else released
void processEventButton(ButtonState &b, const bool newState)
{
	//LOG(INFO, "Processing Event: ", (newState ? "Pressed" : "Released"));
	if (newState)
	{
		// If button was just pressed, set 'pressed' and 'held'
		if (!b.held)
		{
			b.pressed = true;
		}
		b.held = true;
		b.released = false;
	}
	else
	{
		//LOG(INFO, "Button state changed to Held");
		b.pressed = false;
		b.held = false;
		b.released = true;
	}
}