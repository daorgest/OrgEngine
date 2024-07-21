//
// Created by Orgest on 4/12/2024.
//

#pragma once

#include "../Core/InputHandler.h"
#include "../Core/init.h"


namespace Platform
{

    // Context (current status)
    struct WindowContext
    {
        HWND hwnd{};
        HINSTANCE hInstance{};
        u32 screenWidth{};
        u32 screenHeight{};
        bool isFocused{false};
        bool isFullscreen{false};
        bool needsSwapchainRecreation{false};

        void SetDimensions(u32 width, u32 height)
        {
            screenWidth = width;
            screenHeight = height;
        }

        void UpdateDimensions()
        {
            if (hwnd)
            {
                RECT rect;
                if (GetClientRect(hwnd, &rect))
                {
                    screenWidth = static_cast<u32>(rect.right - rect.left);
                    screenHeight = static_cast<u32>(rect.bottom - rect.top);
                }
            }
        }

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
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED | SWP_NOACTIVATE);
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
            isFullscreen = !isFullscreen;
            UpdateDimensions();
            needsSwapchainRecreation = true;
            LOG(INFO, "Toggled fullscreen mode. New dimensions: ", screenWidth, "x", screenHeight);
        }
    };

    class Win32
    {
    public:
        explicit Win32(WindowContext* windowContext);
        ~Win32();
        bool Init();
        static double GetAbsoluteTime();
        bool DestroyAppWindow();
        void LogInputStates() const;

        Win32(const Win32&) = delete;
        Win32& operator=(const Win32&) = delete;
        Win32(Win32&&) noexcept = default;
        Win32& operator=(Win32&&) noexcept = default;

    private:
        static Input input;  // Static to maintain state across messages
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        [[nodiscard]] bool RegisterWindowClass() const;

        const wchar_t* appName_ = L"OrgEngine - Vulkan";
        WindowContext* windowContext_;
    };
}