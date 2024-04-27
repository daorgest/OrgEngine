//
// Created by Orgest on 4/12/2024.
//

#include "VulkanMain.h"

#include "../Platform/PlatformWindows.h"

constexpr bool bUseValidationLayers = false;

using namespace GraphicsAPI;
Win32::WindowManager windowManager;


const HWND hwnd = windowManager.getHwnd();
const HINSTANCE hInstance = windowManager.getHInstance();

void Vulkan::InitVulkanAPI(const Win32::WindowManager& windowManager)
{

    InitVulkan(vd);
    PrintAvailableExtensions();
}

void Vulkan::PrintAvailableExtensions() const
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


void Vulkan::InitVulkan(VulkanData &vd)
{
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

    // CreateSurface(hInstance, hwnd, vd);

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

    Logger::Log(LogLevel::INFO, "GPU: ", deviceProperties.deviceName);

    // Assuming driverVersion is a version number you want to display as is
    Logger::Log(LogLevel::INFO, "Driver Version: ", std::to_string(deviceProperties.driverVersion));

    // Decode the Vulkan API version number according to Vulkan's versioning scheme
    uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.apiVersion);
    uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.apiVersion);
    uint32_t apiPatch = VK_VERSION_PATCH(deviceProperties.apiVersion);
    Logger::Log(LogLevel::INFO, "Vulkan API Version: ", fmt::format("{}.{}.{}", apiMajor, apiMinor, apiPatch));

    // Create a logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;


    if (vkCreateDevice(vd.physicalDevice_, &deviceCreateInfo, nullptr, &vd.device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

void Vulkan::CreateSurface(HINSTANCE hInstance, HWND hwnd, VulkanData& vd)
{
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = hInstance;
    createInfo.hwnd = hwnd;


    if (vkCreateWin32SurfaceKHR(vd.instance_, &createInfo, nullptr, &vd.surface_) != VK_SUCCESS) {
        MessageBox(nullptr, L"Failed to create Vulkan Surface!, Check to see if your drivers are installed proprely",
                       L"GPU Error!", MB_OK);
        exit(1);
    }
}



