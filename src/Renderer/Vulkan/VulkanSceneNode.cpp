//
// Created by Orgest on 8/31/2024.
//

#include "VulkanSceneNode.h"
#include "VulkanLoader.h"

using namespace GraphicsAPI::Vulkan;

// Implementation of Node::RefreshTransform
void Node::RefreshTransform(const glm::mat4& parentMatrix)
{
	worldTransform = parentMatrix * localTransform;
	for (auto& child : children)
	{
		child->RefreshTransform(worldTransform);
	}
}

// Implementation of Node::Draw
void Node::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	for (auto& child : children)
	{
		child->Draw(topMatrix, ctx);
	}
}

// Implementation of MeshNode::Draw
void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	glm::mat4 nodeMatrix = topMatrix * worldTransform;

	for (auto& s : mesh->surfaces)
	{
		RenderObject def
		{
			.indexCount = s.count,
			.firstIndex = s.startIndex,
			.indexBuffer = mesh->meshBuffers.indexBuffer.buffer,
			.material = &s.material->data,
			.transform = nodeMatrix,
			.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress
		};

		// Add render object to opaque surfaces
		ctx.OpaqueSurfaces.push_back(def);
	}

	// Recursively call Draw on child nodes
	Node::Draw(topMatrix, ctx);
}
