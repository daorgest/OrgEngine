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
		void Cleanup();

		bool Init() const;
		void FramebufferSizeCallback(int width, int height) const;
		void Render() const;
		void RenderLoop() const;
	private:
		Platform::WindowContext* wc_;
	};
}
