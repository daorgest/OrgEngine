//
// Created by Orgest on 4/12/2024.
//

#include "VulkanMain.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "fmt/color.h"

#ifndef NDEBUG
constexpr bool bUseValidationLayers = true;
#else
constexpr bool bUseValidationLayers = false;
#endif

using namespace GraphicsAPI;

VkEngine::VkEngine(Win32::WindowManager *winManager) : winManager_(winManager)
{
   // // if (winManager)
   //      Init();
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
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();
        ImGuiMainMenu();

        ImGui::Render();

        Draw();
    }
}

void VkEngine::ImGuiMainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {

            if (ImGui::MenuItem("Option 1"))
            {

            }
            if (ImGui::MenuItem("Option 2"))
            {

            }

            ImGui::EndMenu();
        }


        ImGui::EndMainMenuBar();
    }
}

#pragma endregion Run

#pragma region Initialization

void VkEngine::InitImgui()
{
    // Create descriptor pool for IMGUI
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(vd.device_, &pool_info, nullptr, &imguiPool));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplWin32_Init(winManager_->getHwnd());

    ImGui_ImplVulkan_InitInfo init_info{
        .Instance = vd.instance_,
        .PhysicalDevice = vd.physicalDevice_,
        .Device = vd.device_,
        .Queue = graphicsQueue_,
        .DescriptorPool = imguiPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .UseDynamicRendering = true,
    };

    // dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat_;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    // add the destroy the imgui created structures
    mainDeletionQueue_.push_function([=]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(vd.device_, imguiPool, nullptr);
    });
}

void VkEngine::Init()
{
    if (winManager_)
    {
        InitVulkan();
        InitSwapChain();
        InitCommands();
        InitSyncStructures();
        InitDescriptors();
        InitPipelines();
        InitImgui();
    }
}

#pragma region Pipelines
void VkEngine::InitPipelines()
{
    InitBackgroundPipelines();
}

void VkEngine::InitBackgroundPipelines()
{
    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &drawImageDescriptorLayout_;
    computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(vd.device_, &computeLayout, nullptr, &gradientPipelineLayout_));

    VkShaderModule computeDrawShader;
    if (!LoadShader("gradient.comp.spv", vd.device_, &computeDrawShader))
    {
        fmt::print("Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stageinfo{};
    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageinfo.pNext = nullptr;
    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = computeDrawShader;
    stageinfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = gradientPipelineLayout_;
    computePipelineCreateInfo.stage = stageinfo;

    VK_CHECK(vkCreateComputePipelines(vd.device_,VK_NULL_HANDLE,1,&computePipelineCreateInfo,
        nullptr, &gradientPipeline_));

    vkDestroyShaderModule(vd.device_, computeDrawShader, nullptr);

    mainDeletionQueue_.push_function([&]() {
        vkDestroyPipelineLayout(vd.device_, gradientPipelineLayout_, nullptr);
        vkDestroyPipeline(vd.device_, gradientPipeline_, nullptr);
        });
}

#pragma endregion Pipelines

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

    VmaAllocatorCreateInfo allocInfo
    {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = vd.physicalDevice_,
        .device = vd.device_,
        .instance = vd.instance_,
    };
    vmaCreateAllocator(&allocInfo, &allocator_);
    mainDeletionQueue_.push_function([&]()
    {
        vmaDestroyAllocator(allocator_);
    });
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

            frames_[i].deletionQueue_.flush();
        }

        mainDeletionQueue_.flush();

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
    if (!hwnd)
    {
        MessageBox(nullptr, L"HWND is null! Cannot create Vulkan surface.",
                   L"Error", MB_OK | MB_ICONERROR);
        exit(1); // Or handle the error appropriately
    }
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

    // draw image size will match the window
	VkExtent3D drawImageExtent = {
		winManager_->GetWidth(),
		winManager_->GetHeight(),
		1
	};

	// hardcoding the draw format to 32 bit float
	drawImage_.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT; // LMFAO??????
	drawImage_.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = ImageCreateInfo(drawImage_.imageFormat, drawImageUsages, drawImageExtent);

	// for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rImageAllocInfo = {};
	rImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	vmaCreateImage(allocator_, &rimg_info, &rImageAllocInfo, &drawImage_.image, &drawImage_.allocation, nullptr);

	// build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = ImageViewCreateInfo(drawImage_.imageFormat, drawImage_.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(vd.device_, &rview_info, nullptr, &drawImage_.imageView));

	// add to deletion queues
	mainDeletionQueue_.push_function([this]() {
		vkDestroyImageView(vd.device_, drawImage_.imageView, nullptr);
		vmaDestroyImage(allocator_, drawImage_.image, drawImage_.allocation);
	});
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
    VK_CHECK(vkCreateCommandPool(vd.device_, &commandPoolInfo, nullptr, &immCommandPool_));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = CommandBufferAllocateInfo(immCommandPool_, 1);

    VK_CHECK(vkAllocateCommandBuffers(vd.device_, &cmdAllocInfo, &immCommandBuffer_));

    mainDeletionQueue_.push_function([=]() {
        vkDestroyCommandPool(vd.device_, immCommandPool_, nullptr);
    });
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

    VK_CHECK(vkCreateFence(vd.device_, &fenceCreateInfo, nullptr, &immFence_));
    mainDeletionQueue_.push_function([=]()
        { vkDestroyFence(vd.device_, immFence_, nullptr); });
}

void VkEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_CHECK(vkResetFences(vd.device_, 1, &immFence_));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer_, 0));

    VkCommandBuffer cmd = immCommandBuffer_;

    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = CommandBufferSubmitInfo(cmd);
    VkSubmitInfo2 submit = SubmitInfo(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submit, immFence_));

    VK_CHECK(vkWaitForFences(vd.device_, 1, &immFence_, true, 9999999999));
}


#pragma endregion Fence

#pragma region Image

VkImageCreateInfo VkEngine::ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
    VkImageCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usageFlags
    };

    return info;
}

VkImageViewCreateInfo VkEngine::ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    return info;
}

void VkEngine::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo {
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .pNext = nullptr,
        .srcImage = source,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = destination,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &blitRegion,
        .filter = VK_FILTER_LINEAR
    };

    vkCmdBlitImage2(cmd, &blitInfo);
}

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
        .subresourceRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)
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

#pragma region Compute-Shaders

bool VkEngine::LoadShader(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
{
    // open the file. With cursor at the end
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char*)buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

#pragma endregion Compute-Shaders

#pragma region Descriptor

void VkEngine::InitDescriptors()
{
    //create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    };

    globalDescriptorAllocator.InitPool(vd.device_, 10, sizes);

    //make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        drawImageDescriptorLayout_ = builder.Build(vd.device_, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    //allocate a descriptor set for our draw image
    drawImageDescriptors_ = globalDescriptorAllocator.Allocate(vd.device_,drawImageDescriptorLayout_);

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = drawImage_.imageView;

    VkWriteDescriptorSet drawImageWrite = {};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = drawImageDescriptors_;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(vd.device_, 1, &drawImageWrite, 0, nullptr);

    //make sure both the descriptor allocator and the new layout get cleaned up properly
    mainDeletionQueue_.push_function([&]() {
        globalDescriptorAllocator.DestroyPool(vd.device_);

        vkDestroyDescriptorSetLayout(vd.device_, drawImageDescriptorLayout_, nullptr);
    });
}

void DescriptorLayoutBuilder::AddBinding(u32 binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newbind
    {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1
    };

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::Clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext,
    VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : bindings) {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}


void DescriptorAllocator::InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * maxSets)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = 0;
    pool_info.maxSets = maxSets;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void DescriptorAllocator::ClearDescriptors(VkDevice device)
{
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::DestroyPool(VkDevice device)
{
    vkDestroyDescriptorPool(device,pool,nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

    return ds;
}

#pragma endregion Descriptor

#pragma region Render

VkRenderingInfo VkEngine::RenderInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment = nullptr)
{
    VkRenderingInfo renderInfo = {};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;
    renderInfo.renderArea.extent = extent; // Swapchain extent or desired extent
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1; // Assuming only one color attachment for ImGui
    renderInfo.pColorAttachments = colorAttachment;
    renderInfo.pDepthAttachment = depthAttachment;

    return renderInfo;
}

VkRenderingAttachmentInfo VkEngine::AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
{
    VkRenderingAttachmentInfo colorAttachment
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = view,
        .imageLayout = layout,
        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE
    };

    if (clear)
        colorAttachment.clearValue = *clear;

    return colorAttachment;
}

#pragma endregion Render

#pragma region Draw

void VkEngine::DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo colorAttachment = AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = RenderInfo(swapchainExtent_, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VkEngine::DrawBackground(VkCommandBuffer cmd)
{
    // //make a clear-color from frame number. This will flash with a 120 frame period.
    // VkClearColorValue clearValue;
    // float flash = std::abs(std::sin(frameNumber_ / 120.f));
    // clearValue = { { 0.0f, 0.0f, flash, 1.0f } };
    //
    // VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    //
    // //clear image
    // vkCmdClearColorImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    // bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline_);

    // bind the descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout_, 0, 1, &drawImageDescriptors_, 0, nullptr);

    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, std::ceil(drawExtent_.width / 16.0), std::ceil(drawExtent_.height / 16.0), 1);
}

void VkEngine::Draw()
{
    // wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(vd.device_, 1, &GetCurrentFrame().renderFence_, true, 1000000000));
    GetCurrentFrame().deletionQueue_.flush();

    VK_CHECK(vkResetFences(vd.device_, 1, &GetCurrentFrame().renderFence_));

    u32 swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(vd.device_, swapchain_, 1000000000, GetCurrentFrame().swapChainSemaphore_, nullptr, &swapchainImageIndex));

    // naming it cmd for shorter writing
    const VkCommandBuffer cmd = GetCurrentFrame().mainCommandBuffer_;

    // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    drawExtent_.width = drawImage_.imageExtent.width;
    drawExtent_.height = drawImage_.imageExtent.height;

    // start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Make the swapchain image into writable mode before rendering
    TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    DrawBackground(cmd);

    // Transition swapchain image to transfer destination optimal layout
    TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy image from drawImage_.image to swapchainImages_[swapchainImageIndex]
    CopyImageToImage(cmd, drawImage_.image, swapchainImages_[swapchainImageIndex], drawExtent_, swapchainExtent_);

    // Transition swapchain image to color attachment optimal layout
    TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);


    DrawImGui(cmd, swapchainImageViews_[swapchainImageIndex]);

    // Transition swapchain image to present source optimal layout
    TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


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
