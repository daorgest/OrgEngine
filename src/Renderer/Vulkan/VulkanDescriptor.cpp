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

void DescriptorAllocatorGrowable::Init(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios)
{
	ratios.clear();

	for (auto r : poolRatios) {
		ratios.push_back(r);
	}

	VkDescriptorPool newPool = CreatePool(device, maxSets, poolRatios);

	setsPerPool = maxSets * 1.5; //grow it next allocation

	readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::ClearPools(VkDevice device)
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

void DescriptorAllocatorGrowable::DestroyPools(VkDevice device)
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

VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice device)
{
	VkDescriptorPool newPool;
	if (!readyPools.empty())
	{
		newPool = readyPools.back();
		readyPools.pop_back();
	}
	else
	{
		// need to create a new pool
		newPool = CreatePool(device, setsPerPool, ratios);

		setsPerPool = setsPerPool * 1.5;
		if (setsPerPool > 4092)
		{
			setsPerPool = 4092;
		}
	}

	return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios)
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

VkDescriptorSet DescriptorAllocatorGrowable::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
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

// VkDescriptorWriter: Handles descriptor set writes
VkDescriptorWriter& VkDescriptorWriter::WriteBuffer(u32 binding, VkBuffer buffer, size_t size, size_t offset,
                                                    VkDescriptorType type) {
	bufferInfos.push_back({
		.buffer = buffer,
		.offset = offset,
		.range = size
	});

	writes.push_back({
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = &bufferInfos.back()
	});

	return *this;
}

VkDescriptorWriter& VkDescriptorWriter::WriteImage(u32 binding, VkImageView image, VkSampler sampler, VkImageLayout layout,
	VkDescriptorType type)
{
	imageInfos.push_back({ .sampler = sampler, .imageView = image, .imageLayout = layout });
	writes.push_back({
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = &imageInfos.back()
	});

	return *this;
}

void VkDescriptorWriter::Clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void VkDescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
    for (auto& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
}