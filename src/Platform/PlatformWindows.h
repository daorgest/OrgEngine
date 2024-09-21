//
// Created by Orgest on 4/12/2024.
//

#pragma once
#ifdef VULKAN_BUILD
#include "../Core/init.h"
#include "WindowContext.h"

namespace Platform
{
    class Win32
    {
    public:
        explicit Win32(WindowContext* windowContext);
        ~Win32();
        bool Init() const;
        static double GetAbsoluteTime();

        Win32(const Win32&) = delete;
        Win32& operator=(const Win32&) = delete;
        Win32(Win32&&) noexcept = default;
        Win32& operator=(Win32&&) noexcept = default;
    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        const wchar_t* appName_ = L"OrgEngine - Vulkan";
        WindowContext*				   wc_;
    };
}
#endif