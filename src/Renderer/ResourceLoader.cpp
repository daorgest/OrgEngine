//
// Created by Orgest on 7/28/2024.
//

#include "ResourceLoader.h"

#include "Vulkan/VulkanMaterials.h"
using namespace GraphicsAPI;


std::vector<u32> ResourceLoader::ReadFile(const std::filesystem::path& filePath) const
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filePath.string());
	}

	size_t fileSize = file.tellg();
	std::vector<u32> buffer(fileSize / sizeof(u32));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}