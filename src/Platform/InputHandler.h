#pragma once
#include <array>
#include <windows.h>


struct Button {
	unsigned char pressed = 0;    // True in the first frame it is pressed
	unsigned char held = 0;       // True while it is pressed
	unsigned char released = 0;   // True in the frame it is released
	unsigned char altWasDown = 0; // True if alt was down while pressed

	enum Keys {
		A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46, G = 0x47, H = 0x48, I = 0x49,
		J = 0x4A, K = 0x4B, L = 0x4C, M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52,
		S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58, Y = 0x59, Z = 0x5A,
		NR0 = '0', NR1 = '1', NR2 = '2', NR3 = '3', NR4 = '4', NR5 = '5', NR6 = '6', NR7 = '7', NR8 = '8', NR9 = '9',
		Space = VK_SPACE, Enter = VK_RETURN, Escape = VK_ESCAPE, Up = VK_UP, Down = VK_DOWN, Left = VK_LEFT, Right = VK_RIGHT,
		Shift = VK_SHIFT, BackSpace = VK_BACK, Plus_Equal = VK_OEM_PLUS, Period_RightArrow = VK_OEM_PERIOD,
		Minus_Underscore = VK_OEM_MINUS, Comma_LeftArrow = VK_OEM_COMMA, SemiColon = VK_OEM_1, Question_BackSlash = VK_OEM_2,
		Tilde = VK_OEM_3, Quotes = VK_OEM_7, Slash = VK_OEM_5, SquareBracketsOpen = VK_OEM_4, SquareBracketsClose = VK_OEM_6,
		BUTTONS_COUNT
	};

};



struct Input
{

	Button keyboard[Button::BUTTONS_COUNT];

	int cursorX = 0;
	int cursorY = 0;

	Button lMouseButton = {};
	Button rMouseButton = {};
	Button mouse4Button = {};
	Button mouse5Button = {};

	char typedInput[20] = {};

	bool focused = false;
};


void processInputAfter(Input &input);

void resetInput(Input &input);

//newState == 1 means pressed else released
void processEventButton(Button &b, bool newState);