//
// Created by Orgest on 4/12/2024.
//

#include "PlatformWindows.h"

// #include "../Renderer/Vulkan/VulkanMain.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Platform
{

    Win32::Win32(WindowContext* windowContext) : windowContext_(windowContext) {}

    Win32::~Win32()
    {
    	DestroyWindow(windowContext_->hwnd);
    }

    LRESULT CALLBACK Win32::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
            return true;

        LRESULT rez = 0;

        switch (msg)
        {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        	// if (wp < static_cast<WPARAM>(Keyboard::Key::BUTTONS_COUNT))
        	// {
        	// 	// input.keyboard.processButtonEvent(static_cast<Keyboard::Key>(wp), true); // Key pressed
        	// }
         //    break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
        	// if (wp < static_cast<WPARAM>(Keyboard::Key::BUTTONS_COUNT))
        	// {
        	// 	// input.keyboard.processButtonEvent(static_cast<Keyboard::Key>(wp), false); // Key pressed
        	// }
            break;
        case WM_MOUSEMOVE:
            // input.cursorX = GET_X_LPARAM(lp);
            // input.cursorY = GET_Y_LPARAM(lp);
            break;
        case WM_LBUTTONDOWN:
        	// input.processKeyboardEvent(input.lMouseButton, true);
        	break;
        case WM_LBUTTONUP:
        	// input.processKeyboardEvent(input.lMouseButton, false);
        	break;
        case WM_RBUTTONDOWN:
        	// input.processKeyboardEvent(input.rMouseButton, true);
        	break;
        case WM_RBUTTONUP:
        	// input.processKeyboardEvent(input.rMouseButton, false);
        	break;
        case WM_SETFOCUS:
            LOG(INFO, "Window in focus");
            // input.focused = true;
            break;
        case WM_KILLFOCUS:
            LOG(INFO, "Window out of focus");
         //    input.focused = false;
        	// input.reset();
            break;
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        default:
            //there are many messages that we didn't treat so we want to call the default window callback for those...
            rez = DefWindowProc(hwnd, msg, wp, lp);
        }

        return rez;
    }

    bool Win32::Init()
    {

    	// // Ensure DPI Awareness for high-DPI displays
    	// SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    	const WNDCLASSEX wcex
		{
			.cbSize = sizeof(wcex),
			.style = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = WndProc,
			.hInstance = windowContext_->hInstance,
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)), // YEAH A BLACK BG
			.lpszClassName = appName_,
		};

    	if (RegisterClassEx(&wcex) == 0)
    	{
    		MessageBox(nullptr, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
    		return false;
    	}

        windowContext_->hwnd = CreateWindowEx(
            0,
            appName_,
#ifdef DEBUG
            L"OrgEngine - Debug",			 // Window text (Debug version)
#else
            L"OrgEngine - Release",          // Window text (Release version)
#endif
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            windowContext_->windowPosX,
            windowContext_->windowPosY,
            static_cast<i32>(windowContext_->screenWidth),
            static_cast<i32>(windowContext_->screenHeight),
            nullptr,
            nullptr,
            windowContext_->hInstance,
            nullptr
        );

        if (!windowContext_->hwnd)
        {
            MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Now to show the window
        ShowWindow(windowContext_->hwnd, SW_SHOW);
        SetForegroundWindow(windowContext_->hwnd);
        SetFocus(windowContext_->hwnd);
        UpdateWindow(windowContext_->hwnd);

        LOG(INFO, "Window created successfully. HWND:", windowContext_->hwnd);
        return true;
    }

    double Win32::GetAbsoluteTime()
    {
        static LARGE_INTEGER frequency;
        static BOOL useHighResTimer = QueryPerformanceFrequency(&frequency);

        if (useHighResTimer)
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            return static_cast<double>(now.QuadPart) / frequency.QuadPart;
        }
        else
        {
            return static_cast<double>(GetTickCount64()) / 1000.0;
        }
    }
}
