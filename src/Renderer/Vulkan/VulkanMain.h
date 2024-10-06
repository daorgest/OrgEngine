//
// Created by Orgest on 4/12/2024.
//
#pragma once
#ifdef VULKAN_BUILD

#include "VulkanDescriptor.h"
#include "VulkanHeader.h"
#include "VulkanInitializers.h"
#include "VulkanLoader.h"
#include "VulkanMaterials.h"
#include "VulkanSceneNode.h"

// Vulkan Includes
#include <tracy/TracyVulkan.hpp>

namespace GraphicsAPI::Vulkan
{
	constexpr unsigned int FRAME_OVERLAP = 2;

	struct DeletionQueue
	{
		std::deque<std::pair<std::string, std::function<void()>>> deletors;

		void pushFunction(std::function<void()>&& function, const std::string& label = "")
		{
			deletors.emplace_back(label, std::move(function));
		}

		void Flush()
		{
			// Reverse iterate the deletion queue to execute all the functions
			for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
			{
				if (!it->first.empty()) std::cout << "Deleting: " << it->first << "\n";

				it->second();  // Call the function
			}

			// Clear the deque by swapping with an empty deque
			std::deque<std::pair<std::string, std::function<void()>>>().swap(deletors);
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
		VkSemaphore swapChainSemaphore_{}, renderSemaphore_{};
		VkFence renderFence_{};

		VkCommandPool commandPool_{};
		VkCommandBuffer mainCommandBuffer_{};

		DeletionQueue deletionQueue_;
		DescriptorAllocatorGrowable frameDescriptors_;
	};

	class VkEngine
	{
	public:
		explicit VkEngine(Platform::WindowContext* winManager, const std::wstring& renderName = L" - Vulkan");
		~VkEngine();

		void Run();
		void Cleanup();

		// Initialization
		bool Init();
		void SetupDebugMessenger();
		void InitVulkan();
		void InitCommands();
		void InitializeCommandPoolsAndBuffers();
		void InitSwapchain();
		void InitSyncStructures();
		void InitDescriptors();
		void InitPipelines();
		void InitBackgroundPipelines();
		void InitImgui();
		void InitMeshPipeline();
		void InitDefaultData();
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

		// Utility Functions
		void CreateImageWithVMA(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags memoryPropertyFlags,
		                        VkImage& image, VmaAllocation& allocation) const;
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
		                             VkExtent2D dstSize);
		AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void DestroyBuffer(const AllocatedBuffer& buffer) const;
		void CleanupAlloc();
		GPUMeshBuffers UploadMesh(std::span<u32> indices, std::span<Vertex> vertices);
		VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer) const;
		void* MapBuffer(const AllocatedBuffer& buffer);
		void UnmapBuffer(const AllocatedBuffer& buffer);
		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
		                     VkImageLayout newLayout) const;
		[[nodiscard]] static VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

		// Swapchain Management
		void CreateSwapchain(u32 width, u32 height);
		[[nodiscard]] VkExtent3D GetScreenResolution() const;
		void DestroySwapchain() const;

		// Textures
		AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		AllocatedImage CreateImageData(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
		                               bool mipmapped = false);
		void DestroyImage(const AllocatedImage& img);

		// Static Utility Functions
		static void CreateSurfaceWin32(HINSTANCE hInstance, HWND hwnd, VulkanData& vd);
		static void PrintAvailableExtensions();
		static std::string decodeDriverVersion(u32 driverVersion, u32 vendorID);

		// Draw resources
		AllocatedImage drawImage_{};
		AllocatedImage depthImage_{};
		AllocatedImage whiteImage_{};
		AllocatedImage blackImage_{};
		AllocatedImage greyImage_{};
		AllocatedImage errorCheckerboardImage_{};

		VkDescriptorSetLayout gpuSceneDataDescriptorLayout_{};
		VkFormat swapchainImageFormat_;

		DrawContext mainDrawContext;
		std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;
		void UpdateScene();


		[[nodiscard]] std::string GetGPUName() const { return deviceProperties.deviceName; }
		[[nodiscard]] float GetFPS() const { return fps_; }

		bool isInit = false;

	private:

		std::wstring renderName = L" - Vulkan";
		bool stopRendering_ = false;
		bool resizeRequested_ = false;
		bool windowShown = false;
		float renderScale = 1.0f;
		float spacing = 5.0f;
		float fps_ = 0.0f;
		float deltaTime_ = 0.0f;
		float lastTime_ = 0.0f;
		float timeSinceLastFPSUpdate_ = 0.0f;
		const float fpsUpdateInterval_ = 1.0f; // Update FPS every 1 second
		bool meshLoaded_ = false;

		// UI visibility flags
		bool showQuickStats_ = true;
		bool showMemoryUsage_ = true;
		bool modelDrawn = true;

		float aspectRatio = 16.0f / 9.0f;

		Platform::WindowContext* windowContext_;
		VulkanData vd;
		VmaAllocator allocator_;
		VkPhysicalDeviceProperties deviceProperties{};
		VkQueue graphicsQueue_{};
		u32 graphicsQueueFamily_{};

		std::vector<VkPresentModeKHR> availablePresentModes_;
		std::vector<std::string> presentModeNames_;
		int currentPresentModeIndex_ = 0;

		// Swapchain properties
		VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
		std::vector<VkImage> swapchainImages_;
		std::vector<VkImageView> swapchainImageViews_;
		VkExtent2D swapchainExtent_{};

		// Frame data
		FrameData frames_[FRAME_OVERLAP];
		FrameData& GetCurrentFrame() { return frames_[frameNumber_ % FRAME_OVERLAP]; }
		int frameNumber_{0};

		VkSampler defaultSamplerLinear_{};
		VkSampler defaultSamplerNearest_{};

		VkExtent2D drawExtent_{};

		// Descriptor-related members
		DescriptorAllocatorGrowable globalDescriptorAllocator{};
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

		// Materials
		MaterialInstance defaultData;
		GLTFMetallicRoughness metalRoughMaterial;

		// Camera and projection parameters
		glm::vec3 cameraPosition = glm::vec3(0, 0, -5);
		float fov = 70.0f;
		float nearPlane = 0.1f;
		float farPlane = 10000.0f;

		TracyVkCtx tracyContext_{};
		// Deletion queue
		DeletionQueue mainDeletionQueue_;

		ComputeEffect gradient{};
		ComputeEffect sky{};
	};
} // namespace GraphicsAPI::Vulkan

#endif