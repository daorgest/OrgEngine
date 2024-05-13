//
// Created by Orgest on 4/12/2024.
//

#include "VulkanMain.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

#ifndef NDEBUG
constexpr bool bUseValidationLayers = true;
#else
constexpr bool bUseValidationLayers = false;
#endif

using namespace GraphicsAPI;
VkEngine::VkEngine(Win32::WindowManager& winManager_) : winManager_(winManager_)
{
}


VkEngine::~VkEngine()
{
    Cleanup();
}

void VkEngine::Cleanup()
{
    if (isInit) {
        DestroySwapchain();
        vkDestroySurfaceKHR(vd.instance_, vd.surface_, nullptr);
        vkDestroyDevice(vd.device_, nullptr);

        vkb::destroy_debug_utils_messenger(vd.instance_, vd.dbgMessenger_);
        vkDestroyInstance(vd.instance_, nullptr);
        winManager_.DestroyAppWindow();  // Ensure the window is destroyed

    }
}

void VkEngine::PrintAvailableExtensions()
{
    uint32_t extensionCount;
    // First call with nullptr to get the count of extensions
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // Allocate a vector to hold the extension properties
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // Second call to get the extension properties data
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    std::cout << "Available Extensions: " << std::endl;
    for (const auto& extension : availableExtensions) {
        std::cout << "\t" << extension.extensionName << " (version " << extension.specVersion << ")" << std::endl;
    }
}


std::string VkEngine::decodeDriverVersion(uint32_t driverVersion, uint32_t vendorID) {
    std::string versionString;
    if (vendorID == 0x10DE) { // NVIDIA
        versionString = std::to_string((driverVersion >> 22) & 0x3FF) + "." +
                        std::to_string((driverVersion >> 14) & 0xFF);
    } else if (vendorID == 0x8086) { // Intel
        versionString = std::to_string(driverVersion >> 14) + "." +
                        std::to_string(driverVersion & 0x3FFF);
    } else if (vendorID == 0x1002) { // AMD
        versionString = std::to_string((driverVersion >> 22) & 0x3FF) + "." +
                        std::to_string((driverVersion >> 12) & 0x3FF) + "." +
                        std::to_string(driverVersion & 0xFFF);
    } else { // Unknown Vendor
        versionString = "Unknown Driver Version";
    }
    return versionString;
}


void VkEngine::InitVulkan()
{
    LOG(INFO, "Initializing Vulkan");
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name ("OrgEngine")
                       .request_validation_layers ()
                       .require_api_version(1, 3)
                       .use_default_debug_messenger ()
                       .build ();
    if (!inst_ret) {
        LOG(ERR, "Failed to create Vulkan instance. Error: " + inst_ret.error().message());
    }

    vd.instance_ = inst_ret.value().instance; // Store the VkInstance
    vd.dbgMessenger_ = inst_ret.value().debug_messenger; // Store the debug messenger

    LOG(INFO, "Vulkan Instance and Debug Messenger created successfully.");
    // Surface creation (specific to window systems)
    CreateSurface(winManager_.getHInstance(), winManager_.getHwnd(), vd);
    LOG(INFO, "Win32 Surface created successfully.");

    // Device selection and setup
    vkb::PhysicalDeviceSelector selector{ inst_ret.value() };
    auto phys_dev_ret = selector
        .set_surface(vd.surface_)
        .set_minimum_version(1, 3) // Require Vulkan 1.3
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete) // Prefer discrete GPUs
        .select();

    if (!phys_dev_ret) {
        LOG(ERR, "Failed to select a Vulkan physical device. Error: " + phys_dev_ret.error().message() , __LINE__);
        return; // Early exit if no suitable device found
    }
    vd.physicalDevice_ = phys_dev_ret.value().physical_device; // Store the physical device

    // Store properties for logging
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vd.physicalDevice_, &deviceProperties);

    LOG(INFO, "Selected GPU: " + std::string(deviceProperties.deviceName));

    std::string driverVersionStr = decodeDriverVersion(deviceProperties.driverVersion, deviceProperties.vendorID);
    LOG(INFO, "Driver Version: " + driverVersionStr);

    // Device features and logical device creation
    vkb::DeviceBuilder deviceBuilder{ phys_dev_ret.value() };
    auto dev_ret = deviceBuilder.build();
    if (!dev_ret) {
        LOG(ERR, "Failed to create a logical device. Error: " + dev_ret.error().message());
        return;
    }
    vd.device_ = dev_ret.value().device; // Store the logical device

    VmaAllocatorCreateInfo allocInfo
    {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = vd.physicalDevice_,
        .device = vd.device_,
        .instance = vd.instance_,
    };
    vmaCreateAllocator(&allocInfo, &allocator_);
    LOG(INFO, "VMA Initalized :D");
    isInit = true;
}


void VkEngine::CreateSurface(HINSTANCE hInstance, HWND hwnd, VulkanData& vd)
{
    VkWin32SurfaceCreateInfoKHR createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = hInstance,
        .hwnd = hwnd,
    };

    if (vkCreateWin32SurfaceKHR(vd.instance_, &createInfo, nullptr, &vd.surface_) != VK_SUCCESS) {
        MessageBox(nullptr, L"Failed to create Vulkan Surface!, Check to see if your drivers are installed proprely",
                       L"GPU Error!", MB_OK);
        exit(1);
    }

}

void VkEngine::CreateSwapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{ vd.physicalDevice_,vd.device_, vd.surface_};

    swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat_ , .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    swapchainExtent_ = vkbSwapchain.extent;
    //store swapchain and its related images
    swapchain_ = vkbSwapchain.swapchain;
    swapchainImages_ = vkbSwapchain.get_images().value();
    swapchainImageViews_ = vkbSwapchain.get_image_views().value();
}

void VkEngine::InitSwapChain()
{
    CreateSwapchain(winManager_.GetWidth(), winManager_.GetHeight());
}

void VkEngine::DestroySwapchain()
{
    vkDestroySwapchainKHR(vd.device_, swapchain_, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < swapchainImageViews_.size(); i++) {

        vkDestroyImageView(vd.device_, swapchainImageViews_[i], nullptr);
    }
}



