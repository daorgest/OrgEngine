//
// Created by Orgest on 4/12/2024.
//

#include "VulkanMain.h"

constexpr bool bUseValidationLayers = false;

using namespace GraphicsAPI;



VkEngine::VkEngine(Win32::WindowManager& winManager) : winManager(winManager)
{
}


VkEngine::~VkEngine()
{
    vd.deletionQueue.flush();  // Clean up all Vulkan objects
}


void VkEngine::SetupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    SetupDebugMessengerCreateInfo(createInfo);

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vd.instance_, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(vd.instance_, &createInfo, nullptr, &vd.dbgMessenger_);
        vd.deletionQueue.push_function([=]() {
            auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vd.instance_, "vkDestroyDebugUtilsMessengerEXT");
            if (destroyFunc != nullptr) {
                destroyFunc(vd.instance_, vd.dbgMessenger_, nullptr);
            }
        });
    } else {
        std::cerr << "Could not load vkCreateDebugUtilsMessengerEXT" << std::endl;
    }
}

void VkEngine::SetupDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32 {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
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
    Logger::Log(INFO, "Initilzing Vulkan");
    VkApplicationInfo appInfo
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "OrgEngine",
        .pEngineName = "OrgEngine",
        .apiVersion = VK_API_VERSION_1_3
    };

    const VkInstanceCreateInfo instInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(std::size(enabledExtensions)),
        .ppEnabledExtensionNames = enabledExtensions
    };

    if (vkCreateInstance(&instInfo, nullptr, &vd.instance_) != VK_SUCCESS) {
        MessageBox(nullptr, L"Failed to create Vulkan instance!, Check to see if your drivers are installed proprely",
                   L"GPU Error!", MB_OK);
        std::cerr << "Failed to create Vulkan instance!, Check to see if your drivers are installed proprely" << std::endl;
    }
    Logger::Log(INFO, "Vulkan Instance created!");

    // Setup debug messenger if validation layers are enabled
    if (bUseValidationLayers && CheckValidationLayerSupport()) {
        SetupDebugMessenger();
        Logger::Log(INFO, "Debug Messenger created!");
    } else if (bUseValidationLayers) {
        Logger::Log(WARN, "Validation layers requested, but not available!");
    }

    // Select a physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vd.instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vd.instance_, &deviceCount, devices.data());

    // For GPU Info
    VkPhysicalDeviceProperties deviceProperties;
    // Choose the best suitable device (e.g., preferring dedicated GPU)
    for (const auto& device : devices) {
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vd.physicalDevice_ = device;
            break;
        }
    }

    if (vd.physicalDevice_ == VK_NULL_HANDLE) {
        vd.physicalDevice_ = devices[0]; // Fallback to the first device if no dedicated GPU is found
    }

    Logger::Log(INFO, "GPU: ", deviceProperties.deviceName);

    std::string driverVersionStr = decodeDriverVersion(deviceProperties.driverVersion, deviceProperties.vendorID);
    // Assuming driverVersion is a version number you want to display as is
    Logger::Log(INFO, "Driver Version: ", driverVersionStr);

    // Decode the Vulkan API version number according to Vulkan's versioning scheme
    uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.apiVersion);
    uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.apiVersion);
    uint32_t apiPatch = VK_VERSION_PATCH(deviceProperties.apiVersion);
    Logger::Log(INFO, "Vulkan API Version: ", fmt::format("{}.{}.{}", apiMajor, apiMinor, apiPatch));

    // Create a logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;


    if (vkCreateDevice(vd.physicalDevice_, &deviceCreateInfo, nullptr, &vd.device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    CreateSurface(winManager.getHInstance(), winManager.getHwnd(), vd);
    Logger::Log(INFO, "Win32 Surface Created!");
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



