//
// Created by Orgest on 7/28/2024.
//


#pragma once
#include <fstream>
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
