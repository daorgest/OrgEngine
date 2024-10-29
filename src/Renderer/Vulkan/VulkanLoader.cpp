//
// Created by Orgest on 7/18/2024.
//

#include "VulkanLoader.h"
#include "VulkanMain.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanImages.h"


using namespace GraphicsAPI::Vulkan;

bool VkLoader::LoadShader(const std::filesystem::path& filePath, VkDevice device, VkShaderModule* outShaderModule) const
{
	try
	{
		auto buffer = ReadFile(filePath);

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

VkFilter VkLoader::ExtractFilter(fastgltf::Filter filter)
{
	switch (filter)
	{
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;
		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapNearest:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_FILTER_LINEAR;
	}
}

VkSamplerMipmapMode VkLoader::ExtractMipmapMode(fastgltf::Filter filter)
{
	switch (filter)
	{
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}


std::optional<std::shared_ptr<LoadedGLTF>> VkLoader::LoadGltfMeshes(VkEngine*                    engine,
                                                                    const std::filesystem::path& filePath,
                                                                    bool                         flipZAxis)
{
    LOG(INFO, "Loading GLTF model: ", filePath.string());

    // Load GLTF file
    auto scene = std::make_shared<LoadedGLTF>();
	scene->creator = engine;
	LoadedGLTF& file = *scene;

	fastgltf::Parser parser {};

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
								 fastgltf::Options::AllowDouble |
								 fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;

	auto data = fastgltf::GltfDataBuffer::FromPath(filePath);
	if (data.error() != fastgltf::Error::None)
	{
		LOG(ERR, "Failed to load GLTF data from file: ", filePath);
		return {};
	}

	fastgltf::Asset gltf;
	auto type = fastgltf::determineGltfFileType(data.get());
	if (type == fastgltf::GltfType::glTF)
	{
		auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);
		if (load)
		{
			gltf = std::move(load.get());
		}
		else
		{
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	}
	else if (type == fastgltf::GltfType::GLB)
	{
		auto load = parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);
		if (load)
		{
			gltf = std::move(load.get());
		}
		else
		{
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	}
	else
	{
		std::cerr << "Failed to determine glTF container" << std::endl;
		return {};
	}

	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
	};

	file.descriptorPool.Init(vd.device, gltf.materials.size(), sizes);

	for (fastgltf::Sampler& sampler : gltf.samplers)
	{

		VkSamplerCreateInfo sampleInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
			.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
			.mipmapMode = ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
			.minLod = 0.0f,
			.maxLod = VK_LOD_CLAMP_NONE
		};

		VkSampler vkSampler;
		if (vkCreateSampler(vd.device, &sampleInfo, nullptr, &vkSampler) == VK_SUCCESS)
		{
			file.samplers.push_back(vkSampler);
		}
		else
		{
			LOG(ERR, "Failed to create Vulkan sampler");
		}
	}

	std::vector<std::shared_ptr<MeshAsset>> meshes;
	std::vector<std::shared_ptr<Node>> nodes;
	std::vector images(gltf.images.size(), engine->errorCheckerboardImage_); // checkerboard image for default
	std::vector<std::shared_ptr<GLTFMaterial>> materials;

	file.materialDataBuffer = engine->CreateBuffer(sizeof(GLTFMetallicRoughness::MaterialConstants) *
		gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	int data_index = 0;
	auto* sceneMaterialConstants =
		static_cast<GLTFMetallicRoughness::MaterialConstants*>(file.materialDataBuffer.info.pMappedData);

	for (fastgltf::Material& mat : gltf.materials)
	{
		auto newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        GLTFMetallicRoughness::MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metalRoughnessFactors.x = mat.pbrData.metallicFactor;
        constants.metalRoughnessFactors.y = mat.pbrData.roughnessFactor;

        // write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

		MaterialPass passType = (mat.alphaMode == fastgltf::AlphaMode::Blend) ?
								MaterialPass::Transparent : MaterialPass::MainColor;

        GLTFMetallicRoughness::MaterialResources materialResources;
        // default the material textures
        materialResources.colorImage = engine->whiteImage_;
        materialResources.colorSampler = engine->defaultSamplerLinear_;
        materialResources.metalRoughImage = engine->whiteImage_;
        materialResources.metalRoughSampler = engine->defaultSamplerLinear_;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallicRoughness::MaterialConstants);

		// Get base color texture if present
		if (mat.pbrData.baseColorTexture)
		{
			auto& texture = gltf.textures[mat.pbrData.baseColorTexture->textureIndex];

			materialResources.colorImage = images[texture.imageIndex.value()];
			if (texture.samplerIndex.has_value())
			{
				materialResources.colorSampler = file.samplers[texture.samplerIndex.value()];
			}
		}

		newMat->data = engine->metalRoughMaterial.WriteMaterial(vd.device, passType, materialResources, file.descriptorPool);
		data_index++;
    }

	// Load meshes
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	for (const auto& mesh : gltf.meshes)
	{
		auto newMesh = std::make_shared<MeshAsset>();
		meshes.push_back(newMesh);
		file.meshes[mesh.name.c_str()] = newMesh;

		indices.clear();
		vertices.clear();

		for (const auto& primitive : mesh.primitives)
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
			newSurface.startIndex = static_cast<u32>(indices.size());
			newSurface.count = static_cast<u32>(indexAccessor.count);

			size_t initialVertex = vertices.size();
			indices.reserve(indices.size() + indexAccessor.count);

			fastgltf::iterateAccessor<u32>(gltf, indexAccessor, [&](const u32 index)
			{
				indices.push_back(index + initialVertex);
			});

			// Load vertex positions and flip Z-axis if necessary
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

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](const glm::vec3 v, const size_t index)
				{
					Vertex newVertex{};
					if (flipZAxis)
					{
						newVertex.position = { v.x, v.y, -v.z };  // Flip Z-axis for Vulkan
					}
					else
					{
						newVertex.position = v;  // No flipping for DirectX
					}
					newVertex.normal = { 1, 0, 0 };  // Default normal
					newVertex.color = glm::vec4{ 1.f };  // Default color
					newVertex.uv_x = 0;
					newVertex.uv_y = 0;
					vertices[initialVertex + index] = newVertex;
				});
			} else
			{
				LOG(WARN, "No POSITION attribute found for primitive in mesh: ", mesh.name);
			}

			// Load vertex normals and flip Z-axis if necessary
			auto normalsAttribute = primitive.findAttribute("NORMAL");
			if (normalsAttribute != primitive.attributes.end())
			{
				auto normalsAccessorIndex = normalsAttribute->accessorIndex;
				if (normalsAccessorIndex < gltf.accessors.size())
				{
					fastgltf::Accessor& normalsAccessor = gltf.accessors[normalsAccessorIndex];
					fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, normalsAccessor, [&](const glm::vec3 v, const size_t index)
					{
						if (flipZAxis)
						{
							vertices[initialVertex + index].normal = { v.x, v.y, -v.z };  // Flip Z-axis for Vulkan
						} else
						{
							vertices[initialVertex + index].normal = v;  // No flipping for DirectX
						}
					});
				} else
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
					fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, uvAccessor, [&](const glm::vec2 v, const size_t index)
					{
						vertices[initialVertex + index].uv_x = v.x;
						vertices[initialVertex + index].uv_y = v.y;
					});
				} else
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
					fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colorsAccessor, [&](const glm::vec4 v, const size_t index)
					{
						vertices[initialVertex + index].color = v;
					});
				} else
				{
					LOG(ERR, "Invalid colors accessor index for mesh: ", mesh.name);
				}
			}

			newSurface.material = primitive.materialIndex.has_value() ?
								  materials[primitive.materialIndex.value()] : materials[0];
			newMesh->surfaces.push_back(newSurface);
		}

		newMesh->meshBuffers = engine->UploadMesh(indices, vertices);
	}

	// Load nodes
    for (const auto& gltfNode : gltf.nodes)
    {
	    std::shared_ptr<Node> newNode;

    	// Check if the node contains a mesh, if so, create a MeshNode
    	if (gltfNode.meshIndex.has_value())
    	{
    		newNode = std::make_shared<MeshNode>();
    		dynamic_cast<MeshNode*>(newNode.get())->mesh = meshes[*gltfNode.meshIndex];
    	}
    	else
    	{
    		newNode = std::make_shared<Node>();
    	}
    	nodes.push_back(newNode);
    	file.nodes[gltfNode.name.c_str()] = newNode;

    	std::visit(fastgltf::visitor {
			[&](const fastgltf::TRS& transform)
			{
				// TRS (Translation, Rotation, Scale)
				glm::vec3 translation(transform.translation[0], transform.translation[1], transform.translation[2]);
				glm::quat rotation(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
				glm::vec3 scale(transform.scale[0], transform.scale[1], transform.scale[2]);

				glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
				glm::mat4 rotationMatrix = glm::toMat4(rotation);
				glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

				// Apply TRS to get the local transform
				newNode->localTransform = translationMatrix * rotationMatrix * scaleMatrix;
			},
			[&](const fastgltf::math::fmat4x4& matrix)
			{
				// Matrix-based transformation
				memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
			}
		}, gltfNode.transform);
    }


	// Setup parent-child relationships between nodes
	for (size_t i = 0; i < gltf.nodes.size(); ++i)
	{
		auto& gltfNode = gltf.nodes[i];
		auto& sceneNode = nodes[i];

		for (const auto& childIndex : gltfNode.children)
		{
			sceneNode->children.push_back(nodes[childIndex]);
			nodes[childIndex]->parent = sceneNode;
		}
	}

	// Identify top-level nodes (nodes without parents)
	for (auto& node : nodes)
	{
		if (node->parent.lock() == nullptr)
		{
			file.topNodes.push_back(node);
			node->RefreshTransform(glm::mat4(1.0f));
		}
	}

	for (fastgltf::Image& image : gltf.images)
	{
		std::optional<AllocatedImage> img = LoadImage(engine, gltf, image);

		if (img.has_value())
		{
			images.push_back(*img);
			file.images[image.name.c_str()] = *img;
		}
		else
		{
			// Use a default checkerboard image if loading fails
			images.push_back(engine->errorCheckerboardImage_);
			std::cout << "GLTF failed to load texture: " << image.name << std::endl;
		}
	}


	return scene;
}

