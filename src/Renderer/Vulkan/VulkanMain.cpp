//
// Created by Orgest on 4/12/2024.
//

#include <backends/imgui_impl_win32.h>
#ifdef VULKAN_BUILD
#include "VulkanMain.h"

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tracy/TracyVulkan.hpp>

#ifndef NDEBUG
constexpr bool bUseValidationLayers = true;
#else
constexpr bool bUseValidationLayers = false;
#endif

using namespace GraphicsAPI::Vulkan;

VkEngine::VkEngine(Platform::WindowContext *winManager) :
	swapchainImageFormat_(), windowContext_(winManager), allocator_(nullptr),
	frames_{}, meshPipelineLayout_(nullptr), meshPipeline_(nullptr), rectangle()
{
	Init();
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

	while (!bQuit)
	{
		// Handle events on queue
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				bQuit = true;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_SYSCOMMAND)
			{
				if (msg.wParam == SC_MINIMIZE)
				{
					stopRendering_ = true;
				}
				else if (msg.wParam == SC_RESTORE)
				{
					stopRendering_ = false;
					resizeRequested_ = true;
				}
			}

			// Handle window size changes
			if (msg.message == WM_SIZE)
			{
				if (msg.wParam != SIZE_MINIMIZED)
				{

					resizeRequested_ = true;
				}
			}
		}
		// win32_->input.update();

		// If WM_QUIT was received, exit the loop
		if (bQuit)
		{
			break;
		}

		if (resizeRequested_)
			ResizeSwapchain();

		// Do not draw if we are minimized
		if (stopRendering_)
		{
			// Throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Start ImGui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		// Render ImGui components
		RenderUI();
		ImGui::Render();

		// Draw the frame
		Draw();
		// win32_->input.reset();
	}

	Cleanup();
}

void VkEngine::ResizeSwapchain()
{

	vkDeviceWaitIdle(vd.device_);

	DestroySwapchain();

	RECT rect;
	GetClientRect(windowContext_->hwnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	swapchainExtent_.width = width;
	swapchainExtent_.height = height;

	CreateSwapchain(swapchainExtent_.width, swapchainExtent_.height);

	resizeRequested_ = false;
}

#pragma endregion Run

#pragma region UI

void VkEngine::RenderQuickStatsImGui()
{
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

	if (ImGui::Begin("Resolution", nullptr, windowFlags))
	{
		u32 width, height;
		windowContext_->GetWindowSize(width, height);
		ImGui::Text("Window Resolution: %ux%u", width, height);
		ImGui::Text("Internal Resolution: %ux%u", drawExtent_.width, drawExtent_.height);
	}
	ImGui::End();
}


void VkEngine::RenderMemoryUsageImGui()
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Memory Usage Debugger", nullptr, windowFlags))
    {
        // Get the budget info for each heap
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(allocator_, budgets);

        static bool autoRefresh = true;
        static float refreshTimer = 0.0f;
        const float refreshInterval = 1.0f; // Refresh every 1 second

        // Heap statistics
        if (ImGui::CollapsingHeader("Heap Statistics"))
        {
            // Loop through each memory heap
            for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
            {
                // Only display heaps that have a non-zero budget
                if (budgets[i].budget > 0)
                {
                    ImGui::Text("Heap %u:", i);
                    ImGui::Indent();

                    ImGui::Text("Block Count: %u", budgets[i].statistics.blockCount);
                    ImGui::Text("Allocation Count: %u", budgets[i].statistics.allocationCount);

                    ImGui::Text("Used Memory: %.2f MB", budgets[i].usage / (1024.0 * 1024.0));
                    ImGui::Text("Budget: %.2f MB", budgets[i].budget / (1024.0 * 1024.0));

                    ImGui::Text("Allocation Bytes: %.2f MB", budgets[i].statistics.allocationBytes / (1024.0 * 1024.0));
                    ImGui::Text("Block Bytes: %.2f MB", budgets[i].statistics.blockBytes / (1024.0 * 1024.0));

                    ImGui::Unindent();
                    ImGui::Separator();
                }
            }
        }

        // Detailed allocation stats
        if (ImGui::CollapsingHeader("Detailed Allocations"))
        {
            ImGui::Checkbox("Auto Refresh", &autoRefresh);

            if (autoRefresh)
            {
                refreshTimer += ImGui::GetIO().DeltaTime;
                if (refreshTimer >= refreshInterval)
                {
                    refreshTimer = 0.0f;
                    char* statsString = nullptr;
                    vmaBuildStatsString(allocator_, &statsString, VK_TRUE);
                    ImGui::TextWrapped("%s", statsString);
                    vmaFreeStatsString(allocator_, statsString);
                }
            }
            else
            {
                if (ImGui::Button("Manual Refresh"))
                {
                    char* statsString = nullptr;
                    vmaBuildStatsString(allocator_, &statsString, VK_TRUE);
                    ImGui::TextWrapped("%s", statsString);
                    vmaFreeStatsString(allocator_, statsString);
                }
            }
        }
    }
    ImGui::End();
}




