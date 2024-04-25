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
        Input input;
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

        void CreateAppWindow();
        static void InputHandler();
        void TheMessageLoop() const;
        static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;
        WindowManager(WindowManager&&) noexcept = default;
        WindowManager& operator=(WindowManager&&) noexcept = default;

        [[nodiscard]] HWND getHwnd() const { return hwnd_; }
        [[nodiscard]] HINSTANCE getHInstance() const { return hInstance_; }


    private:
        HWND hwnd_;
        HINSTANCE hInstance_;
        const wchar_t* appName_ = L"OrgEngine - Vulkan";
        static inline WindowStuff windowStuff;
        static inline WindowDimensions dimensions;
    };
}


#endif //WINDOWS_H
