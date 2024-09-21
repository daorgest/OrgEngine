//
// Created by Orgest on 8/31/2024.
//

#ifndef VULKANSCENENODE_H
#define VULKANSCENENODE_H
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>


namespace GraphicsAPI::Vulkan
{
	struct MaterialInstance;
	struct MeshAsset;
	struct DrawContext;

	class IRenderable
	{
	public:
		virtual ~IRenderable() = default;

	private:
		virtual void Draw(const glm::mat4& topMatrix, DrawContext& context) const = 0;
	};

	struct Node : IRenderable
	{
		// parent pointer must be a weak pointer to avoid circular dependencies
		std::weak_ptr<Node> parent;
		std::vector<std::shared_ptr<Node>> children;

		glm::mat4 localTransform;
		glm::mat4 worldTransform;

		void RefreshTransform(const glm::mat4& parentMatrix);
		virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);
	};

	// Implementation of a drawable mesh node.
	struct MeshNode : Node
	{
		std::shared_ptr<MeshAsset> mesh;

		void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
	};

	// Structure to hold rendering-related data
	struct RenderObject
	{
		uint32_t indexCount;
		uint32_t firstIndex;
		VkBuffer indexBuffer;

		MaterialInstance *material;

		glm::mat4		transform;
		VkDeviceAddress vertexBufferAddress;
	};

	// Structure to hold a list of RenderObjects
	struct DrawContext
	{
		std::vector<RenderObject> OpaqueSurfaces;
	};
}


#endif //VULKANSCENENODE_H
