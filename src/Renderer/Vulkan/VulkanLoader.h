//
// Created by Orgest on 7/18/2024.
//
#pragma once
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

	class VkLoader : public ResourceLoader
	{
	public:
		bool LoadShader(const std::filesystem::path &filePath, VkDevice device, VkShaderModule *outShaderModule) const;
		static std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(VkEngine* engine,
									       const std::filesystem::path& filePath, bool OverrideColors = false,
									       bool flipZAxis = true);
	};
} // namespace GraphicsAPI::Vulkan
#endif