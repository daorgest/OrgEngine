//
// Created by Orgest on 4/12/2024.
//
#pragma once

#include    <deque>
#include	<string>
#include    <vector>
#ifdef _WIN32
#define	VK_USE_PLATFORM_WIN32_KHR
#endif
#include <functional>
#include <vk_mem_alloc.h>
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

    struct FrameData
    {
        VkCommandPool commandPool_;
        VkCommandBuffer mainCommandBuffer_;
    };

    constexpr unsigned int FRAME_OVERLAP = 2;

    struct VulkanData
    {
        VkInstance instance_{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT dbgMessenger_{VK_NULL_HANDLE};
        VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
        VkDevice device_{VK_NULL_HANDLE};
        VkSurfaceKHR surface_{VK_NULL_HANDLE};
        DeletionQueue deletionQueue;
    };

    class VkEngine
    {
    public:

        bool isInit = false;

        explicit VkEngine(Win32::WindowManager &winManager);
        ~VkEngine();
        void Cleanup();

        void InitVulkan();

        // Swapchain management
        void CreateSwapchain(uint32_t width, uint32_t height);
        void InitSwapChain();
        void DestroySwapchain();

    private:
        VulkanData vd;
        Win32::WindowManager &winManager_;

        // Swapchain properties
        VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
        VkFormat swapchainImageFormat_{};
        std::vector<VkImage> swapchainImages_;
        std::vector<VkImageView> swapchainImageViews_;
        VkExtent2D swapchainExtent_{};

        // Allocator for Vulkan memory
        VmaAllocator allocator_{};

        static void PrintAvailableExtensions();
        static std::string decodeDriverVersion(uint32_t driverVersion, uint32_t vendorID);
        static void CreateSurface(HINSTANCE hInstance, HWND hwnd, VulkanData& vd);
    };
}
