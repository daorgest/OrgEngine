//
// Created by Orgest on 8/22/2024.
//

#include "PlatformSDL.h"

using namespace Platform;



SDL::SDL(WindowContext* windowContext) : windowContext_(windowContext) {}

SDL::~SDL()
{
	Cleanup();
}

void SDL::Init() const
{
	if(!SDL_Init(SDL_INIT_VIDEO))
	{
		LOG(ERR, "SDL Init Failed :(");
		exit(1);
	}
	windowContext_->window = SDL_CreateWindow(
		appName, windowContext_->screenWidth, windowContext_->screenHeight,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

	if (!windowContext_->window)
	{
		LOG(ERR, "Failed to create SDL window: ", SDL_GetError());
		exit(1);
	}

	LOG(INFO, "SDL window created successfully.");
	exit(1);
}


void SDL::Cleanup() const
{
	if (windowContext_->window)
	{
		SDL_DestroyWindow(windowContext_->window);
		windowContext_->window = nullptr;
	}
	SDL_Quit();
}
