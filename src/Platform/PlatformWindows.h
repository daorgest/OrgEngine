//
// Created by Orgest on 4/12/2024.
//

#pragma once

#include "../Core/InputHandler.h"
#include "../Core/init.h"
#include "WindowContext.h"


// namespace GraphicsAPI::Vulkan
// {
// 	class VkEngine;
// }
namespace Platform
{
    class Win32
    {
    public:
        explicit Win32(WindowContext* windowContext);
        ~Win32();
        bool Init();
        void MessageLoop();
        static double GetAbsoluteTime();
        bool DestroyAppWindow();

        Win32(const Win32&) = delete;
        Win32& operator=(const Win32&) = delete;
        Win32(Win32&&) noexcept = default;
        Win32& operator=(Win32&&) noexcept = default;
        //
        // static Input input;  // Static to maintain state across messages

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        const wchar_t* appName_ = L"OrgEngine - Vulkan";
        WindowContext*				   windowContext_;
		// GraphicsAPI::Vulkan::VkEngine* gameEngine_{};
    };
}