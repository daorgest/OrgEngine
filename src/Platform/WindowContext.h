// WindowContext.h

#pragma once

#include "../Core/Logger.h"
#include "../Core/PrimTypes.h"

#ifdef USE_SDL3
    #include <SDL3/SDL.h>
#else
    #include <Windows.h>
#endif

namespace Platform
{
    struct WindowContext
    {

        u32 screenWidth = 1280;
        u32 screenHeight = 720;
        bool isFullscreen{};


        WindowContext(const u32 screenWidth, const u32 screenHeight, const bool fullscreen = false):
            screenWidth(screenWidth),
            screenHeight(screenHeight), isFullscreen(fullscreen) {}

        #ifdef USE_SDL3
            SDL_Window* window = nullptr;
            SDL_GLContext glContext = nullptr;
        #else
            HWND hwnd{};
            HINSTANCE hInstance{};
        #endif

        void GetWindowSize(u32& width, u32& height) const
        {
            #ifdef USE_SDL3
                if (window)
                {
                    int w, h;
                    SDL_GetWindowSize(window, &w, &h);
                    width = static_cast<u32>(w);
                    height = static_cast<u32>(h);
                }
            #else
                if (hwnd)
                {
                    RECT rect;
                    if (GetClientRect(hwnd, &rect))
                    {
                        width = static_cast<u32>(rect.right - rect.left);
                        height = static_cast<u32>(rect.bottom - rect.top);
                    }
                }
            #endif
        }

        void UpdateDimensions()
        {
            GetWindowSize(screenWidth, screenHeight);
        }

        void ToggleFullscreen()
        {
            isFullscreen = !isFullscreen;

            #ifdef USE_SDL3
                if (isFullscreen)
                {
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                }
                else
                {
                    SDL_SetWindowFullscreen(window, 0);
                }
            #else
                if (isFullscreen)
                {
                    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED | SWP_NOACTIVATE);
                    ShowWindow(hwnd, SW_MAXIMIZE);
                }
                else
                {
                    SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE);
                    ShowWindow(hwnd, SW_NORMAL);
                }
            #endif

            UpdateDimensions();
            LOG(INFO, "Toggled fullscreen mode. New dimensions: ", screenWidth, "x", screenHeight);
        }
    };
}
