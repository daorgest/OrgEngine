//
// Created by Orgest on 7/21/2024.
//

#include "VulkanDescriptor.h"
#include "VulkanHeader.h"

using namespace GraphicsAPI::Vulkan;

void DescriptorLayoutBuilder::AddBinding(u32 binding, VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding newbind
	{
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = 1
	};

	bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::Clear()
{
	bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext,
													 VkDescriptorSetLayoutCreateFlags flags)
{
	for (auto& b : bindings)
	{
		b.stageFlags |= shaderStages;
	}

	VkDescriptorSetLayoutCreateInfo info
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = pNext,
		.flags = flags,
		.bindingCount = static_cast<u32>(bindings.size()),
		.pBindings = bindings.data()
	};

	VkDescriptorSetLayout set;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

	return set;
}

void VkDescriptor::Init(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios)
{
    // Copy pool size ratios for future use
    ratios.assign(poolRatios.begin(), poolRatios.end());

    // Create the initial descriptor pool and store it
    VkDescriptorPool initialPool = CreatePool(device, maxSets, poolRatios);
    setsPerPool = static_cast<u32>(maxSets * 1.5);  // Grow pool size for future allocations
    readyPools.push_back(initialPool);
}

void VkDescriptor::ClearPools(VkDevice device)
{
    // Reset ready and full pools
    for (auto& pool : readyPools)
    {
        vkResetDescriptorPool(device, pool, 0);
    }
    for (auto& pool : fullPools)
    {
        vkResetDescriptorPool(device, pool, 0);
        readyPools.push_back(pool);  // Move them back to ready pools
    }
    fullPools.clear();
}

void VkDescriptor::DestroyPools(VkDevice device)
{
    // Destroy all descriptor pools
    for (auto& pool : readyPools)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    readyPools.clear();

    for (auto& pool : fullPools)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    fullPools.clear();
}

VkDescriptorPool VkDescriptor::GetPool(VkDevice device)
{
    if (!readyPools.empty())
    {
        VkDescriptorPool pool = readyPools.back();
        readyPools.pop_back();
        return pool;
    }
    // If no ready pools, create a new one
    VkDescriptorPool newPool = CreatePool(device, setsPerPool, ratios);
    setsPerPool = std::min(static_cast<u32>(setsPerPool * 1.5), 4092u);  // Ensure pool size grows but not beyond 4092
    return newPool;
}

VkDescriptorPool VkDescriptor::CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(poolRatios.size());

    // Create pool size descriptions
    for (const auto& ratio : poolRatios)
    {
        poolSizes.push_back({
            .type = ratio.type,
            .descriptorCount = static_cast<u32>(ratio.ratio * setCount)
        });
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = setCount,
        .poolSizeCount = static_cast<u32>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    VkDescriptorPool newPool;
    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool));
    return newPool;
}

VkDescriptorSet VkDescriptor::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    // Get or create a pool for allocation
    VkDescriptorPool pool = GetPool(device);

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        // Mark the current pool as full and retry with a new pool
        fullPools.push_back(pool);
        pool = GetPool(device);
        allocInfo.descriptorPool = pool;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    // Pool remains ready unless explicitly marked full
    readyPools.push_back(pool);
    return ds;
}

void VkDescriptorWriter::WriteBuffer(u32 binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    VkWriteDescriptorSet writeSet = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &bufferInfo
    };

    writes.push_back(writeSet);
}

void VkDescriptorWriter::WriteImage(u32 binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
    });

    VkWriteDescriptorSet writeSet
	{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &imageInfo
    };

    writes.push_back(writeSet);
}

void VkDescriptorWriter::Clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void VkDescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet& set)
{
    for (auto& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
}