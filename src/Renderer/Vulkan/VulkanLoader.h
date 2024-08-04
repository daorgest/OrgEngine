//
// Created by Orgest on 7/18/2024.
//
#pragma once
#include "VulkanHeader.h"
#include "../ResourceLoader.h"
#define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>


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

	class VkLoader : public ResourceLoader
	{
	public:
		bool LoadShader(const std::filesystem::path &filePath, VkDevice device, VkShaderModule *outShaderModule);
		std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(
			VkEngine *engine,const std::filesystem::path &filePath);
	private:
		void loadIndices(const fastgltf::Accessor& indexAccessor, std::vector<u32>& indices,
			const std::vector<Vertex>& vertices, const fastgltf::Asset& gltf);
		void loadVertices(const fastgltf::Accessor& posAccessor, std::vector<Vertex>& vertices, const fastgltf::Asset& gltf);
		void loadAttribute(const fastgltf::Primitive &primitive, const std::string &attributeName,
						   const fastgltf::Asset								 &gltf,
						   const std::function<void(const fastgltf::Accessor &)> &accessorFunc);
	};
} // namespace GraphicsAPI::Vulkan
