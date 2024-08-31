//
// Created by Orgest on 7/27/2024.
//

#ifndef VULKANMATERIALS_H
#define VULKANMATERIALS_H

#include "VulkanDescriptor.h"
#include "VulkanLoader.h"
#include "VulkanPipelines.h"

namespace GraphicsAPI::Vulkan {
	class VkEngine;
}

namespace GraphicsAPI::Vulkan
{

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
		MaterialPipeline* pipeline;
		VkDescriptorSet materialSet;
		MaterialPass passType;
	};

	class GLTFMetallicRoughness : public VkLoader
	{
	public:
		MaterialPipeline opaquePipeline{};
		MaterialPipeline transparentPipeline{};

		VkDescriptorSetLayout materialLayout{};

		struct MaterialConstants
		{
			glm::vec4 colorFactors;
			glm::vec4 metalRoughnessFactors;
			// padding, we need it anyway for uniform buffers
			SafeArray<glm::vec4, 14> extra;
		};

		struct MaterialResources
		{
			AllocatedImage colorImage;
			VkSampler colorSampler;
			AllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			VkBuffer dataBuffer;
			u32 dataBufferOffset;
		};

		VkDescriptorWriter writer;

		void BuildPipelines(VkEngine* engine, VkDevice device);
		void ClearResources(VkDevice device);

		MaterialInstance WriteMaterial
		(
			VkDevice device,
			MaterialPass pass,
			const MaterialResources& resources,
			DescriptorAllocatorGrowable& descriptorAllocator
		);
	};
}
#endif //VULKANMATERIALS_H
