//
// Created by Orgest on 4/12/2024.
//

#include "VulkanMain.h"
#include "VkBootstrap.h"
#include "VulkanPipelines.h"

#include <vk_mem_alloc.h>
#include <fmt/color.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#ifndef NDEBUG
constexpr bool bUseValidationLayers = true;
#else
constexpr bool bUseValidationLayers = false;
#endif

using namespace GraphicsAPI::Vulkan;

VkEngine::VkEngine(Platform::WindowContext* winManager) : winManager_(winManager)
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

	while (!bQuit)
	{
		// Handle events on queue
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bQuit = true;
			}

			if (msg.message == WM_SYSCOMMAND)
			{
				if (msg.wParam == SC_MINIMIZE)
				{
					stopRendering_ = true;
				}
				else if (msg.wParam == SC_RESTORE)
				{
					stopRendering_ = false;
				}
			}
		}

		// do not draw if we are minimized
		if (stopRendering_)
		{
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
			if (ImGui::MenuItem("Toggle Fullscreen"))
			{
				winManager_->ToggleFullscreen(); // BREAKS FOR NOW LMAO

			}
			if (ImGui::MenuItem("Option 2"))
			{
				// Action for Option 2
			}

			// Background effect settings
			ImGui::Separator();
			ComputeEffect& selected = backgroundEffects[currentBackgroundEffect_];

			ImGui::Text("Selected effect: %s", selected.name);
			ImGui::SliderInt("Effect Index", &currentBackgroundEffect_, 0, backgroundEffects.size() - 1);
			ImGui::InputFloat4("data1", reinterpret_cast<float*>(&selected.data.data1));
			ImGui::InputFloat4("data2", reinterpret_cast<float*>(&selected.data.data2));
			ImGui::InputFloat4("data3", reinterpret_cast<float*>(&selected.data.data3));
			ImGui::InputFloat4("data4", reinterpret_cast<float*>(&selected.data.data4));

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

#pragma endregion Run

#pragma region Initialization

void VkEngine::Init()
{
	if (winManager_)
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
	}
}

