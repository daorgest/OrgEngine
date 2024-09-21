//
// Created by Orgest on 8/4/2024.
//

#pragma once

#include <Windows.h>
#include "../Core/Logger.h"
#include "../Core/PrimTypes.h"

// Context (current status)
namespace Platform
{
	struct WindowContext
	{
#ifdef _DEBUG
		const char* appName = "OrgEngine - Debug";
#else
		const char* appName = "OrgEngine - Release";
#endif

#if defined(VULKAN_BUILD) && defined(_WIN32)
		HWND							hwnd{};
		HINSTANCE						hInstance{};
		u32	screenWidth =				GetSystemMetrics(SM_CXSCREEN);
		u32	screenHeight =				GetSystemMetrics(SM_CYSCREEN);
		u32	windowPosX =				(GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		u32	windowPosY =				(GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
		bool isFocused =				false;
		bool isFullscreen =				false;
		bool needsSwapchainRecreation =	false;

		void GetWindowSize(u32& width, u32& height) const
		{
			if (hwnd)
			{
				RECT rect;
				if (GetClientRect(hwnd, &rect))
				{
					width = static_cast<u32>(rect.right - rect.left);
					height = static_cast<u32>(rect.bottom - rect.top);
				}
			}
		}

		void UpdateDimensions() { GetWindowSize(screenWidth, screenHeight); }

		void ToggleFullscreen()
		{
			if (isFullscreen)
			{
				SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
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
		}
#endif
	};
}  // namespace Platform
