#pragma once

#include <memory>
#include <vector>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

#include "../../Core/PrimTypes.h"

using NodeID = u32;

namespace GraphicsAPI::Vulkan
{
	// Forward declarations
	struct MaterialInstance;
	struct MeshAsset;
	struct DrawContext;

	class IRenderable
	{
	public:
		virtual ~IRenderable() = default;
		virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
	};

	struct Node : IRenderable
	{
		std::weak_ptr<Node> parent;
		std::vector<std::shared_ptr<Node>> children;

		glm::mat4 localTransform;
		glm::mat4 worldTransform;

		// Refresh the transformation matrix of the node
		void RefreshTransform(const glm::mat4& parentMatrix);

		// Draw the node and its children
		void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
	};

	// Drawable mesh node class
	struct MeshNode : Node
	{
		std::shared_ptr<MeshAsset> mesh;

		// Draw method for MeshNode
		void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
	};

	// Structure to hold rendering-related data
	struct RenderObject
	{
		u32               indexCount;
		u32               firstIndex;
		VkBuffer          indexBuffer;

		MaterialInstance* material;

		glm::mat4         transform;
		VkDeviceAddress   vertexBufferAddress;
	};

	// Structure to hold a list of RenderObjects
	struct DrawContext
	{
		std::vector<RenderObject> OpaqueSurfaces;
		std::vector<RenderObject> TransparentSurfaces;
	};
}
