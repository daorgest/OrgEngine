//
// Created by Orgest on 7/18/2024.
//
#pragma once
#include <fastgltf/types.hpp>

#include "VulkanDescriptor.h"
#include "VulkanSceneNode.h"
#ifdef VULKAN_BUILD

#include "VulkanHeader.h"
#include "../ResourceLoader.h"

namespace GraphicsAPI::Vulkan
{
	struct MaterialInstance;

	enum class MaterialPass : u8
	{
		MainColor,
		Transparent,
		Other
	};

	struct MaterialPipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	struct MaterialInstance
	{
		MaterialPipeline* pipeline{ nullptr };   // Pointer to the associated pipeline
		VkDescriptorSet materialSet{ VK_NULL_HANDLE };   // Descriptor set for the material
		MaterialPass passType{};   // The type of material pass (MainColor, Transparent, etc.)
	};

	struct GLTFMaterial
	{
		MaterialInstance data;
	};


	struct GeoSurface
	{
		u32 startIndex;
		u32 count;
		std::shared_ptr<GLTFMaterial> material;
	};


	struct MeshAsset
	{
		std::string name;

		std::vector<GeoSurface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	// forward declaration
	class VkEngine;

	struct LoadedGLTF : IRenderable
	{
		// storage for all the data on a given glTF file
		std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
		std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
		std::unordered_map<std::string, AllocatedImage> images;
		std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

		std::vector<std::shared_ptr<Node>> topNodes;

		std::vector<VkSampler> samplers;

		DescriptorAllocatorGrowable descriptorPool;

		AllocatedBuffer materialDataBuffer;

		VkEngine* creator;

		~LoadedGLTF() override { ClearAll(); };

		void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
	private:
		void ClearAll();
	};


	class VkLoader : public ResourceLoader
	{
	public:
		bool LoadShader(const std::filesystem::path& filePath, VkDevice device, VkShaderModule* outShaderModule) const;
		static VkFilter ExtractFilter(fastgltf::Filter filter);
		static VkSamplerMipmapMode ExtractMipmapMode(fastgltf::Filter filter);
		static std::optional<std::shared_ptr<LoadedGLTF>> LoadGltfMeshes(VkEngine* engine,
		                                                                 const std::filesystem::path& filePath,
		                                                                 bool flipZAxis = true);
		static std::optional<AllocatedImage> LoadImage(VkEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);
	};
} // namespace GraphicsAPI::Vulkan
#endif