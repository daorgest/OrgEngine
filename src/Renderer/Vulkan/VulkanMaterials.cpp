//
// Created by Orgest on 7/27/2024.
//

#include "VulkanMaterials.h"

#include "VulkanMain.h"

using namespace GraphicsAPI::Vulkan;


void GLTFMetallicRoughness::BuildPipelines(VkEngine* engine, VkDevice device)
{
	// Load shaders
	VkShaderModule meshFragShader;
	if (!LoadShader("shaders/mesh.frag.spv", device, &meshFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
		return;
	}

	VkShaderModule meshVertexShader;
	if (!LoadShader("shaders/mesh.vert.spv", device, &meshVertexShader)) {
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

	materialLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { // NOLINT(*-avoid-c-arrays)
		engine->gpuSceneDataDescriptorLayout_,
		materialLayout
	};

	// Create pipeline layout
	VkPipelineLayoutCreateInfo mesh_layout_info = engine->CreatePipelineLayoutInfo();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &newLayout));

	opaquePipeline.layout = newLayout;
	transparentPipeline.layout = newLayout;

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
	opaquePipeline.pipeline = pipelineBuilder.BuildPipeline(device, pipelineBuilder.data);

	// Build transparent pipeline
	pipelineBuilder.EnableBlendingAdditive();
	pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
	transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(device, pipelineBuilder.data);

	// Destroy shaders
	vkDestroyShaderModule(device, meshFragShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

MaterialInstance GLTFMetallicRoughness::WriteMaterial(VkDevice device, MaterialPass pass,
													  const MaterialResources& resources,
													  DescriptorAllocatorGrowable& descriptorAllocator)
{
	MaterialInstance matData{};
	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	}
	else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.Allocate(device, materialLayout);

	writer.Clear();
	writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.UpdateSet(device, matData.materialSet);

	return matData;
}