void VkEngine::RenderMainMenu()
{
	if (ImGui::BeginMainMenuBar())
	{
		// File Menu
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene")) { /* Handle new scene */ }
			if (ImGui::MenuItem("Save Scene")) { /* Handle save scene */ }
			if (ImGui::MenuItem("Exit Program")) { PostQuitMessage(0); }
			ImGui::EndMenu();
		}

		// View Menu
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Toggle Fullscreen"))
			{
				windowContext_->ToggleFullscreen();
			}
			ImGui::EndMenu();
		}

		// Options Menu
		if (ImGui::BeginMenu("Options"))
		{
			ImGui::SliderFloat("Render Scale", &renderScale, 0.3f, 1.f);

			// Background effect settings
			ImGui::Separator();
			ComputeEffect& selected = backgroundEffects[currentBackgroundEffect_];
			ImGui::Text("Selected effect: %s", selected.name);
			ImGui::SliderInt("Effect Index", &currentBackgroundEffect_, 0, static_cast<int>(backgroundEffects.size()) - 1);
			ImGui::InputFloat4("data1", glm::value_ptr(selected.data.data1));
			ImGui::InputFloat4("data2", glm::value_ptr(selected.data.data2));
			ImGui::InputFloat4("data3", glm::value_ptr(selected.data.data3));
			ImGui::InputFloat4("data4", glm::value_ptr(selected.data.data4));

			// Camera settings
			ImGui::Separator();
			ImGui::Text("Camera Settings");
			ImGui::SliderFloat3("Position", glm::value_ptr(cameraPosition), -100.0f, 100.0f);
			ImGui::SliderFloat("FOV", &fov, 1.0f, 180.0f);
			ImGui::InputFloat("Near Plane", &nearPlane, 0.01f, 1.0f, "%.2f");
			ImGui::InputFloat("Far Plane", &farPlane, 10.0f, 1000.0f, "%.2f");

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}



void VkEngine::RenderUI()
{
	// Main Menu
	RenderMainMenu();

	// Panels for Quick Stats, VRAM, etc.
	RenderQuickStatsImGui();
	RenderMemoryUsageImGui(); // Call separately
}


#pragma endregion UI

#pragma region Initialization

bool VkEngine::Init()
{
	if (windowContext_)
	{
		InitVulkan();
		// SetupDebugMessenger();
		InitSwapChain();
		InitCommands();
		InitSyncStructures();
		InitDescriptors();
		InitPipelines();
		InitDefaultData();
		InitImgui();
		isInit = true;
		return true;
	}
	return false;
}

