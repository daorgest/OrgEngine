//
// Created by Orgest on 7/28/2024.
//


#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "../Core/PrimTypes.h"
namespace GraphicsAPI
{

	class ResourceLoader
	{
	public:
		virtual ~ResourceLoader() = default;

	protected:
		[[nodiscard]] std::vector<u32> ReadFile(const std::filesystem::path &filePath) const;
	};

} // namespace GraphicsAPI
