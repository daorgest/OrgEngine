#pragma once

#include "../Core/Logger.h"
#include "../Core/PrimTypes.h"

#ifdef OPENGL_BUILD
#include <SDL3/SDL.h> // For SDL3 when using OpenGL
#else
#include <Windows.h>  // For Vulkan (and Win32 in general)
#endif

namespace Platform
{
	struct WindowContext
	{
#ifdef _DEBUG
		std::wstring appName = L"OrgEngine - Debug";
#else
		std::wstring appName = L"OrgEngine - Release";
#endif

		// Common metadata
		u32 screenWidth = 0;
		u32 screenHeight = 0;
		u32 windowPosX = 0;  // Position to be determined based on the system
		u32 windowPosY = 0;  // Position to be determined based on the system

		bool isFullscreen = false;    // Fullscreen state
		bool isFocused = false;       // Window focus state

#ifdef OPENGL_BUILD
		// SDL3-specific fields for OpenGL
		SDL_Window* sdlWindow = nullptr;    // SDL window handle
		SDL_GLContext sdlContext = nullptr; // SDL OpenGL context
#else
		// Win32-specific fields for Vulkan
		HWND hwnd{};             // Handle to the window (Vulkan/Win32 specific)
		HINSTANCE hInstance{};    // Handle to the application instance (Win32 specific)
		bool needsSwapchainRecreation = false;
#endif

		explicit WindowContext(u32 width = 0, u32 height = 0, std::wstring name = L"OrgEngine - Debug")
			: appName(std::move(name)), screenWidth(width), screenHeight(height)
		{
			// Initialize screen dimensions based on the platform
#ifdef OPENGL_BUILD
			// SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
			// const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(displayID);
			//
			// if (displayMode)
			// {
			// 	screenWidth = displayMode->w;
			// 	screenHeight = displayMode->h;
			// }
			// else
			// {
			// 	LOG(ERR, "Failed to get desktop display mode: " + std::string(SDL_GetError()));
			// }

#endif
			windowPosX = (screenWidth - screenWidth) / 2;  // Center by default
			windowPosY = (screenHeight - screenHeight) / 2; // Center by default
		}

		// Method to get the current window size
		void GetWindowSize(u32& width, u32& height) const
		{
#ifdef OPENGL_BUILD
			if (sdlWindow)
			{
				int w, h;
				SDL_GetWindowSize(sdlWindow, &w, &h);
				width = static_cast<u32>(w);
				height = static_cast<u32>(h);
			}
#else
			if (hwnd)
			{
				RECT rect;
				if (GetClientRect(hwnd, &rect))
				{
					width = static_cast<u32>(rect.right - rect.left);
					height = static_cast<u32>(rect.bottom - rect.top);
				}
			}
#endif
		}

		// Method to toggle fullscreen mode
		void ToggleFullscreen()
		{
#ifdef OPENGL_BUILD
			isFullscreen = !isFullscreen;
			if (isFullscreen)
			{
				SDL_SetWindowFullscreen(sdlWindow, true);
			}
			else
			{
				SDL_SetWindowFullscreen(sdlWindow, 0);
			}
#else
			// Win32-specific fullscreen handling for Vulkan
			if (isFullscreen)
			{
				SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowPos(hwnd, HWND_NOTOPMOST, windowPosX, windowPosY, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
				ShowWindow(hwnd, SW_NORMAL);
			}
			else
			{
				SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
							 SWP_FRAMECHANGED | SWP_NOACTIVATE);
				ShowWindow(hwnd, SW_MAXIMIZE);
			}
			isFullscreen = !isFullscreen;
			UpdateDimensions();
			needsSwapchainRecreation = true;
			LOG(INFO, "Toggled fullscreen mode. New dimensions: ", screenWidth, "x", screenHeight);
#endif
		}

		// Method to update the window dimensions
		void UpdateDimensions()
		{
			GetWindowSize(screenWidth, screenHeight);
		}
	};
}  // namespace Platform
