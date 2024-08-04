//
// Created by Orgest on 7/27/2024.
//

#include "VulkanMaterials.h"

using namespace GraphicsAPI::Vulkan;


void VkMaterials::BuildPipelines(VkEngine* engine)
{
	// Load shaders
	VkShaderModule meshFragShader;
	if (!load->LoadShader("../../shaders/mesh.frag.spv", vd->device_, &meshFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
		return;
	}

	VkShaderModule meshVertexShader;
	if (!load->LoadShader("../../shaders/mesh.vert.spv", vd->device_, &meshVertexShader)) {
		fmt::println("Error when building the triangle vertex shader module");
		return;
	}

	// Define push constant range
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Build descriptor layout
	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	gLTFMetallicRoughness.materialLayout = layoutBuilder.Build(vd->device_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { // NOLINT(*-avoid-c-arrays)
		engine->gpuSceneDataDescriptorLayout_,
		gLTFMetallicRoughness.materialLayout
	};

	// Create pipeline layout
	VkPipelineLayoutCreateInfo mesh_layout_info = engine->CreatePipelineLayoutInfo();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(vd->device_, &mesh_layout_info, nullptr, &newLayout));

	gLTFMetallicRoughness.opaquePipeline.layout = newLayout;
	gLTFMetallicRoughness.transparentPipeline.layout = newLayout;

	// Build the pipeline
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	pipelineBuilder.SetColorAttachmentFormat(engine->swapchainImageFormat_);
	pipelineBuilder.SetDepthFormat(engine->depthImage_.imageFormat);
	pipelineBuilder.data.config.layout = newLayout;

	// Build opaque pipeline
	gLTFMetallicRoughness.opaquePipeline.pipeline = pipelineBuilder.BuildPipeline(vd->device_, pipelineBuilder.data);

	// Build transparent pipeline
	pipelineBuilder.EnableBlendingAdditive();
	pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
	gLTFMetallicRoughness.transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(vd->device_, pipelineBuilder.data);

	// Destroy shaders
	vkDestroyShaderModule(vd->device_, meshFragShader, nullptr);
	vkDestroyShaderModule(vd->device_, meshVertexShader, nullptr);
}
