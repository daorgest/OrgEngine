#include "PlatformWindows.h"

#include <dwmapi.h>

#include "../Core/Timer.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Platform
{

    Win32::Win32(WindowContext* wc) : wc_(wc)
    {
        Init();
    }

    Win32::~Win32()
    {
        DestroyWindow(wc_->hwnd);
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
        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            break;
        case WM_SETFOCUS:
            LOG(INFO, "Window in focus");
            break;
        case WM_KILLFOCUS:
            LOG(INFO, "Window out of focus");
            break;
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        default:
            rez = DefWindowProc(hwnd, msg, wp, lp);
        }

        return rez;
    }

    bool Win32::Init() const
    {
    	Timer timer(__FUNCTION__);
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
#ifdef DEBUG
            L"OrgEngine - Debug",
#else
            L"OrgEngine - Release",
#endif
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

        ShowWindow(wc_->hwnd, SW_SHOW);
        SetForegroundWindow(wc_->hwnd);
        SetFocus(wc_->hwnd);
        UpdateWindow(wc_->hwnd);

        LOG(INFO, "Window created successfully. HWND:", wc_->hwnd);
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