void VkEngine::InitImgui()
{
	// Create descriptor pool for IMGUI
	VkDescriptorPoolSize pool_sizes[] = {
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

	ImGui_ImplWin32_Init(winManager_->hwnd);

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
	mainDeletionQueue_.push_function([=]()
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(vd.device_, imguiPool, nullptr);
	});
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

	if (!instRet)
	{
		LOG(ERR, "Failed to create Vulkan instance. Error: " + instRet.error().message());
		return;
	}

	vd.instance_ = instRet.value().instance;
	vd.dbgMessenger_ = instRet.value().debug_messenger;
	LOG(INFO, "Vulkan Instance and Debug Messenger created successfully.");

	CreateSurfaceWin32(winManager_->hInstance, winManager_->hwnd, vd);
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
		LOG(ERR, "Failed to select a Vulkan physical device. Error: " + physDeviceRet.error().message(), __LINE__);
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
	if (isInit)
	{
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
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
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(vd.device_, frames_[i].commandPool_, nullptr);
		}
		DestroySwapchain();
		vkDestroySurfaceKHR(vd.instance_, vd.surface_, nullptr);
		vkDestroyDevice(vd.device_, nullptr);

		vkb::destroy_debug_utils_messenger(vd.instance_, vd.dbgMessenger_);
		vkDestroyInstance(vd.instance_, nullptr);
		// if (winManager_)
		// 	winManager_->DestroyAppWindow();
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

	if (vkCreateWin32SurfaceKHR(vd.instance_, &createInfo, nullptr, &vd.surface_) != VK_SUCCESS)
	{
		MessageBox(nullptr, L"Failed to create Vulkan Surface!, Check to see if your drivers are installed properly",
		           L"GPU Error!", MB_OK);
		exit(1);
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
}

void VkEngine::InitSwapChain()
{
    // Destroy the old swapchain before creating a new one
    DestroySwapchain();

    // Recreate surface
    CreateSurfaceWin32(winManager_->hInstance, winManager_->hwnd, vd);

    CreateSwapchain(winManager_->screenWidth, winManager_->screenHeight);

    // draw image size will match the window
    VkExtent3D drawImageExtent = {
        winManager_->screenWidth,
        winManager_->screenHeight,
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
    VkImageViewCreateInfo rview_info = ImageViewCreateInfo(drawImage_.imageFormat, drawImage_.image,
                                                           VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(vd.device_, &rview_info, nullptr, &drawImage_.imageView));

    // add to deletion queues
    mainDeletionQueue_.push_function([this]()
    {
        vkDestroyImageView(vd.device_, drawImage_.imageView, nullptr);
        vmaDestroyImage(allocator_, drawImage_.image, drawImage_.allocation);
    });
}

void VkEngine::DestroySwapchain() const
{
	if (swapchain_ != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vd.device_, swapchain_, nullptr);
	}

	for (int i = 0; i < swapchainImageViews_.size(); i++)
	{
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

	mainDeletionQueue_.push_function([=]()
	{
		vkDestroyCommandPool(vd.device_, immCommandPool_, nullptr);
	});
}


void VkEngine::InitializeCommandPoolsAndBuffers()
{
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		FrameData& frame = frames_[i];

		VkCommandPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = graphicsQueueFamily_,
		};

		if (vkCreateCommandPool(vd.device_, &poolInfo, nullptr, &frame.commandPool_) != VK_SUCCESS)
		{
			LOG(ERR, "Failed to create command pool");
			return;
		}

		VkCommandBufferAllocateInfo cmdAllocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = frame.commandPool_,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		if (vkAllocateCommandBuffers(vd.device_, &cmdAllocInfo, &frame.mainCommandBuffer_) != VK_SUCCESS)
		{
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

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(vd.device_, &fenceCreateInfo, nullptr, &frames_[i].renderFence_));

		VK_CHECK(vkCreateSemaphore(vd.device_, &semaphoreCreateInfo, nullptr, &frames_[i].swapChainSemaphore_));
		VK_CHECK(vkCreateSemaphore(vd.device_, &semaphoreCreateInfo, nullptr, &frames_[i].renderSemaphore_));
	}

	VK_CHECK(vkCreateFence(vd.device_, &fenceCreateInfo, nullptr, &immFence_));
	mainDeletionQueue_.push_function([=]()
	{
		vkDestroyFence(vd.device_, immFence_, nullptr);
	});
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
		.components = {VK_COMPONENT_SWIZZLE_IDENTITY},
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

	VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
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
	// Determine default pipeline stages and access masks
	VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	// Optimize pipeline stages and access masks based on specific layout transitions
	if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		// Transition from undefined layout to transfer destination optimal layout
		srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		srcAccessMask = 0;
		dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	}
	else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout ==
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// Transition from transfer destination optimal layout to shader read-only optimal layout
		srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	}
	else if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout ==
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		// Transition from undefined layout to depth/stencil attachment optimal layout
		srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		srcAccessMask = 0;
		dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	// Create image memory barrier to manage image layout transition
	VkImageMemoryBarrier2 imageBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	imageBarrier.srcStageMask = srcStageMask;
	imageBarrier.srcAccessMask = srcAccessMask;
	imageBarrier.dstStageMask = dstStageMask;
	imageBarrier.dstAccessMask = dstAccessMask;
	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.image = image;
	imageBarrier.subresourceRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	// Create dependency info to include the image barrier
	VkDependencyInfo depInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	// Record pipeline barrier command in the specified command buffer
	vkCmdPipelineBarrier2(cmd, &depInfo);
}


VkImageSubresourceRange VkEngine::ImageSubresourceRange(VkImageAspectFlags aspectMask) const
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

	// // Assert to check for uninitialized buffer handle
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

void VkEngine::DestroyBuffer(const AllocatedBuffer& buffer)
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
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);
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

	InitTrianglePipeline();
	InitMeshPipeline();
}

VkPipelineShaderStageCreateInfo VkEngine::PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo stageInfo{};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.pNext = nullptr;
	stageInfo.stage = stage;
	stageInfo.module = shaderModule;
	stageInfo.pName = "main";

	return stageInfo;
}

