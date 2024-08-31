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

	struct DescriptorLayoutBuilder
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		void AddBinding(u32 binding, VkDescriptorType type);
		void Clear();
		VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr,
									VkDescriptorSetLayoutCreateFlags flags = 0);
	};

	struct DescriptorAllocatorGrowable
	{
		struct PoolSizeRatio
		{
			VkDescriptorType type;
			float ratio;
		};

		VkDescriptorPool pool;

		void InitPool(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios);
		void ClearDescriptors(VkDevice device) const;
		void DestroyPool(VkDevice device);

		VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
	};

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

	class VkDescriptor
	{
	public:
		struct PoolSizeRatio
		{
			VkDescriptorType type;
			float ratio;
		};

		void Init(VkDevice device, u32 initialSets, std::span<PoolSizeRatio> poolRatios);
		void ClearPools(VkDevice device);
		void DestroyPools(VkDevice device);

		VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);

	private:
		VkDescriptorPool GetPool(VkDevice device);
		VkDescriptorPool CreatePool(VkDevice device, u32 setCount, std::span<PoolSizeRatio> poolRatios);

		std::vector<PoolSizeRatio> ratios;
		std::vector<VkDescriptorPool> fullPools;
		std::vector<VkDescriptorPool> readyPools;
		u32 setsPerPool{};
	};


} // namespace GraphicsAPI::Vulkan


#endif // VULKANDESCRIPTOR_H
