//
// Created by Orgest on 7/21/2024.
//

#ifndef VULKANDESCRIPTOR_H
#define VULKANDESCRIPTOR_H

#pragma once

#include <deque>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>
#include "../../Core/PrimTypes.h"

namespace GraphicsAPI::Vulkan
{
    // Descriptor Layout Builder: Builds descriptor set layouts.
    struct DescriptorLayoutBuilder
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        void AddBinding(u32 binding, VkDescriptorType type);
        void Clear();
        VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    };

    // Descriptor Writer: Handles writing descriptor sets.
    struct VkDescriptorWriter
    {
        std::deque<VkDescriptorImageInfo> imageInfos;
        std::deque<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> writes;

        void WriteImage(u32 binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
        void WriteBuffer(u32 binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
        void Clear();
        void UpdateSet(VkDevice device, VkDescriptorSet set);
    };

    // VkDescriptor: Manages descriptor pools and allocation.
    class VkDescriptor
    {
    public:
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float ratio;
        };

        // Initialize a descriptor pool with the specified pool size ratios.
        void Init(VkDevice device, u32 initialSets, std::span<PoolSizeRatio> poolRatios);

        // Clear all descriptor pools.
        void ClearPools(VkDevice device);

        // Destroy all descriptor pools.
        void DestroyPools(VkDevice device);

        // Allocate a descriptor set from the pool.
        VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);

    private:
        VkDescriptorPool GetPool(VkDevice device); // Retrieve a ready pool.
        VkDescriptorPool CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios); // Create a new descriptor pool.

        std::vector<PoolSizeRatio> ratios;  // Pool size ratios (used for creating new pools).
        std::vector<VkDescriptorPool> fullPools;  // Pools that are full and cannot allocate more sets.
        std::vector<VkDescriptorPool> readyPools;  // Pools that can still allocate sets.
        u32 setsPerPool{};  // Number of sets per pool.
    };

} // namespace GraphicsAPI::Vulkan

#endif // VULKANDESCRIPTOR_H
