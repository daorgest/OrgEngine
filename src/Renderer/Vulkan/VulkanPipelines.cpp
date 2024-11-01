#include "VulkanPipelines.h"
#include "VulkanMain.h"

using namespace GraphicsAPI::Vulkan;

void PipelineBuilder::Clear(PipelineData& data)
{
	data.config.inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
	};

	data.config.rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
	};

	data.config.colorBlendAttachment = {};

	data.config.multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
	};

	data.config.layout = VK_NULL_HANDLE;

	data.config.depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
	};

	data.config.renderInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO
	};
	data.shaderStages.stages.clear();
}

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device, const PipelineData& data)
{
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &data.config.colorBlendAttachment
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
    	.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    	.pNext = &data.config.renderInfo,
    	.stageCount = static_cast<u32>(data.shaderStages.stages.size()),
    	.pStages = data.shaderStages.stages.data(),
    	.pVertexInputState = &vertexInputInfo,
    	.pInputAssemblyState = &data.config.inputAssembly,
    	.pViewportState = &viewportState,
    	.pRasterizationState = &data.config.rasterizer,
    	.pMultisampleState = &data.config.multisampling,
    	.pDepthStencilState = &data.config.depthStencil,
    	.pColorBlendState = &colorBlending,
    	.layout = data.config.layout,
    };

	VkDynamicState state[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = &state[0]
	};

	pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
		nullptr, &pipeline) != VK_SUCCESS)
	{
		LOG(ERR, "Failed to create pipeline");
		return VK_NULL_HANDLE;
	}
	return pipeline;
}

PipelineBuilder& PipelineBuilder::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
    data.shaderStages.stages.clear();
    data.shaderStages.stages.push_back(
        VkInfo::PipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    data.shaderStages.stages.push_back(
        VkInfo::PipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
	return *this;
}

PipelineBuilder& PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
{
    data.config.inputAssembly.topology = topology;
    data.config.inputAssembly.primitiveRestartEnable = VK_FALSE;
	return *this;
}

// Raster states
PipelineBuilder& PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
    data.config.rasterizer.polygonMode = mode;
    data.config.rasterizer.lineWidth = 1.0f;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    data.config.rasterizer.cullMode = cullMode;
    data.config.rasterizer.frontFace = frontFace;
	return *this;
}

// multisample state
PipelineBuilder& PipelineBuilder::SetMultisamplingNone()
{
    data.config.multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };
	return *this;
}

PipelineBuilder& PipelineBuilder::DisableBlending()
{
    data.config.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    data.config.colorBlendAttachment.blendEnable = VK_FALSE;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
{
    data.config.colorAttachmentFormat = format;
    data.config.renderInfo.colorAttachmentCount = 1;
    data.config.renderInfo.pColorAttachmentFormats = &data.config.colorAttachmentFormat;
	return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthFormat(VkFormat format)
{
    data.config.renderInfo.depthAttachmentFormat = format;
	return *this;
}

PipelineBuilder& PipelineBuilder::DisableDepthTest()
{
    data.config.depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_NEVER,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableDepthTest(bool depthWriteEnable, VkCompareOp op)
{
	data.config.depthStencil = {
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = depthWriteEnable,
		.depthCompareOp = op,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableBlendingAdditive()
{
	data.config.colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	return *this;
}

PipelineBuilder& PipelineBuilder::EnableBlendingAlphaBlend()
{
	data.config.colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	return *this;
}

PipelineBuilder& PipelineBuilder::Layout(VkPipelineLayout& layout)
{
	data.config.layout = layout;
	return *this;
}

