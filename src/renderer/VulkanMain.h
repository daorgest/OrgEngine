//
// Created by Orgest on 4/12/2024.
//
#pragma once

#include    <deque>
#include	<string>
#include    <vector>
#ifdef _WIN32
#define		VK_USE_PLATFORM_WIN32_KHR
#endif
#include <functional>
#include <vulkan/vulkan.h>
#include "../Platform/PlatformWindows.h"

namespace GraphicsAPI
{

    struct DeletionQueue
    {
        std::deque<std::function<void()>> deletors;

        void push_function(std::function<void()>&& function) {
            deletors.push_back(function);
        }

        void flush() {
            // reverse iterate the deletion queue to execute all the functions
            for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
                (*it)(); //call functors
            }

            deletors.clear();
        }
    };

    inline struct VulkanData
    {
        VkInstance instance_{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT dbgMessenger_{VK_NULL_HANDLE};
        VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
        VkDevice device_{VK_NULL_HANDLE};
        VkSurfaceKHR surface_{VK_NULL_HANDLE};
        DeletionQueue deletionQueue;
    } vd;

    class VkEngine
    {
    public:
        explicit VkEngine(Win32::WindowManager &winManager);
        ~VkEngine();

        void InitVulkan();
        static void CreateSurface(HINSTANCE hInstance, HWND hwnd, VulkanData& vd);
        static void SetupDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        static void PrintAvailableExtensions();
        static std::string decodeDriverVersion(uint32_t driverVersion, uint32_t vendorID);
        static void SetupDebugMessenger();
        std::vector<const char*> getRequiredExtensions();
    private:
        Win32::WindowManager &winManager;
        static constexpr const char* enabledExtensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME // REALLLY THINKIN ABOUT USING VK BOOTSTRAP
        };
    };
}