void VkEngine::InitVulkan()
{
	ZoneScoped;

	vkb::InstanceBuilder builder;
	auto instRet = builder.set_app_name("OrgEngine")
	                      .request_validation_layers(bUseValidationLayers)
	                      .require_api_version(1, 3)
	                      .use_default_debug_messenger()
	                      .build();

	if (!instRet)
	{
		LOG(ERR, "Failed to create Vulkan instance. Error: " + instRet.error().message());
		return;
	}

	vd.instance_ = instRet.value().instance;
	vd.dbgMessenger_ = instRet.value().debug_messenger;

	CreateSurfaceWin32(windowContext_->hInstance, windowContext_->hwnd, vd);
	LOG(INFO, "Win32 Surface created successfully.");

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{};
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{};
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{instRet.value()};
	auto physDeviceRet = selector.set_surface(vd.surface_)
	                             .set_minimum_version(1, 3)
	                             .set_required_features_13(features13)
	                             .set_required_features_12(features12)
	                             .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
	                             .select();
	if (!physDeviceRet)
	{
		LOG(ERR, "Failed to select a Vulkan physical device. Error: " + physDeviceRet.error().message());
		return;
	}
	vd.physicalDevice_ = physDeviceRet.value().physical_device;
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(vd.physicalDevice_, &deviceProperties);
	LOG(INFO, "Selected GPU: " + std::string(deviceProperties.deviceName));
	LOG(INFO, "Driver Version: " + decodeDriverVersion(deviceProperties.driverVersion, deviceProperties.vendorID));

	vkb::DeviceBuilder deviceBuilder{physDeviceRet.value()};
	auto devRet = deviceBuilder.build();
	if (!devRet)
	{
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
	mainDeletionQueue_.pushFunction([&]()
	{
		vmaDestroyAllocator(allocator_); }, "Vma Allocator");
	LOG(INFO, "VMA Initialized :D");

	graphicsQueue_ = devRet.value().get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily_ = devRet.value().get_queue_index(vkb::QueueType::graphics).value();

	InitializeCommandPoolsAndBuffers();
#ifdef TRACY_ENABLE
	TracyVkContext(vd.physicalDevice_, vd.device_, graphicsQueue_, GetCurrentFrame().mainCommandBuffer_);
#endif
	isInit = true;
}

void VkEngine::InitImgui()
{
	// Create descriptor pool for IMGUI
	const VkDescriptorPoolSize pool_sizes[] = {
		{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = static_cast<u32>(std::size(pool_sizes));
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(vd.device_, &pool_info, nullptr, &imguiPool));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	io.ConfigDebugIsDebuggerPresent = true;

	ImGui_ImplWin32_Init(windowContext_->hwnd);

	ImGui_ImplVulkan_InitInfo init_info
	{
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
	mainDeletionQueue_.pushFunction([device = vd.device_, pool = imguiPool]
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(device, pool, nullptr);
		},
		" Imgui");
}

#pragma endregion Initialization

#pragma region Cleanup

void VkEngine::Cleanup()
{
	if (isInit)
	{
		vkDeviceWaitIdle(vd.device_);
		TracyVkDestroy(tracyContext_);

		for (auto & frame : frames_)
		{
			// Destroy sync objects
			vkDestroyFence(vd.device_, frame.renderFence_, nullptr);
			vkDestroySemaphore(vd.device_, frame.renderSemaphore_, nullptr);
			vkDestroySemaphore(vd.device_, frame.swapChainSemaphore_, nullptr);

			frame.deletionQueue_.Flush();
			vkDestroyCommandPool(vd.device_, frame.commandPool_, nullptr);
		}

		for (const auto &mesh : testMeshes)
		{
			DestroyBuffer(mesh->meshBuffers.indexBuffer);
			DestroyBuffer(mesh->meshBuffers.vertexBuffer);
		}
		DestroySwapchain();
		mainDeletionQueue_.Flush();

		vkDestroySurfaceKHR(vd.instance_, vd.surface_, nullptr);
		vkDestroyDevice(vd.device_, nullptr);

		vkb::destroy_debug_utils_messenger(vd.instance_, vd.dbgMessenger_);
		vkDestroyInstance(vd.instance_, nullptr);
		isInit = false;
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
	for (const auto& extension : availableExtensions)
	{
		std::cout << "\t" << extension.extensionName << " (version " << extension.specVersion << ")" << std::endl;
	}
}

std::string VkEngine::decodeDriverVersion(u32 driverVersion, u32 vendorID)
{
	std::string versionString;
	if (vendorID == 0x10DE)
	{
		// NVIDIA
		versionString = std::to_string((driverVersion >> 22) & 0x3FF) + "." +
			std::to_string((driverVersion >> 14) & 0xFF);
	}
	else if (vendorID == 0x8086)
	{
		// Intel
		versionString = std::to_string(driverVersion >> 14) + "." +
			std::to_string(driverVersion & 0x3FFF);
	}
	else if (vendorID == 0x1002)
	{
		// AMD
		versionString = std::to_string((driverVersion >> 22) & 0x3FF) + "." +
			std::to_string((driverVersion >> 12) & 0x3FF) + "." +
			std::to_string(driverVersion & 0xFFF);
	}
	else
	{
		// Unknown Vendor
		versionString = "Unknown Driver Version";
	}
	return versionString;
}

#pragma endregion Helper Functions

#pragma region Surface Management

void VkEngine::CreateSurfaceWin32(HINSTANCE hInstance, HWND hwnd, VulkanData& vd)
{
	VkWin32SurfaceCreateInfoKHR createInfo
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = hInstance,
		.hwnd = hwnd,
	};

	if (vkCreateWin32SurfaceKHR(vd.instance_, &createInfo, nullptr, &vd.surface_) != VK_SUCCESS)
	{
		LOG(ERR, "Failed to create Vulkan Surface! Check if your drivers are installed properly.");
	}
}

#pragma endregion Surface Management

#pragma region Swapchain Management

void VkEngine::CreateSwapchain(u32 width, u32 height)
{
	vkb::SwapchainBuilder swapchainBuilder{vd.physicalDevice_, vd.device_, vd.surface_};

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
	LOG(INFO, "Swapchain created/recreated, Resolution: ", width, "x" , height);
}

VkExtent3D VkEngine::getScreenResolution() const
{
	return
	{
		.width = windowContext_->screenWidth,
		.height = windowContext_->screenHeight,
		.depth = 1,
	};
}

void VkEngine::InitSwapChain()
{
    // Destroy the old swapchain before creating a new one
    DestroySwapchain();

    // Recreate surface
    CreateSurfaceWin32(windowContext_->hInstance, windowContext_->hwnd, vd);

    CreateSwapchain(windowContext_->screenWidth, windowContext_->screenHeight);


    // hardcoding the draw format to 32 bit float
    drawImage_.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT; // LMFAO??????
    drawImage_.imageExtent = getScreenResolution();

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	VkImageCreateInfo drawImageInfo = VkInfo::ImageInfo(drawImage_.imageFormat, drawImageUsages, getScreenResolution());
	CreateImageWithVMA(drawImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, drawImage_.image, drawImage_.allocation);

	VkImageViewCreateInfo drawImageViewInfo = VkInfo::ImageViewInfo(drawImage_.imageFormat, drawImage_.image, VK_IMAGE_ASPECT_COLOR_BIT);
	VK_CHECK(vkCreateImageView(vd.device_, &drawImageViewInfo, nullptr, &drawImage_.imageView));

	// Set up depth image
	depthImage_.imageFormat = VK_FORMAT_D32_SFLOAT;
	depthImage_.imageExtent = getScreenResolution();

	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageCreateInfo depthImageInfo = VkInfo::ImageInfo(depthImage_.imageFormat, depthImageUsages, getScreenResolution());
	CreateImageWithVMA(depthImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage_.image, depthImage_.allocation);

	VkImageViewCreateInfo depthImageViewInfo = VkInfo::ImageViewInfo(depthImage_.imageFormat, depthImage_.image, VK_IMAGE_ASPECT_DEPTH_BIT);
	VK_CHECK(vkCreateImageView(vd.device_, &depthImageViewInfo, nullptr, &depthImage_.imageView));

	// Add cleanup to deletion queue
	mainDeletionQueue_.pushFunction([this]()
	{
		vkDestroyImageView(vd.device_, drawImage_.imageView, nullptr);
		vmaDestroyImage(allocator_, drawImage_.image, drawImage_.allocation);

		vkDestroyImageView(vd.device_, depthImage_.imageView, nullptr);
		vmaDestroyImage(allocator_, depthImage_.image, depthImage_.allocation);
	}, "Destroy Draw and Depth Images");
}

void VkEngine::DestroySwapchain() const
{
	if (swapchain_ != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vd.device_, swapchain_, nullptr);
	}

	for (auto swapchainImageView : swapchainImageViews_)
	{
		vkDestroyImageView(vd.device_, swapchainImageView, nullptr);
	}
}

#pragma endregion Swapchain Management

#pragma region Command Pool

void VkEngine::InitCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo = VkInfo::CommandPoolInfo(graphicsQueueFamily_,
	                                                                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(vd.device_, &commandPoolInfo, nullptr, &immCommandPool_));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = VkInfo::CommandBufferAllocateInfo(immCommandPool_, 1);

	VK_CHECK(vkAllocateCommandBuffers(vd.device_, &cmdAllocInfo, &immCommandBuffer_));

	mainDeletionQueue_.pushFunction([=]()
	{
		vkDestroyCommandPool(vd.device_, immCommandPool_, nullptr); 
	}, "Destroying Command Pool");

	tracyContext_ = TracyVkContext(vd.physicalDevice_, vd.device_, graphicsQueue_, immCommandBuffer_);
}

