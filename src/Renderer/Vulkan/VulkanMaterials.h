//
// Created by Orgest on 7/27/2024.
//

#ifndef VULKANMATERIALS_H
#define VULKANMATERIALS_H

#include "VulkanDescriptor.h"
#include "VulkanMain.h"

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

	struct GLTFMetallicRoughness
	{
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
			uint32_t dataBufferOffset;
		};

		VkDescriptorWriter writer;

	};

	class VkMaterials
	{
	public:
		void BuildPipelines(VkEngine* engine);
		void ClearResources(VkDevice device);

		MaterialInstance WriteMaterial
		(
			VkDevice device,
			MaterialPass pass,
			const GLTFMetallicRoughness::MaterialResources& resources,
			DescriptorLayoutBuilder& descriptorAllocator
		);
	private:
		VulkanData *vd = nullptr;
		VkLoader *load = nullptr;
		GLTFMetallicRoughness gLTFMetallicRoughness;
	};
}
#endif //VULKANMATERIALS_H
