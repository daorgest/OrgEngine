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

VkEngine::VkEngine(Win32::WindowManager *winManager) : winManager_(winManager)
{
}

VkEngine::~VkEngine()
{
    Cleanup();
}

#pragma region Run

void VkEngine::Run()
{
    MSG msg = {};
    bool bQuit = false;

    while (!bQuit) {
        // Handle events on queue
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                bQuit = true;
            }

            if (msg.message == WM_SYSCOMMAND) {
                if (msg.wParam == SC_MINIMIZE) {
                    stopRendering_ = true;
                } else if (msg.wParam == SC_RESTORE) {
                    stopRendering_ = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stopRendering_) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Draw();
    }
}

#pragma endregion Run

#pragma region Initialization

void VkEngine::Init()
{
    if (winManager_)
    {
        InitVulkan();
        InitSwapChain();
        InitCommands();
        InitSyncStructures();
    }
}

void VkEngine::InitVulkan()
{
    LOG(INFO, "Initializing Vulkan");

    vkb::InstanceBuilder builder;
    auto instRet = builder.set_app_name("OrgEngine")
                          .request_validation_layers(bUseValidationLayers)
                          .require_api_version(1, 3)
                          .use_default_debug_messenger()
                          .build();

    if (!instRet) {
        LOG(ERR, "Failed to create Vulkan instance. Error: " + instRet.error().message());
        return;
    }

    vd.instance_ = instRet.value().instance;
    vd.dbgMessenger_ = instRet.value().debug_messenger;
    LOG(INFO, "Vulkan Instance and Debug Messenger created successfully.");

    CreateSurfaceWin32(winManager_->getHInstance(), winManager_->getHwnd(), vd);
    LOG(INFO, "Win32 Surface created successfully.");

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{ instRet.value() };
    auto physDeviceRet = selector.set_surface(vd.surface_)
        .set_minimum_version(1, 3)
        .set_required_features_13(features13)
        .set_required_features_12(features12)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .select();
    if (!physDeviceRet) {
        LOG(ERR, "Failed to select a Vulkan physical device. Error: " + physDeviceRet.error().message(), __LINE__);
        return;
    }

    vd.physicalDevice_ = physDeviceRet.value().physical_device;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vd.physicalDevice_, &deviceProperties);

    LOG(INFO, "Selected GPU: " + std::string(deviceProperties.deviceName));
    LOG(INFO, "Driver Version: " + decodeDriverVersion(deviceProperties.driverVersion, deviceProperties.vendorID));

    vkb::DeviceBuilder deviceBuilder{ physDeviceRet.value() };
    auto devRet = deviceBuilder.build();
    if (!devRet) {
        LOG(ERR, "Failed to create a logical device. Error: " + devRet.error().message());
        return;
    }
    vd.device_ = devRet.value().device;

    VmaAllocatorCreateInfo allocInfo{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = vd.physicalDevice_,
        .device = vd.device_,
        .instance = vd.instance_,
    };
    vmaCreateAllocator(&allocInfo, &allocator_);
    LOG(INFO, "VMA Initialized :D");

    graphicsQueue_ = devRet.value().get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily_ = devRet.value().get_queue_index(vkb::QueueType::graphics).value();

    InitializeCommandPoolsAndBuffers();
    isInit = true;
}


#pragma endregion Initialization

#pragma region Cleanup

void VkEngine::Cleanup()
{
    if (isInit) {

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            // Already written from before
            vkDestroyCommandPool(vd.device_, frames_[i].commandPool_, nullptr);

            // Destroy sync objects
            vkDestroyFence(vd.device_, frames_[i].renderFence_, nullptr);
            vkDestroySemaphore(vd.device_, frames_[i].renderSemaphore_, nullptr);
            vkDestroySemaphore(vd.device_, frames_[i].swapChainSemaphore_, nullptr);
        }

        vkDeviceWaitIdle(vd.device_);
        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(vd.device_, frames_[i].commandPool_, nullptr);
        }
        DestroySwapchain();
        vkDestroySurfaceKHR(vd.instance_, vd.surface_, nullptr);
        vkDestroyDevice(vd.device_, nullptr);

        vkb::destroy_debug_utils_messenger(vd.instance_, vd.dbgMessenger_);
        vkDestroyInstance(vd.instance_, nullptr);
        if (winManager_)
            winManager_->DestroyAppWindow();

    }
}

#pragma endregion Cleanup

#pragma region Helper Functions

void VkEngine::PrintAvailableExtensions()
{
    u32 extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    std::cout << "Available Extensions: " << std::endl;
    for (const auto& extension : availableExtensions) {
        std::cout << "\t" << extension.extensionName << " (version " << extension.specVersion << ")" << std::endl;
    }
}