#pragma endregion Command Pool

void VkEngine::InitializeCommandPoolsAndBuffers()
{
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		FrameData& frame = frames_[i];

		VkCommandPoolCreateInfo poolInfo = VkInfo::CommandPoolInfo(graphicsQueueFamily_,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		if (vkCreateCommandPool(vd.device_, &poolInfo, nullptr, &frame.commandPool_) != VK_SUCCESS)
		{
			LOG(ERR, "Failed to create command pool");
			return;
		}

		VkCommandBufferAllocateInfo cmdAllocInfo = VkInfo::CommandBufferAllocateInfo(frame.commandPool_, 1);

		if (vkAllocateCommandBuffers(vd.device_, &cmdAllocInfo, &frame.mainCommandBuffer_) != VK_SUCCESS)
		{
			LOG(ERR, "Failed to allocate command buffers");
			return;
		}
	}
}

#pragma region Fence

void VkEngine::InitSyncStructures()
{
	// Create synchronization structures:
	// - One fence to control when the GPU has finished rendering the frame.
	// - Two semaphores to synchronize rendering with the swapchain.
	// We want the fence to start signaled so we can wait on it on the first frame.
	VkFenceCreateInfo fenceCreateInfo = VkInfo::FenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = VkInfo::SemaphoreInfo(0);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(vd.device_, &fenceCreateInfo, nullptr, &frames_[i].renderFence_));

		VK_CHECK(vkCreateSemaphore(vd.device_, &semaphoreCreateInfo, nullptr, &frames_[i].swapChainSemaphore_));
		VK_CHECK(vkCreateSemaphore(vd.device_, &semaphoreCreateInfo, nullptr, &frames_[i].renderSemaphore_));
	}

	VK_CHECK(vkCreateFence(vd.device_, &fenceCreateInfo, nullptr, &immFence_));
	mainDeletionQueue_.pushFunction([=]()
	{
		vkDestroyFence(vd.device_, immFence_, nullptr);
	},"Destroying Fence");
}

void VkEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(vd.device_, 1, &immFence_));
	VK_CHECK(vkResetCommandBuffer(immCommandBuffer_, 0));

	VkCommandBuffer cmd = immCommandBuffer_;

	VkCommandBufferBeginInfo cmdBeginInfo = VkInfo::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = VkInfo::CommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = VkInfo::SubmitInfo(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submit, immFence_));

	VK_CHECK(vkWaitForFences(vd.device_, 1, &immFence_, true, 9999999999));
}


#pragma endregion Fence

#pragma region Image

AllocatedImage VkEngine::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage
	{
		.imageExtent = size,
		.imageFormat = format
	};

	VkImageCreateInfo imgInfo = VkInfo::ImageInfo(format, usage, size);
	if (mipmapped)
	{
		imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo
	{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	VK_CHECK(vmaCreateImage(allocator_, &imgInfo, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT)
	{
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build an image-view for the image
	VkImageViewCreateInfo viewInfo = VkInfo::ImageViewInfo(format, newImage.image, aspectFlag);
	viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

	VK_CHECK(vkCreateImageView(vd.device_, &viewInfo, nullptr, &newImage.imageView));

	return newImage;

}

AllocatedImage VkEngine::CreateImageData(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t dataSize = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadBuffer = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadBuffer.info.pMappedData, data, dataSize);

	AllocatedImage newImage = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	ImmediateSubmit([&](VkCommandBuffer cmd)
	{
		TransitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion
		{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageExtent = size
		};

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		TransitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	DestroyBuffer(uploadBuffer);

	return newImage;
}

void VkEngine::DestroyImage(const AllocatedImage& img)
{
	vkDestroyImageView(vd.device_, img.imageView, nullptr);
	vmaDestroyImage(allocator_, img.image, img.allocation);
}

void VkEngine::CreateImageWithVMA(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags memoryPropertyFlags, VkImage& image, VmaAllocation& allocation) const
{
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = memoryPropertyFlags;

	VK_CHECK(vmaCreateImage(allocator_, &imageInfo, &allocInfo, &image, &allocation, nullptr));
}


void VkEngine::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
								VkExtent2D dstSize)
{
	VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

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

	VkBlitImageInfo2 blitInfo{
		.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
		.pNext = nullptr,
		.srcImage = source,
							  .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							  .dstImage = destination,
							  .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							  .regionCount = 1,
							  .pRegions = &blitRegion,
							  .filter = VK_FILTER_LINEAR};

	vkCmdBlitImage2(cmd, &blitInfo);
}

void VkEngine::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) const
{
	ZoneScoped;
	VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	imageBarrier.pNext = nullptr;

	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange = ImageSubresourceRange(aspectMask);
	imageBarrier.image = image;

	VkDependencyInfo depInfo {};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}


VkImageSubresourceRange VkEngine::ImageSubresourceRange(VkImageAspectFlags aspectMask)
{
	return VkImageSubresourceRange
	{
		.aspectMask = aspectMask,
		// Specifies which aspects of the image are included in the view (e.g., color, depth, stencil).
		.baseMipLevel = 0, // The first mipmap level accessible to the view.
		.levelCount = VK_REMAINING_MIP_LEVELS,
		// Specifies the number of mipmap levels (VK_REMAINING_MIP_LEVELS to include all levels).
		.baseArrayLayer = 0, // The first array layer accessible to the view.
		.layerCount = VK_REMAINING_ARRAY_LAYERS
		// Specifies the number of array layers (VK_REMAINING_ARRAY_LAYERS to include all layers).
	};
}
#pragma endregion Image

#pragma region Buffer

AllocatedBuffer VkEngine::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	// allocate buffer
	VkBufferCreateInfo bufferInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = allocSize,
		.usage = usage
	};

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer{};

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
		&newBuffer.info));
	// vmaSetAllocationName(allocator_, newBuffer.allocation, "MyBufferAllocation");
	//
	// // // Assert to check for uninitialized buffer handle
	// assert(newBuffer.buffer != VK_NULL_HANDLE && "Buffer creation failed: Buffer handle is VK_NULL_HANDLE");
	// assert(newBuffer.buffer != reinterpret_cast<VkBuffer>(0xcccccccccccccccc) && "Buffer creation failed: Buffer handle is uninitialized (0xcccccccccccccccc)");

	return newBuffer;
}

