//
// Created by Orgest on 4/12/2024.
//

#pragma once

#include "../Core/init.h"
#include "../Core/InputHandler.h"


namespace Win32
{

    struct WindowDimensions
    {
        u32 screenWidth{};
        u32 screenHeight{};
    };

    class WindowManager
    {
    public:
        explicit WindowManager(u32 screenWidth = 0, u32 screenHeight = 0);
        ~WindowManager();
        void Init();
        bool DestroyAppWindow();

        bool CreateAppWindow();
        void LogInputStates() const;
        static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        bool RegisterWindowClass() const;

        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;
        WindowManager(WindowManager&&) noexcept = default;
        WindowManager& operator=(WindowManager&&) noexcept = default;

        [[nodiscard]] HWND getHwnd() const { return hwnd_; }
        [[nodiscard]] HINSTANCE getHInstance() const { return hInstance_; }
        [[nodiscard]] u32 GetWidth() const { return dimensions.screenWidth; }
        [[nodiscard]] u32 GetHeight() const { return dimensions.screenHeight;}
    private:
        HWND hwnd_;
        HINSTANCE hInstance_;
        WindowDimensions dimensions;
        static Input input;  // Static to maintain state across messages
        const wchar_t* appName_ = L"OrgEngine - Vulkan";
    };
}