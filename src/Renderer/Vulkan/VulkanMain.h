//
// Created by Orgest on 4/12/2024.
//
#pragma once
#ifdef VULKAN_BUILD


// Vulkan Includes
#include "VulkanDescriptor.h"
#include "VulkanHeader.h"
#include "VulkanLoader.h"
#include "VulkanPipelines.h"

namespace GraphicsAPI::Vulkan
{
	constexpr unsigned int FRAME_OVERLAP = 2;

	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void pushFunction(std::function<void()> &&function) { deletors.emplace_back(std::move(function)); }

		void Flush()
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

	struct GPUSceneData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewproj;
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // w for sun power
		glm::vec4 sunlightColor;
	};

	struct ComputePushConstants
	{
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	};

	struct ComputeEffect
	{
		const char *name;

		VkPipeline pipeline;
		VkPipelineLayout layout;

		ComputePushConstants data;
	};

	struct FrameData
	{
		VkSemaphore swapChainSemaphore_, renderSemaphore_;
		VkFence renderFence_;

		VkCommandPool commandPool_;
		VkCommandBuffer mainCommandBuffer_;

		DeletionQueue deletionQueue_;
		VkDescriptor frameDescriptors_;
	};

	class VkEngine : public ResourceLoader
	{
	public:
		explicit VkEngine(Platform::WindowContext *winManager);
		~VkEngine() override;

		void Run();
		void Cleanup();

		// Initialization
		void Init();
		void SetupDebugMessenger();
		void InitVulkan();
		void InitCommands();
		void InitializeCommandPoolsAndBuffers();
		void InitSwapChain();
		void InitSyncStructures();
		void InitDescriptors();
		void InitPipelines();
		void InitBackgroundPipelines();
		void InitImgui();
		void InitMeshPipeline();
		void InitDefaultData();

		void CreateCheckerboardImage();
		void CreateSamplers();
		// Rendering
		void Draw();
		void ResizeSwapchain();
		void DrawBackground(VkCommandBuffer cmd);
		void DrawGeometry(VkCommandBuffer cmd);
		void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView);
		void RenderUI();
		void RenderMainMenu();
		void RenderMemoryUsageImGui();
		void RenderQuickStatsImGui();
		void ImGuiMainMenu();
		void UpdateFPS();

		// Utility Functions
		static VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
																			 VkShaderModule		   shaderModule);
		VkComputePipelineCreateInfo			   ComputePipelineCreateInfo(VkPipelineShaderStageCreateInfo shaderStage,
																		 VkPipelineLayout				 layout);
		VkPipelineLayoutCreateInfo CreatePipelineLayoutInfo();
		static VkCommandPoolCreateInfo CommandPoolCreateInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags);
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, u32 count);
		VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags);
		VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
		VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
		VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo,
								 VkSemaphoreSubmitInfo *waitSemaphoreInfo);
		VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags) const;
		VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags) const;
		VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
		VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
		void CreateImageWithVMA(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags memoryPropertyFlags,
								VkImage &image, VmaAllocation &allocation);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
							  VkExtent2D dstSize);
		AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void DestroyBuffer(const AllocatedBuffer &buffer);
		void CleanupAlloc();
		GPUMeshBuffers UploadMesh(std::span<u32> indices, std::span<Vertex> vertices);
		VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer) const;
		void LogMemoryUsage();
		void *MapBuffer(const AllocatedBuffer &buffer);
		void UnmapBuffer(const AllocatedBuffer &buffer);
		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
							 VkImageLayout newLayout) const;
		[[nodiscard]] VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask) const;
		static VkRenderingInfo RenderInfo(VkExtent2D extent, VkRenderingAttachmentInfo *colorAttachment,
								   VkRenderingAttachmentInfo *depthAttachment);
		VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue *clear, VkImageLayout layout);
		VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view, VkImageLayout layout);
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);

		// Swapchain Management
		void	   CreateSwapchain(u32 width, u32 height);
		[[nodiscard]] VkExtent3D getScreenResolution() const;
		void DestroySwapchain() const;

		// Textures
		AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		AllocatedImage CreateImageData(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
								   bool mipmapped = false);
		AllocatedImage CreateDefaultImage(const glm::vec4& color, const std::string& name);
		void DestroyImage(const AllocatedImage &img);

		// Static Utility Functions
		static void CreateSurfaceWin32(HINSTANCE hInstance, HWND hwnd, VulkanData &vd);
		static void PrintAvailableExtensions();
		static std::string decodeDriverVersion(u32 driverVersion, u32 vendorID);

		VkDescriptorSetLayout gpuSceneDataDescriptorLayout_{};
		VkFormat swapchainImageFormat_;
		AllocatedImage depthImage_{};

	private:
		bool isInit = false;
		bool stopRendering_ = false;
		bool resizeRequested = false;
		float renderScale = 1.0f;
		float spacing = 5.0f;
		float fps_ = 0.0f;
		bool meshLoaded_ = false;

		Platform::WindowContext* windowContext_;
		VkLoader* load{};
		VulkanData vd;
		VmaAllocator allocator_;
		VkQueue graphicsQueue_{};
		u32 graphicsQueueFamily_{};

		// Swapchain properties
		VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
		std::vector<VkImage> swapchainImages_;
		std::vector<VkImageView> swapchainImageViews_;
		VkExtent2D swapchainExtent_{};

		// Frame data
		FrameData frames_[FRAME_OVERLAP];
		FrameData& GetCurrentFrame() { return frames_[frameNumber_ % FRAME_OVERLAP]; }
		int frameNumber_{0};

		// Draw resources
		AllocatedImage drawImage_{};
		AllocatedImage whiteImage_{};
		AllocatedImage blackImage_{};
		AllocatedImage greyImage_{};
		AllocatedImage errorCheckerboardImage_{};

		VkSampler defaultSamplerLinear_{};
		VkSampler defaultSamplerNearest_{};

		VkExtent2D drawExtent_{};

		DescriptorAllocator globalDescriptorAllocator{};

		VkDescriptorSet drawImageDescriptors_{};
		VkDescriptorSetLayout drawImageDescriptorLayout_{};
		VkDescriptorSetLayout singleImageDescriptorLayout_{};
		VkPipeline gradientPipeline_{};
		VkPipelineLayout gradientPipelineLayout_{};
		GPUSceneData sceneData{};

		// Immediate GPU Commands
		VkFence immFence_{};
		VkCommandBuffer immCommandBuffer_{};
		VkCommandPool immCommandPool_{};

		// Mesh pipeline
		VkPipelineLayout meshPipelineLayout_;
		VkPipeline meshPipeline_;
		GPUMeshBuffers rectangle;

		// Triangle pipeline
		VkPipelineLayout trianglePipelineLayout_{};
		VkPipeline trianglePipeline_{};

		// Background effects
		std::vector<ComputeEffect> backgroundEffects;
		int currentBackgroundEffect_{0};

		// Asset loading
		VkLoader loader_;
		std::vector<std::shared_ptr<MeshAsset>> testMeshes;
		u8 meshSelector = testMeshes.size();

		// Camera and projection parameters
		glm::vec3 cameraPosition = glm::vec3(0, 0, -5);
		float fov = 70.0f;
		float nearPlane = 0.1f;
		float farPlane = 10000.0f;

		// Deletion queue
		DeletionQueue mainDeletionQueue_;
	};
} // namespace GraphicsAPI::Vulkan

#endif