VkComputePipelineCreateInfo VkEngine::ComputePipelineCreateInfo(VkPipelineShaderStageCreateInfo shaderStage, VkPipelineLayout layout) {
	VkComputePipelineCreateInfo computePipelineInfo = {};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = nullptr;
	computePipelineInfo.layout = layout;
	computePipelineInfo.stage = shaderStage;

	return computePipelineInfo;
}

VkPipelineLayoutCreateInfo VkEngine::CreatePipelineLayoutInfo()
{
	VkPipelineLayoutCreateInfo layout{};
	layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout.pNext = nullptr;
	layout.pSetLayouts = &drawImageDescriptorLayout_;
	layout.setLayoutCount = 1;

	return layout;
}
void VkEngine::InitBackgroundPipelines()
{
	CreatePipelineLayoutInfo();

	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &drawImageDescriptorLayout_;
	computeLayout.setLayoutCount = 1;

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(vd.device_, &computeLayout, nullptr, &gradientPipelineLayout_));

	VkShaderModule gradientShader;
	if (!LoadShader("shaders/gradient_color.comp.spv", vd.device_, &gradientShader)) {
		fmt::print("Error when building the compute shader \n");
	}

	VkShaderModule skyShader;
	if (!LoadShader("shaders/sky.comp.spv", vd.device_, &skyShader)) {
		fmt::print("Error when building the compute shader \n");
	}

	VkPipelineShaderStageCreateInfo gradientStageInfo = PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, gradientShader);
	VkPipelineShaderStageCreateInfo skyStageInfo = PipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, skyShader);

	VkComputePipelineCreateInfo gradientPipelineCreateInfo = ComputePipelineCreateInfo(gradientStageInfo, gradientPipelineLayout_);
	VkComputePipelineCreateInfo skyPipelineCreateInfo = ComputePipelineCreateInfo(skyStageInfo, gradientPipelineLayout_);

	ComputeEffect gradient{};
	gradient.layout = gradientPipelineLayout_;
	gradient.name = "gradient";
	gradient.data = {};
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(vd.device_, VK_NULL_HANDLE, 1, &gradientPipelineCreateInfo, nullptr, &gradient.pipeline));

	ComputeEffect sky{};
	sky.layout = gradientPipelineLayout_;
	sky.name = "sky";
	sky.data = {};
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(vd.device_, VK_NULL_HANDLE, 1, &skyPipelineCreateInfo, nullptr, &sky.pipeline));

	// add the 2 background effects into the array
	backgroundEffects.push_back(gradient);
	backgroundEffects.push_back(sky);

	// destroy structures properly
	vkDestroyShaderModule(vd.device_, gradientShader, nullptr);
	vkDestroyShaderModule(vd.device_, skyShader, nullptr);

	mainDeletionQueue_.push_function([&]()
	{
		vkDestroyPipelineLayout(vd.device_, gradientPipelineLayout_, nullptr);
		vkDestroyPipeline(vd.device_, sky.pipeline, nullptr);
		vkDestroyPipeline(vd.device_, gradient.pipeline, nullptr);

	});
}

