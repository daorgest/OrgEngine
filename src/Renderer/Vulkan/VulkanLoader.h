//
// Created by Orgest on 7/18/2024.
//
#pragma once

#include <filesystem>
#include "../../Core/PrimTypes.h"
#include "VulkanTypes.h"

namespace GraphicsAPI::Vulkan
{
	struct GeoSurface
	{
		u32 startIndex;
		u32 count;
	};

	struct MeshAsset
	{
		std::string name;

		std::vector<GeoSurface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	// forward declaration
	class VkEngine;

	class VkLoader
	{
	public:
		std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VkEngine *engine,
																			  const std::filesystem::path &filePath);
	};
} // namespace GraphicsAPI::Vulkan
