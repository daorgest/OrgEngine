//
// Created by Orgest on 4/12/2024.
//
#pragma once

#include "../../Core/PrimTypes.h"
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include "../../Platform/PlatformWindows.h"
#endif
// Vulkan Includes
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "backends/imgui_impl_vulkan.h"
#include "glm/glm.hpp"

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            LOG(ERR, "Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

namespace GraphicsAPI::Vulkan
{
	constexpr unsigned int FRAME_OVERLAP = 2;

	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function)
		{
			deletors.emplace_back(std::move(function));
		}

		void flush()
		{
			// Reverse iterate the deletion queue to execute all the functions
			for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
			{
				(*it)(); // Call functors
			}

			// Clear the deque by swapping with an empty deque
			std::deque<std::function<void()>>().swap(deletors);
		}
	};



	struct ComputePushConstants
	{
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	};

	struct ComputeEffect {
		const char* name;

		VkPipeline pipeline;
		VkPipelineLayout layout;

		ComputePushConstants data;
	};

	struct DescriptorLayoutBuilder
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		void AddBinding(u32 binding, VkDescriptorType type);
		void Clear();
		VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr,
		                            VkDescriptorSetLayoutCreateFlags flags = 0);
	};

	struct DescriptorAllocator
	{
		struct PoolSizeRatio
		{
			VkDescriptorType type;
			float ratio;
		};

		VkDescriptorPool pool;

		void InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
		void ClearDescriptors(VkDevice device);
		void DestroyPool(VkDevice device);

		VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
	};

	struct AllocatedImage
	{
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D imageExtent;
		VkFormat imageFormat;
	};

	struct FrameData
	{
		VkSemaphore swapChainSemaphore_, renderSemaphore_;
		VkFence renderFence_;
		VkCommandPool commandPool_;
		VkCommandBuffer mainCommandBuffer_;
		DeletionQueue deletionQueue_;
	};

	struct VulkanData
	{
		VkInstance instance_{VK_NULL_HANDLE};
		VkDebugUtilsMessengerEXT dbgMessenger_{VK_NULL_HANDLE};
		VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
		VkDevice device_{VK_NULL_HANDLE};
		VkSurfaceKHR surface_{VK_NULL_HANDLE};
	};

	class VkEngine
	{
	public:
		bool isInit = false;

		explicit VkEngine(Platform::WindowContext* winManager);
		~VkEngine();

		void Run(); // New method declaration
		void ImGuiMainMenu();
		void InitImgui();
		void Init();
		bool stopRendering_ = false; // New member variable

		void Cleanup();

		void SetupDebugMessenger();
		void InitVulkan();
		void InitCommands();
		void InitSwapChain();
		void InitializeCommandPoolsAndBuffers();
		void InitDescriptors();
		void InitPipelines();
		static VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
		                                                              VkShaderModule shaderModule);
		VkComputePipelineCreateInfo ComputePipelineCreateInfo(VkPipelineShaderStageCreateInfo shaderStage,
		                                                      VkPipelineLayout layout);
		VkPipelineLayoutCreateInfo CreatePipelineLayoutInfo();
		void InitBackgroundPipelines();
		void CreateSwapchain(u32 width, u32 height);
		static void CreateSurfaceWin32(HINSTANCE hInstance, HWND hwnd, VulkanData& vd);
		VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count);
		VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags);
		VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
		VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
		VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
		                         VkSemaphoreSubmitInfo* waitSemaphoreInfo);
		VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags) const;
		VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags) const;
		void InitSyncStructures();
		VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
		VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
		void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
		                      VkExtent2D dstSize);
		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
		VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask) const;
		bool LoadShader(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
		VkRenderingInfo RenderInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment,
		                           VkRenderingAttachmentInfo* depthAttachment);
		VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);
		void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView);
		void DrawBackground(VkCommandBuffer cmd);
		void DrawGeometry(VkCommandBuffer cmd);
		void Draw();

		// Swapchain management
		void DestroySwapchain() const;

		int frameNumber_{0};
		FrameData frames_[FRAME_OVERLAP]{};
		FrameData& GetCurrentFrame() { return frames_[frameNumber_ % FRAME_OVERLAP]; };

		VkQueue graphicsQueue_{};
		uint32_t graphicsQueueFamily_{};

		// Swapchain properties
		VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
		VkFormat swapchainImageFormat_{};
		std::vector<VkImage> swapchainImages_;
		std::vector<VkImageView> swapchainImageViews_;
		VkExtent2D swapchainExtent_{};

		// Draw resources
		AllocatedImage drawImage_{};
		VkExtent2D drawExtent_{};

		DescriptorAllocator globalDescriptorAllocator{};

		VkDescriptorSet drawImageDescriptors_{};
		VkDescriptorSetLayout drawImageDescriptorLayout_{};

		VkPipeline gradientPipeline_{};
		VkPipelineLayout gradientPipelineLayout_{};

		// Immediate GPU Commands
		VkFence immFence_{};
		VkCommandBuffer immCommandBuffer_{};
		VkCommandPool immCommandPool_{};

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		DeletionQueue mainDeletionQueue_;

	private:
		VulkanData vd;
		Platform::WindowContext* winManager_;
		std::vector<ComputeEffect> backgroundEffects;
		int currentBackgroundEffect_{0};

		// triangle
		VkPipelineLayout trianglePipelineLayout_{};
		VkPipeline trianglePipeline_{};
		void InitTrianglePipeline();


		// VkExtent2D windowSize
		// {
		//     .width = winManager_->GetWidth(),
		//     .height = winManager_->GetHeight()
		// };

		// Allocator for Vulkan memory
		VmaAllocator allocator_{};

		static void PrintAvailableExtensions();
		static std::string decodeDriverVersion(uint32_t driverVersion, uint32_t vendorID);
	};
}
