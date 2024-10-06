//
// Created by Orgest on 7/23/2024.
//

#pragma once
#ifdef VULKAN_BUILD
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include "../../Core/Array.h"
#include "../../Core/Vector.h"
#include "../../Platform/WindowContext.h"

#include <backends/imgui_impl_vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


#include <fmt/core.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#ifdef DEBUG
#define VK_CHECK(x)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult err = x;                                                                                              \
        if (err != VK_SUCCESS)                                                                                         \
        {                                                                                                              \
            fmt::print("Vulkan error at {}:{}: {}\n", __FILE__, __LINE__, string_VkResult(err));                      \
            abort();                                                                                                   \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)
#else
#define VK_CHECK(x) (x)
#endif


namespace GraphicsAPI::Vulkan
{

	struct VulkanData
	{
		VkInstance				 instance{VK_NULL_HANDLE};
		VkDebugUtilsMessengerEXT dbgMessenger{VK_NULL_HANDLE};
		VkPhysicalDevice		 physicalDevice{VK_NULL_HANDLE};
		VkDevice				 device{VK_NULL_HANDLE};
		VkSurfaceKHR			 surface{VK_NULL_HANDLE};
	};

	struct AllocatedBuffer
	{
		VkBuffer		  buffer;
		VmaAllocation	  allocation;
		VmaAllocationInfo info;
	};

	struct Vertex
	{
		glm::vec3 position;
		float	  uv_x;
		glm::vec3 normal;
		float	  uv_y;
		glm::vec4 color;
	};

	// holds the resources needed for a mesh
	struct GPUMeshBuffers
	{
		AllocatedBuffer indexBuffer;
		AllocatedBuffer vertexBuffer;
		VkDeviceAddress vertexBufferAddress;
	};

	// push constants for our mesh object draws
	struct GPUDrawPushConstants
	{
		glm::mat4		worldMatrix;
		VkDeviceAddress vertexBuffer;
	};

	struct AllocatedImage
	{
		VkImage		  image;
		VkImageView	  imageView;
		VmaAllocation allocation;
		VkExtent3D	  imageExtent;
		VkFormat	  imageFormat;
	};
}
#endif