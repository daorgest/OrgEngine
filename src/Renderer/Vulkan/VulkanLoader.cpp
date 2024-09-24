//
// Created by Orgest on 7/18/2024.
//

#include "VulkanLoader.h"
#include "VulkanMain.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>

namespace GraphicsAPI::Vulkan
{

	bool VkLoader::LoadShader(const std::filesystem::path& filePath, VkDevice device, VkShaderModule* outShaderModule) const
	{
		try
		{
			std::vector<u32> buffer = ReadFile(filePath);

			VkShaderModuleCreateInfo createInfo
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = buffer.size() * sizeof(u32),
				.pCode = buffer.data()
			};

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create shader for file: " + filePath.string());
			}

			*outShaderModule = shaderModule;
			return true;
		}
		catch (const std::exception& e)
		{
			LOG(ERR, e.what());
			return false;
		}
	}

	std::optional<std::vector<std::shared_ptr<MeshAsset>>> VkLoader::LoadGltfMeshes(VkEngine* engine,
                                                                               const std::filesystem::path& filePath, bool OverrideColors)
	{
		LOG(INFO, "Loading GLTF model: ", filePath.string());

		auto gltfFile = fastgltf::GltfDataBuffer::FromPath(filePath);
		if (!gltfFile)
		{
			LOG(ERR, "Failed to load GLTF file at path: ", filePath.string());
			return std::nullopt;
		}

		constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;
		fastgltf::Parser parser{};

		auto loadResult = parser.loadGltfBinary(gltfFile.get(), filePath.parent_path(), gltfOptions);
		if (!loadResult)
		{
			LOG(ERR, "Failed to parse GLTF at path: ", filePath.string(), " Error code: ", fastgltf::to_underlying(loadResult.error()));
			return std::nullopt;
		}

		LOG(INFO, "Successfully loaded GLTF model: ", filePath.string());
		fastgltf::Asset gltf = std::move(loadResult.get());
		std::vector<std::shared_ptr<MeshAsset>> meshes;

		for (fastgltf::Mesh& mesh : gltf.meshes)
		{
			MeshAsset newMesh;
			newMesh.name = mesh.name;

			// Containers for vertices and indices
			std::vector<u32> indices;
			std::vector<Vertex> vertices;

			for (auto&& primitive : mesh.primitives)
			{
				GeoSurface newSurface{};
				if (!primitive.indicesAccessor)
				{
					LOG(WARN, "Primitive has no indices accessor, skipping.");
					continue;
				}

				auto indicesAccessorIndex = primitive.indicesAccessor.value();
				if (indicesAccessorIndex >= gltf.accessors.size())
				{
					LOG(ERR, "Invalid indices accessor index for mesh: ", mesh.name);
					continue;
				}

				fastgltf::Accessor& indexAccessor = gltf.accessors[indicesAccessorIndex];
				newSurface.startIndex			  = static_cast<u32>(indices.size());
				newSurface.count				  = static_cast<u32>(indexAccessor.count);

				size_t initialVertex = vertices.size();
				indices.reserve(indices.size() + indexAccessor.count);

				fastgltf::iterateAccessor<u32>(gltf, indexAccessor,
											   [&](const u32 index) { indices.push_back(index + initialVertex); });

				// Load vertex positions
				auto posAttribute = primitive.findAttribute("POSITION");
				if (posAttribute != primitive.attributes.end())
				{
					auto positionAccessorIndex = posAttribute->accessorIndex;
					if (positionAccessorIndex >= gltf.accessors.size())
					{
						LOG(ERR, "Invalid position accessor index for mesh: ", mesh.name);
						continue;
					}

					fastgltf::Accessor& posAccessor = gltf.accessors[positionAccessorIndex];
					vertices.resize(vertices.size() + posAccessor.count);

					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
						[&](const glm::vec3 v, const size_t index)
						{
							Vertex newVertex{};
							newVertex.position = v;
							newVertex.normal = {1, 0, 0};  // Default normal
							newVertex.color = glm::vec4{1.f}; // Default color
							newVertex.uv_x = 0;
							newVertex.uv_y = 0;
							vertices[initialVertex + index] = newVertex;
						});
				}
				else
				{
					LOG(WARN, "No POSITION attribute found for primitive in mesh: ", mesh.name);
				}

				// Load vertex normals
				auto normalsAttribute = primitive.findAttribute("NORMAL");
				if (normalsAttribute != primitive.attributes.end())
				{
					auto normalsAccessorIndex = normalsAttribute->accessorIndex;
					if (normalsAccessorIndex < gltf.accessors.size())
					{
						fastgltf::Accessor& normalsAccessor = gltf.accessors[normalsAccessorIndex];
						fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, normalsAccessor,
																	  [&](const glm::vec3 v, const size_t index)
																	  { vertices[initialVertex + index].normal = v; });
					}
					else
					{
						LOG(ERR, "Invalid normals accessor index for mesh: ", mesh.name);
					}
				}

				// Load UVs
				auto uvAttribute = primitive.findAttribute("TEXCOORD_0");
				if (uvAttribute != primitive.attributes.end())
				{
					auto uvAccessorIndex = uvAttribute->accessorIndex;
					if (uvAccessorIndex < gltf.accessors.size())
					{
						fastgltf::Accessor& uvAccessor = gltf.accessors[uvAccessorIndex];
						fastgltf::iterateAccessorWithIndex<glm::vec2>( gltf, uvAccessor,
							[&](const glm::vec2 v, const size_t index)
							{
								vertices[initialVertex + index].uv_x = v.x;
								vertices[initialVertex + index].uv_y = v.y;
							});
					}
					else
					{
						LOG(ERR, "Invalid UV accessor index for mesh: ", mesh.name);
					}
				}

				// Load vertex colors
				auto colorsAttribute = primitive.findAttribute("COLOR_0");
				if (colorsAttribute != primitive.attributes.end())
				{
					auto colorsAccessorIndex = colorsAttribute->accessorIndex;
					if (colorsAccessorIndex < gltf.accessors.size())
					{
						fastgltf::Accessor& colorsAccessor = gltf.accessors[colorsAccessorIndex];
						fastgltf::iterateAccessorWithIndex<glm::vec4>(
							gltf, colorsAccessor,
							[&](const glm::vec4 v, const size_t index)
							{
								vertices[initialVertex + index].color = v;
							});
					}
					else
					{
						LOG(ERR, "Invalid colors accessor index for mesh: ", mesh.name);
					}
				}

				newMesh.surfaces.push_back(newSurface);
			}

			if (OverrideColors)
			{
				for (Vertex& vtx : vertices)
				{
					vtx.color = glm::vec4(vtx.normal, 1.f);
				}
			}

			newMesh.meshBuffers = engine->UploadMesh(indices, vertices);
			meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
		}


		return meshes;
	}
}
