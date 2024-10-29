//
// Created by Orgest on 7/27/2024.
//
#pragma once

#ifdef VULKAN_BUILD
#include "VulkanDescriptor.h"
#include "VulkanLoader.h"
#include "VulkanPipelines.h"

namespace GraphicsAPI::Vulkan {
	class VkEngine;
}

namespace GraphicsAPI::Vulkan
{
	// GLTFMaterial class handling the metallic-roughness material model
	class GLTFMetallicRoughness final : public VkLoader
	{
	public:
		GLTFMetallicRoughness() = default;

		// Holds pipelines for opaque and transparent materials
		MaterialPipeline opaquePipeline{};
		MaterialPipeline transparentPipeline{};

		MaterialInstance data;

		VkDescriptorSetLayout materialLayout{ VK_NULL_HANDLE };  // Descriptor set layout for material resources

		// Constants used in the material (uniform buffer)
		struct MaterialConstants
		{
			glm::vec4 colorFactors{};
			glm::vec4 metalRoughnessFactors{};
			glm::vec4 extra[14];
		};

		// Resources used by the material (images, samplers, and buffer)
		struct MaterialResources
		{
			AllocatedImage colorImage{};
			VkSampler colorSampler{ VK_NULL_HANDLE };
			AllocatedImage metalRoughImage{};
			VkSampler metalRoughSampler{ VK_NULL_HANDLE };
			VkBuffer dataBuffer{ VK_NULL_HANDLE };
			u32 dataBufferOffset{ 0 };
		};

		// Writer for updating descriptor sets
		VkDescriptorWriter writer{};

		// Builds the graphics pipelines for opaque and transparent materials
		void BuildPipelines(VkEngine* engine, VkDevice device);

		// Clears any Vulkan resources associated with the material
		void ClearResources(VkDevice device);

		// Writes material data to a descriptor set and returns a MaterialInstance
		MaterialInstance WriteMaterial(
		    VkDevice device,
		    MaterialPass pass,
		    const MaterialResources& resources,
		    DescriptorAllocatorGrowable& descriptorAllocator
		);
	};
}
#endif
