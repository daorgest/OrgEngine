//
// Created by Orgest on 4/12/2024.
//
#pragma once

#include    <iostream>
#include	<memory>
#include	<string>
#include    <vector>

#ifdef _WIN32
#define		VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Win32
{
    class WindowManager;
}

namespace GraphicsAPI
{
    inline struct VulkanData
    {
        VkInstance instance_{};
        VkDebugUtilsMessengerEXT dbgMessenger_{};
        VkPhysicalDevice physicalDevice_{};
        VkDevice device_{};
        VkSurfaceKHR surface_{};
    } vd ; // YEAHHHHHHHHHHH FOR ANYTHING THIS, ACCEESS IT HERE


    class Vulkan
    {
    public:
        void InitVulkan(VulkanData& vd);
        void CreateSurface(HINSTANCE hInstance, HWND hwnd, VulkanData& vd);
        void InitVulkanAPI(const Win32::WindowManager& windowManager);
        void PrintAvailableExtensions() const;
        static void SetupDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    private:

        static constexpr const char* enabledExtensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };
    };
}
