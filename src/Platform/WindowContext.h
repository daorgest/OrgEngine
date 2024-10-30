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

		// Screen Information
		u32 screenWidth  = 0;
		u32 screenHeight = 0;
		u32 windowPosX   = 0;
		u32 windowPosY   = 0;

#ifdef OPENGL_BUILD
		// SDL3-specific fields for OpenGL
		SDL_Window* sdlWindow = nullptr;    // SDL window handle
		SDL_GLContext sdlContext = nullptr; // SDL OpenGL context
		u64 lastTime = 0;                   // Last recorded time in milliseconds (for delta time)
#else
		// Win32-specific fields for Vulkan
		HWND      hwnd{};      // Handle to the window (Vulkan/Win32 specific)
		HINSTANCE hInstance{}; // Handle to the application instance (Win32 specific)
		bool      needsSwapchainRecreation = false;

		// Timer variables for Win32 (for delta time)
		LARGE_INTEGER lastTime{};
		LARGE_INTEGER frequency{};
#endif

		bool isFullscreen = false; // Fullscreen state
		bool isFocused    = false; // Window focus state

		explicit WindowContext(u32 width = 0, u32 height = 0, std::wstring name = L"OrgEngine - Debug")
	: appName(std::move(name)), screenWidth(width), screenHeight(height)
		{
#ifdef OPENGL_BUILD
			if (screenWidth == 0 || screenHeight == 0)
			{
				const SDL_DisplayMode* dm = SDL_GetDesktopDisplayMode(0); // Use display index 0
				if (dm != nullptr)
				{
					screenWidth = dm->w;
					screenHeight = dm->h;
				}
			}
			lastTime = SDL_GetTicks(); // Initialize SDL timer
#else
			if (screenWidth == 0)
			{
				screenWidth = GetSystemMetrics(SM_CXSCREEN);
			}
			if (screenHeight == 0)
			{
				screenHeight = GetSystemMetrics(SM_CYSCREEN);
			}
			windowPosX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
			windowPosY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

			QueryPerformanceFrequency(&frequency); // Initialize high-resolution timer for Win32
			QueryPerformanceCounter(&lastTime);    // Set initial time
#endif
		}



		// Method to update input with current window size
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
					width  = static_cast<u32>(rect.right - rect.left);
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
				SetWindowPos(hwnd, HWND_NOTOPMOST, windowPosX, windowPosY, screenWidth, screenHeight,
				             SWP_FRAMECHANGED | SWP_NOACTIVATE);
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

		double GetDeltaTime()
		{
#ifdef OPENGL_BUILD
			u64 currentTime = SDL_GetTicks();
			double deltaTime = (currentTime - lastTime) / 1000.0; // Convert to seconds
			lastTime = currentTime;
#else
			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);
			double deltaTime = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
			lastTime = currentTime;
#endif
			return deltaTime;
		}

	};
}  // namespace Platform