void* VkEngine::MapBuffer(const AllocatedBuffer& buffer)
{
	void* data;
	vmaMapMemory(allocator_, buffer.allocation, &data);
	return data;
}

void VkEngine::UnmapBuffer(const AllocatedBuffer& buffer)
{
	vmaUnmapMemory(allocator_, buffer.allocation);
}

void VkEngine::DestroyBuffer(const AllocatedBuffer& buffer) const
{
	vmaDestroyBuffer(allocator_, buffer.buffer, buffer.allocation);
}

void VkEngine::CleanupAlloc()
{
	vmaDestroyAllocator(allocator_);
}

VkDeviceAddress VkEngine::GetBufferDeviceAddress(VkBuffer buffer) const
{
    VkBufferDeviceAddressInfo deviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer
    };
    return vkGetBufferDeviceAddress(vd.device_, &deviceAddressInfo);
}

GPUMeshBuffers VkEngine::UploadMesh(std::span<u32> indices, std::span<Vertex> vertices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(u32);
    const size_t stagingBufferSize = vertexBufferSize + indexBufferSize;

    GPUMeshBuffers newSurface{};

    // Create vertex buffer on GPU
    newSurface.vertexBuffer = CreateBuffer(vertexBufferSize,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);
    newSurface.vertexBufferAddress = GetBufferDeviceAddress(newSurface.vertexBuffer.buffer);

    // Create index buffer on GPU
    newSurface.indexBuffer = CreateBuffer(indexBufferSize,
                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VMA_MEMORY_USAGE_GPU_ONLY);

    // Create staging buffer on CPU (host-visible)
    AllocatedBuffer stagingBuffer = CreateBuffer(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // Map staging buffer to CPU address space
    void* mappedData = nullptr;
    VK_CHECK(vmaMapMemory(allocator_, stagingBuffer.allocation, &mappedData));

    // Copy vertex and index data to staging buffer
    memcpy(mappedData, vertices.data(), vertexBufferSize);
    memcpy(static_cast<char*>(mappedData) + vertexBufferSize, indices.data(), indexBufferSize);

    // Unmap the staging buffer
    vmaUnmapMemory(allocator_, stagingBuffer.allocation);

    // Submit copy commands to transfer data from staging buffer to GPU buffers
    ImmediateSubmit([&](const VkCommandBuffer cmd) {
        // Copy vertex data from staging buffer to GPU vertex buffer
        VkBufferCopy vertexCopy{ 0, 0, vertexBufferSize };
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        // Copy index data from staging buffer to GPU index buffer
        VkBufferCopy indexCopy{ vertexBufferSize, 0, indexBufferSize };
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    });

    // Clean up staging buffer
    DestroyBuffer(stagingBuffer);

    return newSurface;
}

#pragma endregion Buffer

#pragma region Pipelines

void VkEngine::InitPipelines()
{
	InitBackgroundPipelines();

	InitMeshPipeline();

	metalRoughMaterial.BuildPipelines(this, vd.device_);
}
void VkEngine::InitBackgroundPipelines()
{
	// CreatePipelineLayoutInfo();

	VkPipelineLayoutCreateInfo computeLayout
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext                  = nullptr,
		.flags                  = 0, // default value
		.setLayoutCount         = 1,
		.pSetLayouts            = &drawImageDescriptorLayout_,
		.pushConstantRangeCount = 0, // default value, adjust if needed
		.pPushConstantRanges    = nullptr // default value, adjust if needed
	};

	VkPushConstantRange pushConstant
	{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset     = 0,
		.size       = sizeof(ComputePushConstants)
	};

	computeLayout.pPushConstantRanges    = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(vd.device_, &computeLayout, nullptr, &gradientPipelineLayout_));

	VkShaderModule gradientShader;
	if (!loader_.LoadShader("shaders/gradient_color.comp.spv", vd.device_, &gradientShader)) {
		LOG(ERR, "Error when building the compute shader");
	}

	VkShaderModule skyShader;
	if (!loader_.LoadShader("shaders/sky.comp.spv", vd.device_, &skyShader)) {
		LOG(ERR,"Error when building the compute shader");
	}

	VkPipelineShaderStageCreateInfo gradientStageInfo = VkInfo::PipelineShaderStageInfo(VK_SHADER_STAGE_COMPUTE_BIT,
		gradientShader);
	VkPipelineShaderStageCreateInfo skyStageInfo = VkInfo::PipelineShaderStageInfo(VK_SHADER_STAGE_COMPUTE_BIT,
		skyShader);

	VkComputePipelineCreateInfo gradientPipelineCreateInfo = VkInfo::ComputePipelineInfo(gradientStageInfo,
		gradientPipelineLayout_);
	VkComputePipelineCreateInfo skyPipelineCreateInfo = VkInfo::ComputePipelineInfo(skyStageInfo,
		gradientPipelineLayout_);

	gradient.name	= "gradient";
	gradient.layout = gradientPipelineLayout_;
	gradient.data	= {glm::vec4(1, 0, 0, 1), glm::vec4(0, 0, 1, 1)};
	VK_CHECK(vkCreateComputePipelines(vd.device_, VK_NULL_HANDLE, 1, &gradientPipelineCreateInfo, nullptr,
									  &gradient.pipeline));

	sky.name   = "sky";
	sky.layout = gradientPipelineLayout_;
	sky.data   = {glm::vec4(0.1, 0.2, 0.4, 0.97)};
	VK_CHECK(vkCreateComputePipelines(vd.device_, VK_NULL_HANDLE, 1, &skyPipelineCreateInfo, nullptr, &sky.pipeline));

	backgroundEffects.push_back(gradient);
	backgroundEffects.push_back(sky);

	// destroy structures properly
	vkDestroyShaderModule(vd.device_, gradientShader, nullptr);
	vkDestroyShaderModule(vd.device_, skyShader, nullptr);

	mainDeletionQueue_.pushFunction(
		[&]()
		{
			if (gradientPipelineLayout_ != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(vd.device_, gradientPipelineLayout_, nullptr);
				gradientPipelineLayout_ = VK_NULL_HANDLE;  // Set to null after destruction
			}

			if (sky.pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(vd.device_, sky.pipeline, nullptr);
				sky.pipeline = VK_NULL_HANDLE;	// Set to null after destruction
			}

			if (gradient.pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(vd.device_, gradient.pipeline, nullptr);
				gradient.pipeline = VK_NULL_HANDLE;	 // Set to null after destruction
			}
		},
		"Destroying Background Pipeline");
}

