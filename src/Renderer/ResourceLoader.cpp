//
// Created by Orgest on 7/28/2024.
//

#include "ResourceLoader.h"

using namespace GraphicsAPI;

std::vector<u32> ResourceLoader::ReadFile(const std::filesystem::path& filePath) const
{
	auto fileSize = file_size(filePath);

	if (fileSize % sizeof(u32) != 0) {
		throw std::runtime_error("File size is not aligned to u32: " + filePath.string());
	}

	// Open the file
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filePath.string());
	}

	// Create a buffer and read file contents
	std::vector<u32> buffer(fileSize / sizeof(u32));
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	if (!file) {
		throw std::runtime_error("Failed to read the entire file: " + filePath.string());
	}

	return buffer;
}

bool ResourceLoader::ReadFileText(const std::filesystem::path& filePath, std::string& outFile)
{
	std::ifstream file(filePath);

	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			outFile.append(line);
			outFile.append("\n");
		}

		file.close();
		return true;
	}
	return false;
}