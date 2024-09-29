//
// Created by Orgest on 8/31/2024.
//

#pragma once

#include "../../Platform/WindowContext.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
namespace GraphicsAPI::OpenGL
{
	class OpenGLMain
	{
	public:
		explicit OpenGLMain(Platform::WindowContext* wc);
		~OpenGLMain();

		bool Init() const;
		void FramebufferSizeCallback(Platform::WindowContext* window, int width, int height) const;
		void Render() const;
		void RenderLoop() const;
	private:
		Platform::WindowContext* wc_;
	};
}
