//
// Created by Orgest on 7/18/2024.
//
#pragma once
#include "../ResourceLoader.h"
#include "VulkanHeader.h"

#include <fastgltf/tools.hpp>

namespace GraphicsAPI::Vulkan
{
	struct MaterialInstance;

	struct GeoSurface
	{
		u32 startIndex;
		u32 count;
		MaterialInstance* material;
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
		std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(VkEngine* engine,
									       const std::filesystem::path& filePath, bool OverrideColors = false);

	};
} // namespace GraphicsAPI::Vulkan
