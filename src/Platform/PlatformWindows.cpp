#include "PlatformWindows.h"

#include "../Core/InputHandler.h"
#ifdef VULKAN_BUILD
#include <dwmapi.h>
#include <imgui.h>

#include "../Core/Timer.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Platform
{

    Win32::Win32(WindowContext* windowContext, const std::wstring& platName) : wc_(windowContext)
    {
    	wc_->appName += platName;
    }

    Win32::~Win32()
    {
        DestroyWindow(wc_->hwnd);
    }

    LRESULT CALLBACK Win32::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return true;
        switch (msg)
        {
        	case WM_SETFOCUS:
        		LOG(INFO, "Window in focus");
        		input.focused = true;
        		break;
        	case WM_KILLFOCUS:
        		LOG(INFO, "Window out of focus");
        		input.focused = false;
        		resetInput(input);
        		break;
        	case WM_SYSKEYUP:
			case WM_KEYUP:
				if (wp < Keyboard::KEYS_COUNT)
				{
					processEventButton(input.keyboard[static_cast<Keyboard::Keys>(wp)], false);
				}
        		break;
        	case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				if (wp == VK_ESCAPE) PostQuitMessage(0);
        		if (wp < Keyboard::KEYS_COUNT)
        		{
        			processEventButton(input.keyboard[static_cast<Keyboard::Keys>(wp)], true);
        		}
        		break;
        	case WM_MOUSEMOVE:
        		input.cursorX = LOWORD(lp);
        		input.cursorY = HIWORD(lp);
        		break;
        	case WM_LBUTTONDOWN:
        		processEventButton(input.mouseButtons[Mouse::Mouse_Left], true);
        	break;
        	case WM_RBUTTONDOWN:
        		processEventButton(input.mouseButtons[Mouse::Mouse_Right], true);
        		break;
        	case WM_LBUTTONUP:
        		processEventButton(input.mouseButtons[Mouse::Mouse_Left], false);
        		break;
        	case WM_RBUTTONUP:
        		processEventButton(input.mouseButtons[Mouse::Mouse_Right], false);
        		break;
        	case WM_MOUSEWHEEL:
        		input.scrollDelta = GET_WHEEL_DELTA_WPARAM(wp);
                // Convert to a more manageable delta (e.g., 1 for up, -1 for down)
                input.scrollDelta = (input.scrollDelta > 0) ? 1.0f : -1.0f;
        		break;
        	case WM_QUIT:
        	case WM_DESTROY: PostQuitMessage(0); break;
			default: return DefWindowProc(hwnd, msg, wp, lp);
        }
        return 0;
    }

    bool Win32::Init() const
    {
        const WNDCLASSEX wcex
        {
            sizeof(wcex),
            CS_HREDRAW | CS_VREDRAW,
            WndProc,
            0,
            0,
            wc_->hInstance,
            nullptr,
            LoadCursor(nullptr, IDC_ARROW),
            static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),
            nullptr,
            appName_,
            nullptr,
        };

        if (RegisterClassEx(&wcex) == 0)
        {
            MessageBox(nullptr, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        wc_->hwnd = CreateWindowEx(
            0,
            appName_,
            wc_->appName.c_str(),
            WS_OVERLAPPEDWINDOW,
            wc_->windowPosX,
            wc_->windowPosY,
            static_cast<i32>(wc_->screenWidth),
            static_cast<i32>(wc_->screenHeight),
            nullptr,
            nullptr,
            wc_->hInstance,
            nullptr
        );

    	// Set rounded corners if running on Windows 10 (Version 1809, Build 17763) or later
    	BOOL useRoundedCorners = DWMWCP_ROUND;
    	DwmSetWindowAttribute(wc_->hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &useRoundedCorners, sizeof(useRoundedCorners));


        if (!wc_->hwnd)
        {
            MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }
}
#endif