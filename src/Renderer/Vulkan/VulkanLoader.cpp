//
// Created by Orgest on 7/18/2024.
//

#include "VulkanLoader.h"
#include "VulkanMain.h"


namespace GraphicsAPI::Vulkan
{

	bool VkLoader::LoadShader(const std::filesystem::path& filePath, VkDevice device, VkShaderModule* outShaderModule) {
		try {
			std::vector<u32> buffer = ReadFile(filePath);

			VkShaderModuleCreateInfo createInfo
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = buffer.size() * sizeof(u32),
				.pCode = buffer.data()
			};

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create shader for file: " + filePath.string());
			}

			*outShaderModule = shaderModule;
			return true;
		}
		catch (const std::exception& e) {
			LOG(ERR, e.what());
			return false;
		}
	}

	std::optional<std::vector<std::shared_ptr<MeshAsset>>> VkLoader::loadGltfMeshes(VkEngine *engine,
																		  const std::filesystem::path &filePath)
	{
		auto data = fastgltf::GltfDataBuffer::FromPath(filePath);
		constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

		fastgltf::Asset gltf;
		fastgltf::Parser parser{};
		auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);
		if (!load)
		{
			LOG(ERR, "Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
			return {};
		}
		gltf = std::move(load.get());

		std::vector<std::shared_ptr<MeshAsset>> meshes;
		std::vector<u32> indices;
		std::vector<Vertex> vertices;

		for (fastgltf::Mesh& mesh : gltf.meshes) {
			MeshAsset newMesh;
			newMesh.name = mesh.name;

			indices.clear();
			vertices.clear();

			for (auto&& primitive : mesh.primitives) {
				GeoSurface newSurface{};
				newSurface.startIndex = static_cast<u32>(indices.size());
				newSurface.count = static_cast<u32>(gltf.accessors[primitive.indicesAccessor.value()].count);

				loadIndices(gltf.accessors[primitive.indicesAccessor.value()], indices, vertices, gltf);
				loadVertices(gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex], vertices, gltf);

				loadAttribute(primitive, "NORMAL", gltf,
							  [&](const fastgltf::Accessor &accessor)
							  {
								  fastgltf::iterateAccessorWithIndex<glm::vec3>(
									  gltf, accessor, [&](glm::vec3 v, size_t index)
									  { vertices[vertices.size() - accessor.count + index].normal = v; });
							  });

				loadAttribute(primitive, "TEXCOORD_0", gltf,
							  [&](const fastgltf::Accessor &accessor)
							  {
								  fastgltf::iterateAccessorWithIndex<glm::vec2>(
									  gltf, accessor,
									  [&](glm::vec2 v, size_t index)
									  {
										  vertices[vertices.size() - accessor.count + index].uv_x = v.x;
										  vertices[vertices.size() - accessor.count + index].uv_y = v.y;
									  });
							  });

				loadAttribute(primitive, "COLOR_0", gltf,
							  [&](const fastgltf::Accessor &accessor)
							  {
								  fastgltf::iterateAccessorWithIndex<glm::vec4>(
									  gltf, accessor, [&](glm::vec4 v, size_t index)
									  { vertices[vertices.size() - accessor.count + index].color = v; });
							  });

				newMesh.surfaces.push_back(newSurface);
			}

			constexpr bool OverrideColors = true;
			if (OverrideColors) {
				for (Vertex& vtx : vertices) {
					vtx.color = glm::vec4(vtx.normal, 1.f);
				}
			}

			newMesh.meshBuffers = engine->UploadMesh(indices, vertices);
			meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
		}

		return meshes;
	}

	void VkLoader::loadIndices(const fastgltf::Accessor& indexAccessor, std::vector<u32>& indices, const std::vector<Vertex>& vertices, const fastgltf::Asset& gltf) {
		indices.reserve(indices.size() + indexAccessor.count);
		fastgltf::iterateAccessor<u32>(gltf, indexAccessor, [&](u32 idx) {
			indices.push_back(idx + vertices.size());
		});
	}

	void VkLoader::loadVertices(const fastgltf::Accessor& posAccessor, std::vector<Vertex>& vertices, const fastgltf::Asset& gltf) {
		vertices.resize(vertices.size() + posAccessor.count);
		fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {
			Vertex newvtx{};
			newvtx.position = v;
			newvtx.normal = {1, 0, 0};
			newvtx.color = glm::vec4{1.f};
			newvtx.uv_x = 0;
			newvtx.uv_y = 0;
			vertices[vertices.size() - posAccessor.count + index] = newvtx;
		});
	}

	void VkLoader::loadAttribute(const fastgltf::Primitive& primitive, const std::string& attributeName, const fastgltf::Asset& gltf, const std::function<void(const fastgltf::Accessor&)>& accessorFunc) {
		if (auto attr = primitive.findAttribute(attributeName); attr != primitive.attributes.end()) {
			accessorFunc(gltf.accessors[attr->accessorIndex]);
		}
	}
}
