//
// Created by Orgest on 10/6/2024.
//

#pragma once
#include "VulkanHeader.h"

namespace GraphicsAPI::Vulkan
{
	class VkImages
	{
	public:
		static AllocatedImage CreateImage(VkDevice device, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VmaAllocator allocator, bool mipmapped);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
		                             VkExtent2D dstSize);
		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
		                     VkImageLayout   newLayout) ;
		static VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);
		static void DestroyImage(const AllocatedImage& img, VkDevice device, VmaAllocator allocator);
		static void CreateImageWithVMA(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags memoryPropertyFlags,
		                                VkImage&                image,
		                                VmaAllocation&          allocation, const VmaAllocator& allocator);
	};
}