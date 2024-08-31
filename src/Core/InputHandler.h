#pragma once
#include <windows.h>
#include <Xinput.h>
#include "Array.h"

#pragma comment(lib, "Xinput.lib")

// Button struct for controller input
struct ControllerButton
{
    bool pressed = false;   // True in the first frame it is pressed
    bool held = false;      // True while it is pressed
    bool released = false;  // True in the first frame it is released

    void Update(bool isPressed);
    void Reset();

    enum XboxButton
    {
        DpadUp =        XINPUT_GAMEPAD_DPAD_UP,
        DpadDown =      XINPUT_GAMEPAD_DPAD_DOWN,
        DpadLeft =      XINPUT_GAMEPAD_DPAD_LEFT,
        DpadRight =     XINPUT_GAMEPAD_DPAD_RIGHT,
        Start =         XINPUT_GAMEPAD_START,
        Select =        XINPUT_GAMEPAD_BACK,
        LeftThumb =     XINPUT_GAMEPAD_LEFT_THUMB,
        RightThumb =    XINPUT_GAMEPAD_RIGHT_THUMB,
        LeftShoulder =  XINPUT_GAMEPAD_LEFT_SHOULDER,
        RightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER,
        A =             XINPUT_GAMEPAD_A,
        B =             XINPUT_GAMEPAD_B,
        X =             XINPUT_GAMEPAD_X,
        Y =             XINPUT_GAMEPAD_Y,
        BUTTONS_COUNT = 14
    };
};

// Button struct for keyboard input
struct KeyboardButton
{
    bool pressed = false;   // True in the first frame it is pressed
    bool held = false;      // True while it is pressed
    bool released = false;  // True in the first frame it is released

    void Update(bool isPressed);
    void Reset();

    enum Keys
    {
        A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46, G = 0x47, H = 0x48, I = 0x49,
        J = 0x4A, K = 0x4B, L = 0x4C, M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52,
        S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58, Y = 0x59, Z = 0x5A,
        NR0 = '0', NR1 = '1', NR2 = '2', NR3 = '3', NR4 = '4', NR5 = '5', NR6 = '6', NR7 = '7', NR8 = '8', NR9 = '9',
        Space = VK_SPACE, Enter = VK_RETURN, Escape = VK_ESCAPE, Up = VK_UP, Down = VK_DOWN, Left = VK_LEFT, Right = VK_RIGHT,
        Shift = VK_SHIFT, BackSpace = VK_BACK, Plus_Equal = VK_OEM_PLUS, Period_RightArrow = VK_OEM_PERIOD,
        Minus_Underscore = VK_OEM_MINUS, Comma_LeftArrow = VK_OEM_COMMA, SemiColon = VK_OEM_1, Question_BackSlash = VK_OEM_2,
        Tilde = VK_OEM_3, Quotes = VK_OEM_7, Slash = VK_OEM_5, SquareBracketsOpen = VK_OEM_4, SquareBracketsClose = VK_OEM_6,
        BUTTONS_COUNT = 56
    };
};

// Controller struct to manage Xbox controller input
struct Controller
{
    struct State
    {
        ControllerButton buttons[ControllerButton::XboxButton::BUTTONS_COUNT];
        u8 leftTrigger = 0;
        u8 rightTrigger = 0;
        i16 leftStickX = 0;
        i16 leftStickY = 0;
        i16 rightStickX = 0;
        i16 rightStickY = 0;
    };

    u32 gamepadIndex = 0;
    bool isConnected = false;
    State currentState{};

    void Update();
    void PollInput();
};

// Input struct to manage keyboard, mouse, and controller input
struct Input
{
    SafeArray<KeyboardButton, KeyboardButton::Keys::BUTTONS_COUNT> keyboard{};
    SafeArray<Controller, 4> controllers{}; // assuming 4 controllers as max

    int cursorX = 0;
    int cursorY = 0;

    KeyboardButton lMouseButton = {};
    KeyboardButton rMouseButton = {};
    KeyboardButton mouse4Button = {};
    KeyboardButton mouse5Button = {};

    SafeArray<char, 20> typedInput;

    bool focused = false;
    bool usingController = false;

    void updateUsingController();
    bool isControllerButtonPressed(ControllerButton::XboxButton button, u32 controllerIndex = 0) const;
    bool isKeyPressed(KeyboardButton::Keys key) const;
    static bool isMouseButtonPressed(const KeyboardButton& button) ;

    // New method to check for input with fallback
    bool isInputActive(KeyboardButton::Keys key, ControllerButton::XboxButton button, u32 controllerIndex = 0) const;
};

void processInputAfter(Input &input);
void resetInput(Input &input);
void processEventButton(KeyboardButton &b, bool newState);