std::string VkEngine::decodeDriverVersion(u32 driverVersion, u32 vendorID)
{
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

#pragma endregion Helper Functions

#pragma region Surface Management

void VkEngine::CreateSurfaceWin32(HINSTANCE hInstance, HWND hwnd, VulkanData& vd)
{
    VkWin32SurfaceCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = hInstance,
        .hwnd = hwnd,
    };

    if (vkCreateWin32SurfaceKHR(vd.instance_, &createInfo, nullptr, &vd.surface_) != VK_SUCCESS) {
        MessageBox(nullptr, L"Failed to create Vulkan Surface!, Check to see if your drivers are installed properly",
                   L"GPU Error!", MB_OK);
        exit(1);
    }
}

#pragma endregion Surface Management

#pragma region Swapchain Management

void VkEngine::CreateSwapchain(u32 width, u32 height)
{
    vkb::SwapchainBuilder swapchainBuilder{ vd.physicalDevice_, vd.device_, vd.surface_ };

    swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .set_desired_format(VkSurfaceFormatKHR{
            .format = swapchainImageFormat_,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    swapchainExtent_ = vkbSwapchain.extent;
    swapchain_ = vkbSwapchain.swapchain;
    swapchainImages_ = vkbSwapchain.get_images().value();
    swapchainImageViews_ = vkbSwapchain.get_image_views().value();
}

void VkEngine::InitSwapChain()
{
    CreateSwapchain(winManager_->GetWidth(), winManager_->GetHeight());
}

void VkEngine::DestroySwapchain() const
{
    vkDestroySwapchainKHR(vd.device_, swapchain_, nullptr);

    for (int i = 0; i < swapchainImageViews_.size(); i++) {
        vkDestroyImageView(vd.device_, swapchainImageViews_[i], nullptr);
    }
}

#pragma endregion Swapchain Management

#pragma region Command Pool

void VkEngine::InitCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo = CommandPoolCreateInfo(graphicsQueueFamily_,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(vd.device_, &commandPoolInfo, nullptr, &frames_[i].commandPool_));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = CommandBufferAllocateInfo(frames_[i].commandPool_, 1);

        VK_CHECK(vkAllocateCommandBuffers(vd.device_, &cmdAllocInfo, &frames_[i].mainCommandBuffer_));
    }
}


void VkEngine::InitializeCommandPoolsAndBuffers()
{
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        FrameData& frame = frames_[i];

        VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphicsQueueFamily_,
        };

        if (vkCreateCommandPool(vd.device_, &poolInfo, nullptr, &frame.commandPool_) != VK_SUCCESS) {
            LOG(ERR, "Failed to create command pool");
            return;
        }

        VkCommandBufferAllocateInfo cmdAllocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frame.commandPool_,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        if (vkAllocateCommandBuffers(vd.device_, &cmdAllocInfo, &frame.mainCommandBuffer_) != VK_SUCCESS) {
            LOG(ERR, "Failed to allocate command buffers");
            return;
        }
    }
}

VkCommandPoolCreateInfo VkEngine::CommandPoolCreateInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags /*=0*/)
{
    const VkCommandPoolCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queueFamilyIndex
    };
    return info;
}

VkCommandBufferAllocateInfo VkEngine::CommandBufferAllocateInfo(VkCommandPool pool, u32 count /*= 1*/)
{
    VkCommandBufferAllocateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr
    };

    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkCommandBufferBeginInfo VkEngine::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags /*= 0*/)
{
    VkCommandBufferBeginInfo info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = nullptr
    };
    return info;

}

#pragma endregion Command Pool

#pragma region Semaphore

VkSemaphoreSubmitInfo VkEngine::SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
{
    return {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = semaphore,
        .value = 1,
        .stageMask = stageMask,
        .deviceIndex = 0,
    };
}

VkCommandBufferSubmitInfo VkEngine::CommandBufferSubmitInfo(VkCommandBuffer cmd)
{
    return {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = cmd,
        .deviceMask = 0
    };
}

VkSubmitInfo2 VkEngine::SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                         VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    return VkSubmitInfo2
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .waitSemaphoreInfoCount = static_cast<u32>(waitSemaphoreInfo == nullptr ? 0 : 1),
        .pWaitSemaphoreInfos = waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,
        .signalSemaphoreInfoCount = static_cast<u32>(signalSemaphoreInfo == nullptr ? 0 : 1),
        .pSignalSemaphoreInfos = signalSemaphoreInfo
    };
}

#pragma endregion Semaphore

#pragma region Fence

VkFenceCreateInfo VkEngine::FenceCreateInfo(VkFenceCreateFlags flags /*=0*/) const
{
    const VkFenceCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };

    return info;
}

VkSemaphoreCreateInfo VkEngine::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags /*= 0*/) const
{
    const VkSemaphoreCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };

    return info;
}