void VkEngine::InitMeshPipeline()
{
    VkShaderModule triangleFragShader;
    if (!loader_.LoadShader("shaders/texImg.frag.spv", vd.device_, &triangleFragShader))
    {
        LOG(ERR, "Error when building the triangle fragment shader module");
    }

    VkShaderModule triangleVertexShader;
    if (!loader_.LoadShader("shaders/coloredTriangleMesh.vert.spv", vd.device_, &triangleVertexShader))
    {
        LOG(ERR, "Error when building the triangle vertex shader module");
    }

    VkPushConstantRange bufferRange
    {
    	.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0,
        .size       = sizeof(GPUDrawPushConstants)
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = VkInfo::CreatePipelineLayoutInfo(1, &singleImageDescriptorLayout_,
    	1, &bufferRange);
    VK_CHECK(vkCreatePipelineLayout(vd.device_, &pipelineLayoutInfo, nullptr, &meshPipelineLayout_));

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.data.config.layout = meshPipelineLayout_;
    pipelineBuilder
        .SetShaders(triangleVertexShader, triangleFragShader)
        .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetPolygonMode(VK_POLYGON_MODE_FILL)
        .SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .SetMultisamplingNone()
        .DisableBlending()
        .EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetColorAttachmentFormat(drawImage_.imageFormat)
        .SetDepthFormat(depthImage_.imageFormat);
		// .EnableBlendingAdditive();

    meshPipeline_ = pipelineBuilder.BuildPipeline(vd.device_, pipelineBuilder.data);

	// Destroy shader modules immediately
	vkDestroyShaderModule(vd.device_, triangleFragShader, nullptr);
	vkDestroyShaderModule(vd.device_, triangleVertexShader, nullptr);

    mainDeletionQueue_.pushFunction([&]()
    {
        vkDestroyPipelineLayout(vd.device_, meshPipelineLayout_, nullptr);
        vkDestroyPipeline(vd.device_, meshPipeline_, nullptr);
		},
		"Mesh Pipeline");
}

#pragma endregion Pipelines

#pragma region Descriptor

void VkEngine::InitDescriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<VkDescriptor::PoolSizeRatio> sizes =
	{
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
	};

	globalDescriptorAllocator.Init(vd.device_, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		drawImageDescriptorLayout_ = builder.Build(vd.device_, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// Single-Image sampler layout for the mesh draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		singleImageDescriptorLayout_ = builder.Build(vd.device_, VK_SHADER_STAGE_FRAGMENT_BIT);
	}


	//allocate a descriptor set for our draw image
	drawImageDescriptors_ = globalDescriptorAllocator.Allocate(vd.device_, drawImageDescriptorLayout_);

	VkDescriptorWriter writer;
	writer.WriteImage(0, drawImage_.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(vd.device_, drawImageDescriptors_);

	// Create a descriptor set layout with a single uniform buffer binding
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		gpuSceneDataDescriptorLayout_ = builder.Build(vd.device_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		// create a descriptor pool
		std::vector<VkDescriptor::PoolSizeRatio> frameSizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		frames_[i].frameDescriptors_ = VkDescriptor{};
		frames_[i].frameDescriptors_.Init(vd.device_, 1000, frameSizes);

		mainDeletionQueue_.pushFunction([&, i]() {
			frames_[i].frameDescriptors_.DestroyPools(vd.device_); }, "Descrptor Pools");
	}
}

#pragma endregion Descriptor

#pragma region Draw

void VkEngine::DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	TracyVkZone(tracyContext_, cmd, "Draw imgui")
	VkRenderingAttachmentInfo colorAttachment = VkInfo::RenderAttachmentInfo(targetImageView, nullptr,
	                                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = VkInfo::RenderInfo(swapchainExtent_, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void VkEngine::DrawBackground(VkCommandBuffer cmd)
{
	TracyVkZone(tracyContext_, cmd, "Draw Background")
	ComputeEffect& effect = backgroundEffects[currentBackgroundEffect_];

	// Bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// Bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout_, 0, 1, &drawImageDescriptors_, 0, nullptr);

	vkCmdPushConstants(cmd, gradientPipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);

	// Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, static_cast<u32>(std::ceil(drawExtent_.width / 16.0)), static_cast<u32>(std::ceil(drawExtent_.height / 16.0)), 1);
}

void VkEngine::DrawGeometry(VkCommandBuffer cmd)
{
    TracyVkZone(tracyContext_, cmd, "Draw Geometry");

    // Allocate a uniform buffer for the scene data
    AllocatedBuffer gpuSceneDataBuffer = CreateBuffer(
        sizeof(GPUSceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Add the buffer to the current frame's deletion queue
    GetCurrentFrame().deletionQueue_.pushFunction([=, this]() {
        DestroyBuffer(gpuSceneDataBuffer);
    });

    // Map the uniform buffer memory to CPU address space and copy scene data
	MapBuffer(gpuSceneDataBuffer);
    vmaUnmapMemory(allocator_, gpuSceneDataBuffer.allocation);

    // Create and update descriptor set for scene data buffer
    VkDescriptorSet globalDescriptor = GetCurrentFrame().frameDescriptors_.Allocate(
        vd.device_,
        gpuSceneDataDescriptorLayout_
    );

    VkDescriptorWriter writer;
    writer.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.UpdateSet(vd.device_, globalDescriptor);

    // Prepare rendering attachments for color and depth
    VkRenderingAttachmentInfo colorAttachment = VkInfo::RenderAttachmentInfo(drawImage_.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depthAttachment = VkInfo::DepthAttachmentInfo(depthImage_.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = VkInfo::RenderInfo(drawExtent_, &colorAttachment, &depthAttachment);

    // Begin rendering
    vkCmdBeginRendering(cmd, &renderInfo);

    // Bind the mesh pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline_);

    // Set dynamic viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = drawExtent_.width;
    viewport.height = drawExtent_.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = drawExtent_.width;
    scissor.extent.height = drawExtent_.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind a texture (error checkerboard image as a fallback)
    VkDescriptorSet imageSet = GetCurrentFrame().frameDescriptors_.Allocate(vd.device_, singleImageDescriptorLayout_);
    {
        VkDescriptorWriter imgWrite;
        imgWrite.WriteImage(0, errorCheckerboardImage_.imageView, defaultSamplerNearest_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        imgWrite.UpdateSet(vd.device_, imageSet);
    }
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout_, 0, 1, &imageSet, 0, nullptr);

    // Calculate aspect ratio
    float aspectRatio = static_cast<float>(drawExtent_.width) / static_cast<float>(drawExtent_.height > 0 ? drawExtent_.height : 1);

    // Set up view and projection matrices (with Y-axis flip for Vulkan)
    glm::mat4 view = glm::translate(cameraPosition);
    glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    projection[1][1] *= -1; // Flip Y-axis for Vulkan

	// Select the mesh and surface to draw (e.g., draw the first mesh and its first surface)
	std::shared_ptr<MeshAsset> mesh = testMeshes[2];  // Change the index to whichever mesh you want to draw
	GeoSurface& surface = mesh->surfaces[0]; // Drawing the first surface of the mesh

	// Apply a transformation to the selected mesh
	glm::mat4 model = glm::translate(glm::vec3{ 0.0f, 0.0f, 0.0f });  // Adjust the translation as needed

	// Set push constants (model, view, projection)
	GPUDrawPushConstants pushConstants{};
	pushConstants.worldMatrix = projection * view * model;
	pushConstants.vertexBuffer = mesh->meshBuffers.vertexBufferAddress;

	vkCmdPushConstants(cmd, meshPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

	// Bind the index buffer and draw the selected mesh and surface
	vkCmdBindIndexBuffer(cmd, mesh->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, surface.count, 1, surface.startIndex, 0, 0);

	// End rendering
	vkCmdEndRendering(cmd);
}


void VkEngine::InitDefaultData()
{
    ZoneScoped;

    // Load basic mesh
	testMeshes = loader_.LoadGltfMeshes(this, "Models\\basicmesh.glb").value();

    // Create 1x1 default textures (white, grey, black)
    {
        static u32 white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        whiteImage_ = CreateImageData(&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        static u32 grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        greyImage_ = CreateImageData(&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        static u32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        blackImage_ = CreateImageData(&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
    }

    // Create checkerboard error texture
    {
        static u32 magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        static u32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));

        SafeArray<u32, static_cast<size_t>(16 * 16)> pixels{};
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        errorCheckerboardImage_ = CreateImageData(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
    }

    // Create samplers (nearest and linear)
    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        if (vkCreateSampler(vd.device_, &samplerInfo, nullptr, &defaultSamplerNearest_) != VK_SUCCESS)
        {
            LOG(ERR, "Failed to create nearest sampler");
        }

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        if (vkCreateSampler(vd.device_, &samplerInfo, nullptr, &defaultSamplerLinear_) != VK_SUCCESS)
        {
            LOG(ERR, "Failed to create linear sampler");
        }
    }

	{
		GLTFMetallicRoughness::MaterialResources materialResources
		{
			.colorImage = whiteImage_,
			.colorSampler = defaultSamplerLinear_,
			.metalRoughImage = whiteImage_,
			.metalRoughSampler = defaultSamplerLinear_
		};

        // Allocate buffer for material constants (color and metallic-roughness)
        AllocatedBuffer materialConstants = CreateBuffer(
            sizeof(GLTFMetallicRoughness::MaterialConstants),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        // Map and write material constant data to buffer
        void* mappedData = nullptr;
        if (vmaMapMemory(allocator_, materialConstants.allocation, &mappedData) == VK_SUCCESS)
        {
            auto* sceneUniformData = static_cast<GLTFMetallicRoughness::MaterialConstants*>(mappedData);
            sceneUniformData->colorFactors = glm::vec4(1, 1, 1, 1);  // White
            sceneUniformData->metalRoughnessFactors = glm::vec4(1, 0.5f, 0, 0);  // Default metallic-roughness

            vmaUnmapMemory(allocator_, materialConstants.allocation);
        }

        // Add material constants buffer to deletion queue for cleanup
        mainDeletionQueue_.pushFunction([=, this]()
        {
            DestroyBuffer(materialConstants); 
		} ,"Destroting buffer for material constants");

        // Set buffer for material resources
        materialResources.dataBuffer = materialConstants.buffer;
        materialResources.dataBufferOffset = 0;

        // Write material resources to create descriptor set and initialize default material instance
        defaultData = metalRoughMaterial.WriteMaterial(
            vd.device_,
            MaterialPass::MainColor,
            materialResources,
            globalDescriptorAllocator
        );
    }

    // Add final destruction callbacks for all resources
    mainDeletionQueue_.pushFunction([=, this]()
    {
        vkDestroySampler(vd.device_, defaultSamplerNearest_, nullptr);
        vkDestroySampler(vd.device_, defaultSamplerLinear_, nullptr);

        DestroyImage(whiteImage_);
        DestroyImage(greyImage_);
        DestroyImage(blackImage_);
        DestroyImage(errorCheckerboardImage_);
		},
		"Destroying Images");
}

void VkEngine::UpdateScene()
{
	mainDrawContext.OpaqueSurfaces.clear();

	loadNodes["Suzanne"]->Draw(glm::mat4{1.0f}, mainDrawContext);

	sceneData.view = glm::translate(glm::vec3{0, 0, -5});
	sceneData.proj = glm::perspective(glm::radians(70.0f),
		static_cast<float>(swapchainExtent_.width) / static_cast<float>(swapchainExtent_.height),
		0.1f, 10000.0f);

	sceneData.proj[1][1] *= -1;  // Flip Y-axis for GLTF compatibility
	sceneData.viewproj = sceneData.proj * sceneData.view;

	sceneData.ambientColor = glm::vec4(0.1f);
	sceneData.sunlightColor = glm::vec4(1.0f);
	sceneData.sunlightDirection = glm::vec4(0, 1, 0.5f, 1.0f);
}


void VkEngine::Draw()
{
	TracyVkZone(tracyContext_, immCommandBuffer_, "Frame Start");
	// UpdateScene();
    VK_CHECK(vkWaitForFences(vd.device_, 1, &GetCurrentFrame().renderFence_, true, 1000000000));
    GetCurrentFrame().deletionQueue_.Flush();
	GetCurrentFrame().frameDescriptors_.ClearPools(vd.device_);

    VK_CHECK(vkResetFences(vd.device_, 1, &GetCurrentFrame().renderFence_));

    u32 swapchainImageIndex;
	VkResult e = vkAcquireNextImageKHR(vd.device_, swapchain_, 1000000000, GetCurrentFrame().swapChainSemaphore_, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resizeRequested_ = true;
		return ;
	}
    VkCommandBuffer cmd = GetCurrentFrame().mainCommandBuffer_;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = VkInfo::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	drawExtent_.height = static_cast<u32>(std::min(static_cast<float>(swapchainExtent_.height), static_cast<float>(drawImage_.imageExtent.height)) * renderScale);
	drawExtent_.width = static_cast<u32>(std::min(static_cast<float>(swapchainExtent_.width), static_cast<float>(drawImage_.imageExtent.width)) * renderScale);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Transition draw image to GENERAL layout for compute shader
	TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Draw the background using compute shader
	DrawBackground(cmd);

	// Transition draw image for color attachment
	TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Transition depth image for depth attachment
	TransitionImage(cmd, depthImage_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	// Draw geometry
	DrawGeometry(cmd);

	// Transition the draw image to TRANSFER_SRC_OPTIMAL layout for copying
	TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Transition the swapchain image to TRANSFER_DST_OPTIMAL layout for copying
	TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy image from drawImage_.image to swapchainImages_[swapchainImageIndex]
	CopyImageToImage(cmd, drawImage_.image, swapchainImages_[swapchainImageIndex], drawExtent_, swapchainExtent_);

	// Transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL layout for rendering ImGui
	TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Draw the ImGui interface
	DrawImGui(cmd, swapchainImageViews_[swapchainImageIndex]);

	// Transition the swapchain image to PRESENT_SRC_KHR layout for presentation
	TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = VkInfo::CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = VkInfo::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, GetCurrentFrame().swapChainSemaphore_);
    VkSemaphoreSubmitInfo signalInfo = VkInfo::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore_);

    VkSubmitInfo2 submit = VkInfo::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submit, GetCurrentFrame().renderFence_));
	TracyVkCollect(tracyContext_, GetCurrentFrame().mainCommandBuffer_);

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

	VkResult presentResult = vkQueuePresentKHR(graphicsQueue_, &presentInfo_);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resizeRequested_ = true;
	}
    frameNumber_++;
	FrameMark;
}

#pragma endregion Draw

#endif