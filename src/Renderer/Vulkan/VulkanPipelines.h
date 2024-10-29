//
// Created by Orgest on 7/5/2024.
//

#pragma once
#ifdef VULKAN_BUILD
#include <vector>
#include <vulkan/vulkan.h>
namespace GraphicsAPI::Vulkan
{
	struct ShaderStages
	{
		std::vector<VkPipelineShaderStageCreateInfo> stages;
	};

	struct PipelineConfig
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		VkPipelineRenderingCreateInfo renderInfo = {};
		VkFormat colorAttachmentFormat = VK_FORMAT_UNDEFINED;
		VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
	};

	struct PipelineData
	{
		ShaderStages shaderStages;
		PipelineConfig config;
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

		VkPipeline       BuildPipeline(VkDevice device, const PipelineData& data);
		PipelineBuilder& SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
		PipelineBuilder& SetInputTopology(VkPrimitiveTopology topology);
		PipelineBuilder& SetPolygonMode(VkPolygonMode mode);
		PipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		PipelineBuilder& SetMultisamplingNone();
		PipelineBuilder& DisableBlending();
		PipelineBuilder& SetColorAttachmentFormat(VkFormat format);
		PipelineBuilder& SetDepthFormat(VkFormat format);
		PipelineBuilder& DisableDepthTest();
		PipelineBuilder& EnableDepthTest(bool depthWriteEnable, VkCompareOp op);
		PipelineBuilder& EnableBlendingAdditive();
		PipelineBuilder& EnableBlendingAlphaBlend();
		PipelineBuilder& Layout(VkPipelineLayout& layout);
	};
}
#endif