void VkEngine::InitSyncStructures()
{
    // Create synchronization structures:
    // - One fence to control when the GPU has finished rendering the frame.
    // - Two semaphores to synchronize rendering with the swapchain.
    // We want the fence to start signaled so we can wait on it on the first frame.
    VkFenceCreateInfo fenceCreateInfo = FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = SemaphoreCreateInfo(0);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(vd.device_, &fenceCreateInfo, nullptr, &frames_[i].renderFence_));

        VK_CHECK(vkCreateSemaphore(vd.device_, &semaphoreCreateInfo, nullptr, &frames_[i].swapChainSemaphore_));
        VK_CHECK(vkCreateSemaphore(vd.device_, &semaphoreCreateInfo, nullptr, &frames_[i].renderSemaphore_));
    }
}

#pragma endregion Fence

#pragma region Image

void VkEngine::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    // Set default stage and access masks to the most conservative values, which are safe but not very efficient
    VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    // Optimize the stage and access masks based on specific transitions
    if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        // Transition from undefined layout to transfer destination layout
        // This means we are preparing the image to receive data through a transfer operation
        srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT; // No previous commands need to complete
        srcAccessMask = 0; // No access mask as the image is in an undefined state
        dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT; // Preparing for a transfer operation
        dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT; // Image will be written by a transfer operation
    } else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Transition from transfer destination layout to shader read-only layout
        // This means the image has been filled with data and is now ready to be read by a shader
        srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT; // Previous transfer operation must complete
        srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT; // Ensure all writes to the image are complete
        dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT; // Preparing for reading in a fragment shader
        dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT; // Image will be read by a shader
    } else if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        // Transition from undefined layout to depth/stencil attachment optimal layout
        // This prepares the image to be used as a depth/stencil buffer
        srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT; // No previous commands need to complete
        srcAccessMask = 0; // No access mask as the image is in an undefined state
        dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT; // Preparing for depth/stencil tests
        dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // Image will be read and written during depth/stencil tests
    }

    // Create the image barrier with the specified parameters
    VkImageMemoryBarrier2 imageBarrier
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = ImageSubresourceRange((newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT)
    };

    // Create the dependency info structure to include the image barrier
    VkDependencyInfo depInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

VkImageSubresourceRange VkEngine::ImageSubresourceRange(VkImageAspectFlags aspectMask) const
{
    return VkImageSubresourceRange
    {
        .aspectMask = aspectMask,       // Specifies which aspects of the image are included in the view (e.g., color, depth, stencil).
        .baseMipLevel = 0,              // The first mipmap level accessible to the view.
        .levelCount = VK_REMAINING_MIP_LEVELS, // Specifies the number of mipmap levels (VK_REMAINING_MIP_LEVELS to include all levels).
        .baseArrayLayer = 0,            // The first array layer accessible to the view.
        .layerCount = VK_REMAINING_ARRAY_LAYERS // Specifies the number of array layers (VK_REMAINING_ARRAY_LAYERS to include all layers).
    };
}
#pragma endregion Image

#pragma region Draw

void VkEngine::Draw()
{
    // wait until the gpu has finished rendering the last frame. Timeout of 1
    // second
    VK_CHECK(vkWaitForFences(vd.device_, 1, &GetCurrentFrame().renderFence_, true, 1000000000));
    VK_CHECK(vkResetFences(vd.device_, 1, &GetCurrentFrame().renderFence_));

    u32 swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(vd.device_, swapchain_, 1000000000,
        GetCurrentFrame().swapChainSemaphore_, nullptr, &swapchainImageIndex));

    //naming it cmd for shorter writing
    VkCommandBuffer cmd = GetCurrentFrame().mainCommandBuffer_;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    // Make the swapchain image into writable mode before rendering
    TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL);

    // Make a clear-color from frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(frameNumber_ / 120.0f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    // Clear Color Image
    vkCmdClearColorImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1,
        &clearRange);

    // make the swapchain image into presentable mode
    TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    // Prepare the submission to the queue.
    // We want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready.
    // We will signal the _renderSemaphore, to signal that rendering has finished.

    VkCommandBufferSubmitInfo cmdinfo = CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapChainSemaphore_);
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore_);

    VkSubmitInfo2 submit = SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

    // Submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution.
    VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submit, GetCurrentFrame().renderFence_));


    // Prepare present
    // This will put the image we just rendered into the visible window.
    // We want to wait on the _renderSemaphore_,
    // as it's necessary that drawing commands have finished before the image is displayed to the user.

    VkPresentInfoKHR presentInfo_
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &GetCurrentFrame().renderSemaphore_,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_,
        .pImageIndices = &swapchainImageIndex
    };

    VK_CHECK(vkQueuePresentKHR(graphicsQueue_, &presentInfo_));

    // Increase the number of frames drawn
    frameNumber_++;
}

#pragma endregion Draw
