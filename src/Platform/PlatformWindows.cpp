//
// Created by Orgest on 4/12/2024.
//

#include "PlatformWindows.h"

namespace Win32
{
    // Definition of the static member variable
    Input WindowManager::input;

    WindowManager::WindowManager()
        : hwnd_(nullptr), hInstance_(GetModuleHandle(nullptr))
    {
        dimensions.screenWidth = GetSystemMetrics(SM_CXSCREEN) / 2;
        dimensions.screenHeight = GetSystemMetrics(SM_CYSCREEN) / 2;
    }

    WindowManager::~WindowManager()
    {
        DestroyAppWindow();
    }

    bool WindowManager::DestroyAppWindow()
    {
        if (hwnd_)
        {
            if (!DestroyWindow(hwnd_))
            {
                return false;
            }
            hwnd_ = nullptr;

            if (!UnregisterClass(appName_, hInstance_))
            {
                return false;
            }
        }
        return true;
    }
    // handling events (like keypresses, rendering)
    LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        LRESULT rez = 0;

        switch (msg)
        {
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
                if (wp < Button::BUTTONS_COUNT)
                {
                    processEventButton(input.keyboard[wp], true); // Key NOT pressed
                }
                break;

            case WM_SYSKEYUP:
            case WM_KEYUP:
                if (wp < Button::BUTTONS_COUNT)
                {
                    processEventButton(input.keyboard[wp], false); // Key NOT pressed
                }
                break;

            case WM_MOUSEMOVE:
                input.cursorX = LOWORD(lp);
                input.cursorY = HIWORD(lp);
                break;

            case WM_LBUTTONDOWN:
                processEventButton(input.lMouseButton, true);
                break;

            case WM_LBUTTONUP:
                processEventButton(input.lMouseButton, false);
                break;

            case WM_RBUTTONDOWN:
                processEventButton(input.rMouseButton, true);
                break;

            case WM_RBUTTONUP:
                processEventButton(input.rMouseButton, false);
                break;

            case WM_SETFOCUS:
                LOG(INFO, "Window in focus");
                input.focused = true;
                break;

            case WM_KILLFOCUS:
                LOG(INFO, "Window out of focus");
                input.focused = false;
                resetInput(input);
                break;

            case WM_CLOSE:
                windowStuff.running = false;
                PostQuitMessage(0);
                break;

            default:
                //there are many messages that we didn't treat so we want to call the default window callback for those...
                rez = DefWindowProc(hwnd, msg, wp, lp);
        }

        return rez;
    }
    // This creates the window class and initiates window creation
    bool WindowManager::RegisterWindowClass() const
    {
        WNDCLASSEX wcex
        {
            .cbSize = sizeof(wcex),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WndProc,
            .hInstance = hInstance_,
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),
            .lpszClassName = appName_,
        };

        if (!RegisterClassEx(&wcex))
        {
            MessageBox(nullptr, L"Cannot register window class.", L"Error", MB_OK | MB_ICONERROR);
            exit(1);
        }
        return true;
    }

    bool WindowManager::CreateAppWindow()
    {
        hwnd_ = CreateWindowEx(
            0,
            appName_,
#ifdef DEBUG
            L"OrgEngine - Debug",
#else
            L"OrgEngine - Release",
#endif
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            dimensions.screenWidth,
            dimensions.screenHeight,
            nullptr,
            nullptr,
            hInstance_,
            nullptr
        );

        if (!hwnd_)
        {
            MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
        UpdateWindow(hwnd_);

        LOG(INFO, "Window created successfully. HWND:", hwnd_);
        return true;
    }

    void WindowManager::LogInputStates() const
    {
        if (input.keyboard[Button::A].pressed)
        {
            LOG(INFO, "A has been pressed");
        }
    }

    void WindowManager::TheMessageLoop() const
    {
        MSG msg = {};
        while (windowStuff.running)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            LogInputStates();

            // GAME LOOP GOES HERE

            if (input.focused)
            {
                resetInput(input);
            }

            processInputAfter(input);
        }
    }
}
