//
// Created by Orgest on 9/23/2024.
//

#include "VulkanInitializers.h"

using namespace GraphicsAPI::Vulkan;


VkCommandPoolCreateInfo VkInfo::CommandPoolInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.queueFamilyIndex = queueFamilyIndex
	};
}

VkCommandBufferAllocateInfo VkInfo::CommandBufferAllocateInfo(VkCommandPool pool, u32 count)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = count
	};
}

VkCommandBufferBeginInfo VkInfo::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = flags,
		.pInheritanceInfo = nullptr
	};
}

VkCommandBufferSubmitInfo VkInfo::CommandBufferSubmitInfo(VkCommandBuffer cmd)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = cmd,
		.deviceMask = 1
	};
}

VkFenceCreateInfo VkInfo::FenceInfo(VkFenceCreateFlags flags)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags
	};;
}

VkSemaphoreCreateInfo VkInfo::SemaphoreInfo(VkSemaphoreCreateFlags flags)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags
	};
}

VkSemaphoreSubmitInfo VkInfo::SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext = nullptr,
		.semaphore = semaphore,
		.value = 1,
		.stageMask = stageMask,
		.deviceIndex = 0,
	};
}

VkSubmitInfo2 VkInfo::SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
	VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = static_cast<u32>(waitSemaphoreInfo == nullptr ? 0 : 1),
		.pWaitSemaphoreInfos = waitSemaphoreInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = cmd,
		.signalSemaphoreInfoCount = static_cast<u32>(signalSemaphoreInfo == nullptr ? 0 : 1),
		.pSignalSemaphoreInfos = signalSemaphoreInfo
	};
}

VkImageCreateInfo VkInfo::ImageInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,  // No image creation flags by default
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // Assume single queue access
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED  // Start with undefined layout
	};
}


VkImageViewCreateInfo VkInfo::ImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags,
	VkImageViewType viewType)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.components = {VK_COMPONENT_SWIZZLE_IDENTITY},
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
}

VkPipelineShaderStageCreateInfo VkInfo::PipelineShaderStageInfo(VkShaderStageFlagBits stage,
	VkShaderModule shaderModule)
{
	return
	{
		.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext  = nullptr,
		.stage  = stage,
		.module = shaderModule,
		.pName  = "main"
	};
}

VkComputePipelineCreateInfo VkInfo::ComputePipelineInfo(const VkPipelineShaderStageCreateInfo& shaderStage,
                                                        VkPipelineLayout                       layout)
{
	return
	{
		.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext              = nullptr,
		.flags              = 0, // default value
		.stage              = shaderStage,
		.layout             = layout,
		.basePipelineHandle = VK_NULL_HANDLE, // default value
		.basePipelineIndex  = -1 // default value
	};
}

VkPipelineLayoutCreateInfo VkInfo::CreatePipelineLayoutInfo(u32 layoutCount, const VkDescriptorSetLayout* setLayouts,
	u32 rangeCount,
	const VkPushConstantRange* bufferRanges)
{
	return
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext                  = nullptr,
		.flags                  = 0,
		.setLayoutCount         = layoutCount,
		.pSetLayouts            = (layoutCount > 0) ? setLayouts : nullptr,
		.pushConstantRangeCount = rangeCount,
		.pPushConstantRanges    = (rangeCount > 0) ? bufferRanges : nullptr,
	};
}


VkRenderingInfo VkInfo::RenderInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment,
								   VkRenderingAttachmentInfo* depthAttachment)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext = nullptr,
		.flags = 0,  // No additional flags
		.renderArea = {
			.offset = {0, 0},
			.extent = extent
		},
		.layerCount = 1,
		.viewMask = 0,  // No multiview
		.colorAttachmentCount = 1,  // Assuming one color attachment
		.pColorAttachments = colorAttachment,
		.pDepthAttachment = depthAttachment,
		.pStencilAttachment = nullptr  // Assuming no stencil attachment
	};
}

VkRenderingAttachmentInfo VkInfo::RenderAttachmentInfo(VkImageView view, const VkClearValue* clear, VkImageLayout layout)
{
	VkRenderingAttachmentInfo colorAttachment =
	{
		.sType        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext        = nullptr,
		.imageView    = view,
		.imageLayout  = layout,
		.loadOp       = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp      = VK_ATTACHMENT_STORE_OP_STORE
	};

	if (clear)
	{
		colorAttachment.clearValue = *clear;
	}

	return colorAttachment;
}

VkRenderingAttachmentInfo VkInfo::DepthAttachmentInfo(VkImageView view, VkImageLayout layout)
{
	return
	{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = view,
		.imageLayout = layout,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {
			.depthStencil = {
				.depth = 0.f,
				.stencil = 0
			}
		}
	};
}
