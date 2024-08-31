#include "InputHandler.h"

#include <algorithm>

// ControllerButton Implementation

void ControllerButton::Update(bool isPressed)
{
    if (isPressed)
    {
        if (!held)
        {
            pressed = true;
        }
        held = true;
        released = false;
    }
    else
    {
        if (held)
        {
            released = true;
        }
        pressed = false;
        held = false;
    }
}

void ControllerButton::Reset()
{
    pressed = false;
    released = false;
}

// KeyboardButton Implementation (identical to ControllerButton for simplicity)

void KeyboardButton::Update(bool isPressed)
{
    if (isPressed)
    {
        if (!held)
        {
            pressed = true;
        }
        held = true;
        released = false;
    }
    else
    {
        if (held)
        {
            released = true;
        }
        pressed = false;
        held = false;
    }
}

void KeyboardButton::Reset()
{
    pressed = false;
    released = false;
}

// Controller Implementation

void Controller::Update()
{
    PollInput();

    // Reset button states for the next frame after processing
    for (auto& button : currentState.buttons)
    {
        button.Reset();
    }
}

void Controller::PollInput()
{
    XINPUT_STATE state{};
    DWORD result = XInputGetState(gamepadIndex, &state);

    if (result == ERROR_SUCCESS)
    {
        isConnected = true;

        // Update each button's state
        for (int i = 0; i < ControllerButton::XboxButton::BUTTONS_COUNT; ++i)
        {
            bool isPressed = state.Gamepad.wButtons & (1 << i);
            currentState.buttons[i].Update(isPressed);
        }

        currentState.leftStickX = state.Gamepad.sThumbLX;
        currentState.leftStickY = state.Gamepad.sThumbLY;
        currentState.rightStickX = state.Gamepad.sThumbRX;
        currentState.rightStickY = state.Gamepad.sThumbRY;
        currentState.leftTrigger = state.Gamepad.bLeftTrigger;
        currentState.rightTrigger = state.Gamepad.bRightTrigger;
    }
    else
    {
        isConnected = false;
    }
}

// Input Implementation

void Input::updateUsingController()
{
    usingController = std::ranges::any_of(controllers, [](const Controller& c)
    {
        return c.isConnected;
    });
}

bool Input::isControllerButtonPressed(ControllerButton::XboxButton button, u32 controllerIndex) const
{
    return controllers[controllerIndex].currentState.buttons[button].pressed;
}

bool Input::isKeyPressed(KeyboardButton::Keys key) const
{
    return keyboard[key].pressed;
}

bool Input::isMouseButtonPressed(const KeyboardButton& button)
{
    return button.pressed;
}

// New method to check for input with fallback
bool Input::isInputActive(KeyboardButton::Keys key, ControllerButton::XboxButton button, u32 controllerIndex) const
{
    if (usingController && controllers[controllerIndex].isConnected)
    {
        return isControllerButtonPressed(button, controllerIndex);
    }
    else
    {
        return isKeyPressed(key);
    }
}

void processInputAfter(Input& input)
{
    // Reset keyboard state
    for (auto& key : input.keyboard)
    {
        key.Reset();
    }

    // Update and reset controller state
    for (auto& controller : input.controllers)
    {
        if (controller.isConnected)
        {
            controller.Update();
        }
    }

    // Reset mouse button states
    input.lMouseButton.Reset();
    input.rMouseButton.Reset();
    input.mouse4Button.Reset();
    input.mouse5Button.Reset();

    // Clear typed input
    input.typedInput.fill(0);
}

void resetInput(Input& input)
{
    input.lMouseButton = {};
    input.rMouseButton = {};

    input.keyboard.fill(KeyboardButton{});
    input.typedInput.fill(0);
}

// Process button events: newState == 1 means pressed, else released
void processEventButton(KeyboardButton& b, bool newState)
{
    b.Update(newState == 1);
}
