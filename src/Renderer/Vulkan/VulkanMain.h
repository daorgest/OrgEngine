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
#include "../Camera.h"
#include "../../Core/InputHandler.h"

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

	struct EngineStats
	{
		float frametime;
		int triCout;
		int drawcallCount;
		float sceneUpdateTime;
		float meshDrawtime;

	};

	struct VRAMUsage
	{
		VkDeviceSize totalVRAM = 0;
		VkDeviceSize usedVRAM = 0;
		VkDeviceSize freeVRAM = 0;
		float usagePercentage = 0.0f;

		void Update()
		{
			freeVRAM = totalVRAM - usedVRAM;
			usagePercentage = (totalVRAM > 0) ? (static_cast<float>(usedVRAM) / totalVRAM) * 100.0f : 0.0f;
		}

		static std::string FormatSizeInGB(VkDeviceSize size)
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << static_cast<float>(size) / (1024.0f * 1024.0f * 1024.0f) << " GB";
			return oss.str();
		}
	};


	class VkEngine
	{
	public:
		explicit VkEngine(Platform::WindowContext* winManager, const std::wstring& renderName = L" - Vulkan");
		~VkEngine();

		void Run();
		void Cleanup();

		// Initialization
		bool        Init();
		void        SetupDebugMessenger();
		void        InitVulkan();
		void        InitCommands();
		void        InitializeCommandPoolsAndBuffers();
		void        InitSwapchain();
		void        InitSyncStructures();
		void        InitDescriptors();
		void        InitPipelines();
		void        InitBackgroundPipelines();
		void        InitImgui();
		void        InitMeshPipeline();
		void        InitDefaultData();
		static void InitImguiStyles();
		// Rendering
		void Draw();
		void DrawBackground(VkCommandBuffer cmd);
		void DrawGeometry(VkCommandBuffer cmd);
		void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView);
		void RenderUI();
		void RenderMainMenu() const;
		void RenderMemoryUsageImGui();
		void RenderSettingsImGui();
		void RenderQuickStatsImGui();

		// Utility Functions
		AllocatedBuffer        CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;
		void                   DestroyBuffer(const AllocatedBuffer& buffer) const;
		void                   CleanupAlloc();
		GPUMeshBuffers         UploadMesh(std::span<u32> indices, std::span<Vertex> vertices);
		static VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer);
		void*                  MapBuffer(const AllocatedBuffer& buffer);
		void                   UnmapBuffer(const AllocatedBuffer& buffer);
		void                   ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;
		static void            SetViewportAndScissor(VkCommandBuffer cmd, const VkExtent2D& extent);

		// Swapchain Management
		void CreateSwapchain(u32 width, u32 height);
		void ResizeSwapchain();
		[[nodiscard]] VkExtent3D GetScreenResolution() const;
		void DestroySwapchain() const;

		// Textures
		AllocatedImage CreateImageData(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);

		// More utility functions
		static void        CreateSurfaceWin32(HINSTANCE hInstance, HWND hwnd, VulkanData& vd);
		static void        PrintAvailableExtensions();
		static std::string decodeDriverVersion(u32 driverVersion, u32 vendorID);
		void               GetVRAMUsage(VkPhysicalDevice physicalDevice, VmaAllocator allocator, VRAMUsage& usage);

		void UpdateScene();

		VmaAllocator allocator_;

		// Draw resources
		AllocatedImage drawImage_{};
		AllocatedImage depthImage_{};
		AllocatedImage whiteImage_{};
		AllocatedImage blackImage_{};
		AllocatedImage greyImage_{};
		AllocatedImage errorCheckerboardImage_{};

		VkDescriptorSetLayout gpuSceneDataDescriptorLayout_{};
		VkFormat swapchainImageFormat_;

		VkSampler defaultSamplerLinear_{};
		VkSampler defaultSamplerNearest_{};

		// Materials
		MaterialInstance defaultData;
		GLTFMetallicRoughness metalRoughMaterial;

		DrawContext mainDrawContext;
		std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

		bool isInit = false;

	private:
		Platform::WindowContext* windowContext_;
		Camera camera_;

		// Vulkan core objects
		VkPhysicalDeviceProperties deviceProperties{};
		VkQueue graphicsQueue_{};
		u32 graphicsQueueFamily_{};
		VkSwapchainKHR swapchain_{VK_NULL_HANDLE};

		// Memory management
		DeletionQueue mainDeletionQueue_;

		// UI visibility flags
		bool showQuickStats_ = true;
		bool showMemoryUsage_ = true;
		bool modelDrawn = true;

		f32 aspectRatio = 16.0f / 9.0f; // default

		std::wstring renderName = L" - Vulkan";
		std::string gpuName;
		bool stopRendering_ = false;
		bool resizeRequested_ = false;
		f32 renderScale = 1.0f;


		// Timing and performance metrics
		float deltaTime = 0.0f, renderTime = 0.0f, displayedFPS = 0.0f, accumulatedTime = 0.0f;
		int frameCount = 0;
		EngineStats stats;
		VRAMUsage vramUsage;
		std::unordered_map<std::string, float> timingResults;

		std::vector<VkPresentModeKHR> availablePresentModes_;
		std::vector<std::string> presentModeNames_;
		int currentPresentModeIndex_ = 0;

		// Swapchain properties
		std::vector<VkImage> swapchainImages_;
		std::vector<VkImageView> swapchainImageViews_;
		VkExtent2D swapchainExtent_{};
		VkExtent2D drawExtent_{};

		// Frame data
		FrameData frames_[FRAME_OVERLAP];
		FrameData& GetCurrentFrame() { return frames_[frameNumber_ % FRAME_OVERLAP]; }
		int frameNumber_{0};

		// Descriptor-related members
		DescriptorAllocatorGrowable globalDescriptorAllocator{};
		VkDescriptorSet drawImageDescriptors_{};
		VkDescriptorSetLayout drawImageDescriptorLayout_{};
		VkDescriptorSetLayout singleImageDescriptorLayout_{};

		VkPipelineLayout gradientPipelineLayout_{};
		GPUSceneData sceneData{};

		// Immediate GPU Commands
		VkFence immFence_{};
		VkCommandBuffer immCommandBuffer_{};
		VkCommandPool immCommandPool_{};

		// Mesh pipeline
		VkPipelineLayout meshPipelineLayout_;
		VkPipeline meshPipeline_;

		// Background effects
		std::vector<ComputeEffect> backgroundEffects;
		int currentBackgroundEffect_{0};

		// Asset loading
		VkLoader loader_;
		std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

		std::vector<std::shared_ptr<MeshAsset>> testMeshes;
		u8 meshSelector = testMeshes.size();

		// Camera and projection parameters
		glm::vec3 cameraPosition = glm::vec3(0, 0, -5);
		f32 fov = 70.0f;
		f32 nearPlane = 0.1f;
		f32 farPlane = 10000.0f;

		TracyVkCtx tracyContext_{};

		ComputeEffect gradient{};
		ComputeEffect sky{};
	};
} // namespace GraphicsAPI::Vulkan

#endif