//
// Created by Orgest on 7/27/2024.
//

#include "VulkanMaterials.h"

#include "VulkanMain.h"

using namespace GraphicsAPI::Vulkan;


void GLTFMetallicRoughness::BuildPipelines(VkEngine* engine, VkDevice device)
{
    // Load vertex and fragment shaders
    VkShaderModule meshFragShader = VK_NULL_HANDLE;
    if (!LoadShader("shaders/mesh.frag.spv", device, &meshFragShader))
    {
        fmt::print("Error loading fragment shader module\n");
        return;
    }

    VkShaderModule meshVertexShader = VK_NULL_HANDLE;
    if (!LoadShader("shaders/mesh.vert.spv", device, &meshVertexShader))
    {
        fmt::print("Error loading vertex shader module\n");
        vkDestroyShaderModule(device, meshFragShader, nullptr); // Clean up previously loaded shader
        return;
    }

    // Define push constant range
    VkPushConstantRange matrixRange{
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

    // Create pipeline layout
    VkPipelineLayoutCreateInfo meshLayoutInfo = engine->CreatePipelineLayoutInfo();
    meshLayoutInfo.setLayoutCount = 2;
    meshLayoutInfo.pSetLayouts = layouts;
    meshLayoutInfo.pPushConstantRanges = &matrixRange;
    meshLayoutInfo.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &opaquePipeline.layout));
    transparentPipeline.layout = opaquePipeline.layout;  // Share layout between opaque and transparent pipelines

    // Build the pipelines
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
    pipelineBuilder.data.config.layout = opaquePipeline.layout;

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

MaterialInstance GLTFMetallicRoughness::WriteMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, VkDescriptor& descriptorAllocator)
{
	// Initialize the material instance with the correct pipeline based on pass type
	MaterialInstance matData{};
	matData.passType = pass;
	matData.pipeline = (pass == MaterialPass::Transparent) ? &transparentPipeline : &opaquePipeline;

	// Allocate descriptor set for the material
	matData.materialSet = descriptorAllocator.Allocate(device, materialLayout);

	// Ensure the descriptor set allocation succeeded
	if (matData.materialSet == VK_NULL_HANDLE)
	{
		fmt::print("Failed to allocate descriptor set for material\n");
		return matData; // Return empty/invalid MaterialInstance
	}

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

