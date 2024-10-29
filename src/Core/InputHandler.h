#pragma once
#include <array>
#include <windows.h>
#include <xinput.h>

#include "Array.h"

struct Mouse
{
	enum MouseButton
	{
		Mouse_Left = 0,
		Mouse_Right,
		Mouse_Button4,
		Mouse_Button5,
		MOUSE_BUTTONS_COUNT
	};
};

// Enum for Xbox buttons
struct Xbox
{
	enum XboxButton
	{
		Xbox_DpadUp,
		Xbox_DpadDown,
		Xbox_DpadLeft,
		Xbox_DpadRight,
		Xbox_Start,
		Xbox_Select,
		Xbox_LeftThumb,
		Xbox_RightThumb,
		Xbox_LeftShoulder,
		Xbox_RightShoulder,
		Xbox_A,
		Xbox_B,
		Xbox_X,
		Xbox_Y,
		XBOX_BUTTONS_COUNT
	};
};

// Enum for keyboard keys
struct Keyboard
{
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
		KEYS_COUNT
	};
};

struct ButtonState
{
	unsigned char pressed : 1;
	unsigned char held : 1;
	unsigned char released : 1;
	unsigned char unused : 5;
};

// Struct to manage overall input (keyboard, Xbox, mouse)
struct Input
{
	SafeArray<ButtonState, Keyboard::KEYS_COUNT> keyboard;  // Keyboard buttons
	SafeArray<ButtonState, Mouse::MOUSE_BUTTONS_COUNT> mouseButtons; // Mouse buttons
	SafeArray<ButtonState, Xbox::XBOX_BUTTONS_COUNT> xboxButtons;   // Xbox buttons


    f32 cursorX = 0;
    f32 cursorY = 0;
	f32 lastX = 0;
	f32 lastY = 0;
	i16 scrollDelta = 0;

	f32 thumbLeftX{};
	f32 thumbLeftY{};
	f32 thumbRightX{};
	f32 thumbRightY{};
	f32 leftTrigger{};
	f32 rightTrigger{};

	bool  focused         = false; // True if the application window is focused
	bool  usingController = false; // True if the user is using a controller
};

// Process input after handling events
void processInputAfter(Input &input);

// Reset all input states to their default
void resetInput(Input &input);

// newState == 1 means pressed, else released
void processEventButton(ButtonState &b, bool newState);

inline Input input;