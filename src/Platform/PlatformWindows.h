//
// Created by Orgest on 4/12/2024.
//

#ifndef WINDOWS_H
#define WINDOWS_H

#include "InputHandler.h"
#include "../Core/init.h"


namespace Win32
{


    struct WindowStuff
    {
        volatile bool running = true;

    };

    struct WindowDimensions
    {
        int screenWidth     = 1280;
        int screenHeight    = 720;
    };

    class WindowManager
    {
    public:

        WindowManager();
        ~WindowManager();
        bool DestroyAppWindow();

        bool CreateAppWindow();
        void LogInputStates() const;
        void TheMessageLoop() const;
        static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        bool RegisterWindowClass() const;

        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;
        WindowManager(WindowManager&&) noexcept = default;
        WindowManager& operator=(WindowManager&&) noexcept = default;

        [[nodiscard]] HWND getHwnd() const { return hwnd_; }
        [[nodiscard]] HINSTANCE getHInstance() const { return hInstance_; }
        [[nodiscard]] int GetWidth() const { return dimensions.screenWidth; }
        [[nodiscard]] int GetHeight() const { return dimensions.screenHeight;}
    private:
        HWND hwnd_;
        HINSTANCE hInstance_;
        static Input input;  // Static to maintain state across messages
        const wchar_t* appName_ = L"OrgEngine - Vulkan";
        static inline WindowStuff windowStuff;
        static inline WindowDimensions dimensions;
    };
}


#endif //WINDOWS_H