// Load image data into an optional AllocatedImage
std::optional<AllocatedImage> VkLoader::LoadImage(VkEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
    AllocatedImage newImage {}; // Image structure to be filled upon successful load
    int width, height, nrChannels;

    // Visit image data and handle different source types (URI, Vector, BufferView)
    std::visit(
        fastgltf::visitor {
            [](auto&) {}, // Default case for unsupported types, do nothing

            // 1. Case: Image stored outside the GLTF/GLB file
            [&](const fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // Ensure no byte offsets for stbi
                assert(filePath.uri.isLocalPath());   // Support only local file URIs

                // Convert URI path to std::string
                std::string path(filePath.uri.path().begin(), filePath.uri.path().end());

                // Load image from file path using stb_image
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    // Set image dimensions and create Vulkan image
                	VkExtent3D imageSize = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
                    newImage = engine->CreateImageData(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
                    stbi_image_free(data); // Free loaded image data
                }
            },

            // 2. Case: Image loaded into a vector structure (base64 or external image)
            [&](const fastgltf::sources::Array& vector) {
                // Load image from memory vector
                unsigned char* data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(vector.bytes.data()),
                                                            static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                if (data) {
                	VkExtent3D imageSize = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
                    newImage = engine->CreateImageData(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
                    stbi_image_free(data);
                }
            },

            // 3. Case: Image embedded in GLB file's buffer view
            [&](const fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor {
                    [](auto&) {}, // Default case

                    [&](const fastgltf::sources::Array& vector) {
                        unsigned char* data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(vector.bytes.data() + bufferView.byteOffset),
                                                                    static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
                        if (data) {
                        	VkExtent3D imageSize = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
                            newImage = engine->CreateImageData(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
                            stbi_image_free(data);
                        }
                    }
                }, buffer.data);
            }
        }, image.data
    );

    // Return the new image if successfully created; otherwise, return an empty optional
    return newImage.image != VK_NULL_HANDLE ? std::optional<AllocatedImage>{newImage} : std::nullopt;
}

void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	// create renderables from the scenenodes
	for (auto& n : topNodes)
	{
		n->Draw(topMatrix, ctx);
	}
}

void LoadedGLTF::ClearAll()
{
	VkDevice dv = vd.device;

	descriptorPool.DestroyPools(dv);
	creator->DestroyBuffer(materialDataBuffer);

	for (auto& [k, v] : meshes) {

		creator->DestroyBuffer(v->meshBuffers.indexBuffer);
		creator->DestroyBuffer(v->meshBuffers.vertexBuffer);
	}

	for (auto& [k, v] : images) {

		if (v.image == creator->errorCheckerboardImage_.image) {
			//dont destroy the default images
			continue;
		}
		VkImages::DestroyImage(v, vd.device, creator->allocator_);
	}

	for (auto& sampler : samplers) {
		vkDestroySampler(dv, sampler, nullptr);
	}
}