void VkEngine::InitTrianglePipeline()
{
	VkShaderModule triangleFragShader;
	if (!LoadShader("shaders/coloredTriangle.frag.spv", vd.device_, &triangleFragShader)) {
		LOG(ERR, "Error when building the triangle fragment shader module");
	}

	VkShaderModule triangleVertexShader;
	if (!LoadShader("shaders/coloredTriangle.vert.spv", vd.device_, &triangleVertexShader)) {
		LOG(ERR, "Error when building the triangle vertex shader module");
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = CreatePipelineLayoutInfo();
	VK_CHECK(vkCreatePipelineLayout(vd.device_, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout_ ));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.data.config.layout = trianglePipelineLayout_;
	pipelineBuilder.SetShaders(triangleVertexShader, triangleFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.DisableDepthTest();

	pipelineBuilder.SetColorAttachmentFormat(drawImage_.imageFormat);
	pipelineBuilder.SetDepthFormat(VK_FORMAT_UNDEFINED);

	trianglePipeline_ = pipelineBuilder.BuildPipeline(vd.device_, pipelineBuilder.data);

	// clean structures
	vkDestroyShaderModule(vd.device_, triangleFragShader, nullptr);
	vkDestroyShaderModule(vd.device_, triangleVertexShader, nullptr);

	mainDeletionQueue_.push_function([&]() {
		vkDestroyPipelineLayout(vd.device_, trianglePipelineLayout_, nullptr);
		vkDestroyPipeline(vd.device_, trianglePipeline_, nullptr);
	});
}

void VkEngine::InitMeshPipeline()
{
	VkShaderModule triangleFragShader;
	if (!LoadShader("shaders/coloredTriangle.frag.spv", vd.device_, &triangleFragShader)) {
		LOG(ERR, "Error when building the triangle fragment shader module");
	}

	VkShaderModule triangleVertexShader;
	if (!LoadShader("shaders/coloredTriangleMesh.vert.spv", vd.device_, &triangleVertexShader)) {
		LOG(ERR, "Error when building the triangle vertex shader module");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = CreatePipelineLayoutInfo();
	pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(vd.device_, &pipelineLayoutInfo, nullptr, &meshPipelineLayout_));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.data.config.layout = meshPipelineLayout_;
	pipelineBuilder.SetShaders(triangleVertexShader, triangleFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.DisableDepthTest();

	pipelineBuilder.SetColorAttachmentFormat(drawImage_.imageFormat);
	pipelineBuilder.SetDepthFormat(VK_FORMAT_UNDEFINED);

	meshPipeline_ = pipelineBuilder.BuildPipeline(vd.device_, pipelineBuilder.data);

	// clean structures
	vkDestroyShaderModule(vd.device_, triangleFragShader, nullptr);
	vkDestroyShaderModule(vd.device_, triangleVertexShader, nullptr);

	mainDeletionQueue_.push_function([&]() {
		vkDestroyPipelineLayout(vd.device_, meshPipelineLayout_, nullptr);
		vkDestroyPipeline(vd.device_, meshPipeline_, nullptr);
	});
}

void VkEngine::InitDefaultData() {
	std::array<Vertex, 4> rectVertices{};

	rectVertices[0].position = {0.5,-0.5, 0};
	rectVertices[1].position = {0.5,0.5, 0};
	rectVertices[2].position = {-0.5,-0.5, 0};
	rectVertices[3].position = {-0.5,0.5, 0};

	rectVertices[0].color = {0,0, 0,1};
	rectVertices[1].color = { 0.5,0.5,0.5 ,1};
	rectVertices[2].color = { 1,0, 0,1 };
	rectVertices[3].color = { 0,1, 0,1 };

	std::array<u32, 6> rectIndices{};

	rectIndices[0] = 0;
	rectIndices[1] = 1;
	rectIndices[2] = 2;

	rectIndices[3] = 2;
	rectIndices[4] = 1;
	rectIndices[5] = 3;

	rectangle = UploadMesh(rectIndices, rectVertices);

	//delete the rectangle data on engine shutdown
	mainDeletionQueue_.push_function([&](){
		DestroyBuffer(rectangle.indexBuffer);
		DestroyBuffer(rectangle.vertexBuffer);
	});

}

#pragma endregion Pipelines

#pragma region Compute-Shaders

// Function to show error message in a MessageBox
void ShowErrorMessage(const std::string& message, const std::string& title = "Error") {
	MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

// Function to read file into a vector of uint32_t
std::vector<uint32_t> ReadFile(const std::string& filePath) {
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open shader file: " + filePath);
	}

	size_t fileSize = file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

bool VkEngine::LoadShader(const char* filePath, VkDevice device, VkShaderModule* outShaderModule) {
	try {
		std::vector<uint32_t> buffer = ReadFile(filePath);

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader for file: " + std::string(filePath));
		}

		*outShaderModule = shaderModule;
		return true;
	}
	catch (const std::exception& e) {
		ShowErrorMessage(e.what());
		return false;
	}
}

#pragma endregion Compute-Shaders

#pragma region Descriptor

void VkEngine::InitDescriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
	{
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
	};

	globalDescriptorAllocator.InitPool(vd.device_, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		drawImageDescriptorLayout_ = builder.Build(vd.device_, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our draw image
	drawImageDescriptors_ = globalDescriptorAllocator.Allocate(vd.device_, drawImageDescriptorLayout_);

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
	mainDeletionQueue_.push_function([&]()
	{
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
	for (auto& b : bindings)
	{
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
	for (PoolSizeRatio ratio : poolRatios)
	{
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
	vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout,
	};

	VkDescriptorSet ds;
	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

	return ds;
}

#pragma endregion Descriptor

#pragma region Render

VkRenderingInfo VkEngine::RenderInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment,
                                     VkRenderingAttachmentInfo* depthAttachment = nullptr)
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

VkRenderingAttachmentInfo VkEngine::AttachmentInfo(VkImageView view, VkClearValue* clear,
                                                   VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
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
	VkRenderingAttachmentInfo colorAttachment = AttachmentInfo(targetImageView, nullptr,
	                                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = RenderInfo(swapchainExtent_, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void VkEngine::DrawBackground(VkCommandBuffer cmd)
{
	ComputeEffect& effect = backgroundEffects[currentBackgroundEffect_];

	// Bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// Bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout_, 0, 1, &drawImageDescriptors_, 0, nullptr);

	vkCmdPushConstants(cmd, gradientPipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);

	// Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(drawExtent_.width / 16.0)), static_cast<uint32_t>(std::ceil(drawExtent_.height / 16.0)), 1);
}

void VkEngine::DrawGeometry(VkCommandBuffer cmd)
{
	VkRenderingAttachmentInfo colorAttachment = AttachmentInfo(drawImage_.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = RenderInfo(drawExtent_, &colorAttachment, nullptr);
	vkCmdBeginRendering(cmd, &renderInfo);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline_);

	//set dynamic viewport and scissor
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

	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);

	// Bind pipeline for the rectangle mesh
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline_);

	GPUDrawPushConstants push_constants{};
	push_constants.worldMatrix = glm::mat4{ 1.f };
	push_constants.vertexBuffer = rectangle.vertexBufferAddress;

	vkCmdPushConstants(cmd, meshPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

	vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	// Draw the indexed mesh (rectangle)
	vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

	testMeshes = loader_.loadGltfMeshes(this,"assets\\basicmesh.glb").value();

	push_constants.vertexBuffer = testMeshes[2]->meshBuffers.vertexBufferAddress;

	vkCmdPushConstants(cmd, meshPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, testMeshes[2]->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, testMeshes[2]->surfaces[0].count, 1, testMeshes[2]->surfaces[0].startIndex, 0, 0);

	vkCmdEndRendering(cmd);
}


void VkEngine::Draw()
{
    // Wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(vd.device_, 1, &GetCurrentFrame().renderFence_, true, 1000000000));
    GetCurrentFrame().deletionQueue_.flush();

    VK_CHECK(vkResetFences(vd.device_, 1, &GetCurrentFrame().renderFence_));

    u32 swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(vd.device_, swapchain_, 1000000000, GetCurrentFrame().swapChainSemaphore_, nullptr, &swapchainImageIndex));

    const VkCommandBuffer cmd = GetCurrentFrame().mainCommandBuffer_;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    drawExtent_.width = drawImage_.imageExtent.width;
    drawExtent_.height = drawImage_.imageExtent.height;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Transition draw image to GENERAL layout for compute shader
	TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Draw the background using compute shader
	DrawBackground(cmd);

	// Transition draw image to COLOR_ATTACHMENT_OPTIMAL layout for rendering
	TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Draw geometry
	DrawGeometry(cmd);

	// Transition the draw image to TRANSFER_SRC_OPTIMAL layout for copying
	TransitionImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Transition the swapchain image to TRANSFER_DST_OPTIMAL layout for copying
	TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy image from drawImage_.image to swapchainImages_[swapchainImageIndex]
	CopyImageToImage(cmd, drawImage_.image, swapchainImages_[swapchainImageIndex], drawExtent_, swapchainExtent_);

	// Transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL layout for rendering
	TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Draw the ImGui interface
	DrawImGui(cmd, swapchainImageViews_[swapchainImageIndex]);

	// Transition the swapchain image to PRESENT_SRC_KHR layout for presentation
	TransitionImage(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, GetCurrentFrame().swapChainSemaphore_);
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, GetCurrentFrame().renderSemaphore_);

    VkSubmitInfo2 submit = SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submit, GetCurrentFrame().renderFence_));

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

    frameNumber_++;
}



#pragma endregion Draw
