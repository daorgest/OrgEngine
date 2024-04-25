//
// Created by Orgest on 4/23/2024.
//
#include "InputHandler.h"


void processInputAfter(Input &input)
{
	for (int i = 0; i < Button::BUTTONS_COUNT; i++)
	{
		input.keyboard[i].pressed = 0;
		input.keyboard[i].released = 0;
		input.keyboard[i].altWasDown = 0;
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

	ZeroMemory(input.keyboard, sizeof(input.keyboard));
	ZeroMemory(input.typedInput, sizeof(input.typedInput));
}

//newState == 1 means pressed else released
void processEventButton(Button &b, bool newState)
{
	if (newState)
	{
		if (!b.held)
		{
			b.pressed = true;
			b.held = true;
			b.released = false;
		}
	}
	else
	{
		b.pressed = false;
		b.held = false;
		b.released = true;
	}
}
