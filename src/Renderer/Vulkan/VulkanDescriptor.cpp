//
// Created by Orgest on 7/21/2024.
//

#include "VulkanDescriptor.h"

#include <vulkan/vk_enum_string_helper.h>

#include "VulkanHeader.h"

using namespace GraphicsAPI::Vulkan;

void VkDescriptor::Init(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios)
{
	ratios.assign(poolRatios.begin(), poolRatios.end());

	VkDescriptorPool newPool = CreatePool(device, maxSets, poolRatios);

	setsPerPool = static_cast<u32>(maxSets * 1.5); // Grow it for the next allocation

	readyPools.push_back(newPool);
}

void VkDescriptor::ClearPools(VkDevice device)
{
	for (const auto& pool : readyPools) {
		vkResetDescriptorPool(device, pool, 0);
	}

	for (const auto& pool : fullPools) {
		vkResetDescriptorPool(device, pool, 0);
		readyPools.push_back(pool);
	}

	fullPools.clear();
}

void VkDescriptor::DestroyPools(VkDevice device)
{
	for (const auto& pool : readyPools) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}
	readyPools.clear();

	for (const auto& pool : fullPools) {
		vkDestroyDescriptorPool(device, pool, nullptr);
	}
	fullPools.clear();
}

VkDescriptorPool VkDescriptor::GetPool(VkDevice device)
{
	VkDescriptorPool newPool;
	if (!readyPools.empty()) {
		newPool = readyPools.back();
		readyPools.pop_back();
	} else {
		// Create a new pool
		newPool = CreatePool(device, setsPerPool, ratios);

		setsPerPool = static_cast<u32>(setsPerPool * 1.5);
		if (setsPerPool > 4092) {
			setsPerPool = 4092;
		}
	}

	return newPool;
}


VkDescriptorPool VkDescriptor::CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(poolRatios.size());

	for (const auto& ratio : poolRatios)
	{
		poolSizes.push_back({
			.type = ratio.type,
			.descriptorCount = static_cast<u32>(ratio.ratio * setCount)
		});
	}

	VkDescriptorPoolCreateInfo poolInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = setCount,
		.poolSizeCount = static_cast<u32>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool);
	return newPool;
}

VkDescriptorSet VkDescriptor::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
	// Get or create a pool to allocate from
	VkDescriptorPool poolToUse = GetPool(device);

	VkDescriptorSetAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = pNext,
		.descriptorPool = poolToUse,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout
	};

	VkDescriptorSet ds;
	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

	// Allocation failed. Try again
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
	{
		fullPools.push_back(poolToUse);

		poolToUse = GetPool(device);
		allocInfo.descriptorPool = poolToUse;

		result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
		VK_CHECK(result);
	}

	readyPools.push_back(poolToUse);
	return ds;
}

void VkDescriptorWriter::WriteBuffer(u32 binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
	VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = offset,
		.range = size
	});

	VkWriteDescriptorSet write
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = &info
	};
	writes.push_back(write);
}

void VkDescriptorWriter::WriteImage(u32 binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
	VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
		.sampler = sampler,
		.imageView = image,
		.imageLayout = layout
	});

	VkWriteDescriptorSet write
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE, // Left empty for now until we need to write it
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = &info
	};

	writes.push_back(write);
}

void VkDescriptorWriter::Clear()
{
	imageInfos.clear();
	writes.clear();
	bufferInfos.clear();
}

void VkDescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
	for (VkWriteDescriptorSet& write : writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
}

