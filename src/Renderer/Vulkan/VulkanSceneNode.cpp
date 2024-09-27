//
// Created by Orgest on 8/31/2024.
//

#include "VulkanSceneNode.h"

#include "VulkanLoader.h"
#include "VulkanMaterials.h"

using namespace GraphicsAPI::Vulkan;

void Node::RefreshTransform(const glm::mat4& parentMatrix)
{
	worldTransform = parentMatrix * localTransform;
	for (const auto& child : children)
	{
		child->RefreshTransform(worldTransform);
	}
}

void Node::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	// draw children
	for (const auto& c : children)
	{
		c->Draw(topMatrix, ctx);
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
			// .material = &s.material->data,
			.transform = nodeMatrix,
			.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress
		};

		ctx.OpaqueSurfaces.push_back(def);
	}

	// Recurse down
	Node::Draw(topMatrix, ctx);
}