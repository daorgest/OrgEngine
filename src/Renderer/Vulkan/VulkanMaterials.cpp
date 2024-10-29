//
// Created by Orgest on 7/27/2024.
//

#include "VulkanMaterials.h"

#include "VulkanMain.h"

using namespace GraphicsAPI::Vulkan;


void GLTFMetallicRoughness::BuildPipelines(VkEngine* engine, VkDevice device)
{
    // Load vertex and fragment shaders
    VkShaderModule meshFragShader;
    if (!LoadShader("shaders/mesh.frag.spv", device, &meshFragShader))
    {
        LOG(ERR, "Error loading fragment shader module");
        return;
    }

    VkShaderModule meshVertexShader;
    if (!LoadShader("shaders/mesh.vert.spv", device, &meshVertexShader))
    {
        LOG(ERR, "Error loading vertex shader module");
        vkDestroyShaderModule(device, meshFragShader, nullptr); // Clean up previously loaded shader
        return;
    }

    // Define push constant range
    VkPushConstantRange matrixRange
	{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants)
    };

    // Build descriptor layout
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {
        engine->gpuSceneDataDescriptorLayout_,
        materialLayout
    };

    // Create pipeline layout using newLayout
    VkPipelineLayoutCreateInfo meshLayoutInfo = VkInfo::CreatePipelineLayoutInfo(2, layouts, 1, &matrixRange);

    VkPipelineLayout newLayout; // Declare new layout
    VK_CHECK(vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &newLayout));

    opaquePipeline.layout = newLayout;  // Assign new layout
    transparentPipeline.layout = newLayout;  // Share layout between opaque and transparent pipelines

    // Build the pipelines
    PipelineBuilder pipelineBuilder;
	pipelineBuilder
		.SetShaders(meshVertexShader, meshFragShader)
		.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.SetPolygonMode(VK_POLYGON_MODE_FILL)
		.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
		.SetMultisamplingNone()
		.DisableBlending()
		.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
		.SetColorAttachmentFormat(engine->drawImage_.imageFormat)
		.SetDepthFormat(engine->depthImage_.imageFormat)
		.Layout(newLayout);

    // Build opaque pipeline
    opaquePipeline.pipeline = pipelineBuilder.BuildPipeline(device, pipelineBuilder.data);

    // Build transparent pipeline (with additive blending and no depth test)
    pipelineBuilder.EnableBlendingAdditive();
    pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
    transparentPipeline.pipeline = pipelineBuilder.BuildPipeline(device, pipelineBuilder.data);

    // Clean up shader modules
    vkDestroyShaderModule(device, meshFragShader, nullptr);
    vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

MaterialInstance GLTFMetallicRoughness::WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	// Initialize the material instance with the correct pipeline based on pass type
	MaterialInstance matData
	{
		.pipeline = (pass == MaterialPass::Transparent) ? &transparentPipeline : &opaquePipeline,
		.materialSet = descriptorAllocator.Allocate(device, materialLayout),
		.passType = pass
	};

	// Clear previous writes and prepare to write new resources
	writer.Clear();
	// Write the material's uniform buffer and images
	writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	// Update the descriptor set with the written resources
	writer.UpdateSet(device, matData.materialSet);

	return matData;
}

