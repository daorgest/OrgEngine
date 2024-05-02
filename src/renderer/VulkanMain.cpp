//
// Created by Orgest on 4/12/2024.
//

#include "VulkanMain.h"

#include "VkBootstrap.h"

#ifndef NDEBUG
constexpr bool bUseValidationLayers = true;
#else
constexpr bool bUseValidationLayers = false;
#endif

using namespace GraphicsAPI;



VkEngine::VkEngine(Win32::WindowManager& winManager) : winManager(winManager)
{
}


VkEngine::~VkEngine()
{
    vd.deletionQueue.flush();  // Clean up all Vulkan objects
}


void VkEngine::SetupDebugMessenger() {
    if (!bUseValidationLayers) {
        LOG(INFO, "Validation layers not enabled.");
        return; // Early exit if validation layers are not enabled
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    SetupDebugMessengerCreateInfo(createInfo);

    // The function pointer for creating the debug messenger
    auto createDebugUtilsMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vd.instance_, "vkCreateDebugUtilsMessengerEXT");
    if (!createDebugUtilsMessengerFunc) {
        LOG(ERR, "Failed to load vkCreateDebugUtilsMessengerEXT.");
        return;
    }

    // Create the debug messenger and handle possible errors
    if (createDebugUtilsMessengerFunc(vd.instance_, &createInfo, nullptr, &vd.dbgMessenger_) != VK_SUCCESS) {
        LOG(ERR, "Failed to set up debug messenger!");
        return;
    }

    // Add to the deletion queue to ensure it's cleaned up at the end
    vd.deletionQueue.push_function([=]() {
        auto destroyDebugUtilsMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vd.instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugUtilsMessengerFunc != nullptr) {
            destroyDebugUtilsMessengerFunc(vd.instance_, vd.dbgMessenger_, nullptr);
        } else {
            LOG(WARN, "Failed to load vkDestroyDebugUtilsMessengerEXT.");
        }
    });

    LOG(INFO, "Debug messenger created successfully.");
}

void VkEngine::SetupDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT
        type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32
    {
        LOG(DEBUGGING, "Validation layer: " + std::string(pCallbackData->pMessage));
        return VK_FALSE;
    };
    createInfo.pUserData = nullptr; // Optional
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
                        std::to_string((driverVersion >> 14) & 0xFF) + "." +
                        std::to_string((driverVersion >> 6) & 0xFF) + "." +
                        std::to_string(driverVersion & 0x3F);
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
    CreateSurface(winManager.getHInstance(), winManager.getHwnd(), vd);
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



