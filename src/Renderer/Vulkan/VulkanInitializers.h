//
// Created by Orgest on 9/23/2024.
//

#pragma once

#include <vulkan/vulkan_core.h>
#include "../../Core/PrimTypes.h"

namespace GraphicsAPI::Vulkan
{
	// you know all of those createInfos, yeah im putting them here lmao...
	class VkInfo
	{
	public:
		// Command pool / buffers
		static VkCommandPoolCreateInfo CommandPoolInfo(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags);
		static VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, u32 count);
		static VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags);
		static VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);

		// Fence
		static VkFenceCreateInfo FenceInfo(VkFenceCreateFlags flags);
		static VkSemaphoreCreateInfo SemaphoreInfo(VkSemaphoreCreateFlags flags);

		// Semaphore
		static VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
		static VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
									   VkSemaphoreSubmitInfo* waitSemaphoreInfo);

		// Images
		static VkImageCreateInfo ImageInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent,
			u32 mipLevels = 1);
		static VkImageViewCreateInfo ImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags,
										   VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

		// Pipelines
		static VkPipelineShaderStageCreateInfo PipelineShaderStageInfo(VkShaderStageFlagBits stage,
		                                                               VkShaderModule        shaderModule);
		static VkComputePipelineCreateInfo ComputePipelineInfo(const VkPipelineShaderStageCreateInfo& shaderStage,
		                                                       VkPipelineLayout                       layout);
		static VkPipelineLayoutCreateInfo CreatePipelineLayoutInfo(u32                          layoutCount,
		                                                           const VkDescriptorSetLayout* setLayouts,
		                                                           u32                          rangeCount   = 0,
		                                                           const VkPushConstantRange*   bufferRanges = nullptr);

		// Rendering Info
		static VkRenderingInfo RenderInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment,
		                                  VkRenderingAttachmentInfo* depthAttachment);
		static VkRenderingAttachmentInfo RenderAttachmentInfo(VkImageView   view, const VkClearValue* clear,
		                                                      VkImageLayout layout);
		static VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view, VkImageLayout layout);
	};
}
