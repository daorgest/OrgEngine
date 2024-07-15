//
// Created by Orgest on 7/5/2024.
//

#ifndef VULKANPIPELINES_H
#define VULKANPIPELINES_H
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
namespace GraphicsAPI::Vulkan
{
	struct ShaderStages
	{
		std::vector<VkPipelineShaderStageCreateInfo> stages;
	};

	struct PipelineConfig
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineLayout layout;
		VkPipelineDepthStencilStateCreateInfo depthStencil;
		VkPipelineRenderingCreateInfo renderInfo;
		VkFormat colorAttachmentFormat;
	};

	struct PipelineData
	{
		ShaderStages shaderStages;
		PipelineConfig config{};
	};

	// debug stuff
	inline size_t getCurrentPipelineDataSize()
	{
		return sizeof(PipelineData);
	}


	class PipelineBuilder
	{
	public:
		PipelineData data;

		PipelineBuilder() { Clear(data); }

		static void Clear(PipelineData& data);

		VkPipeline BuildPipeline(VkDevice device, const PipelineData& data);
		void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
		void SetInputTopology(VkPrimitiveTopology topology);
		void SetPolygonMode(VkPolygonMode mode);
		void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void SetMultisamplingNone();
		void DisableBlending();
		void SetColorAttachmentFormat(VkFormat format);
		void SetDepthFormat(VkFormat format);
		void DisableDepthTest();
	};
}



#endif //VULKANPIPELINES_H
