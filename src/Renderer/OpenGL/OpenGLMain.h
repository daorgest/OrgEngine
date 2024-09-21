//
// Created by Orgest on 8/31/2024.
//

#pragma once
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include "../../Platform/WindowContext.h"
namespace GraphicsAPI::OpenGL
{
	class OpenGLMain
	{
	public:
		OpenGLMain(Platform::WindowContext* wc);
		~OpenGLMain();
		bool Init();
		void FramebufferSizeCallback(Platform::WindowContext* window, int width, int height) const;
		void Render() const;
		void RenderLoop() const;
	private:
		Platform::WindowContext* wc_{};
		SDL_Window* window_{};

		SDL_GLContext glContext_;
	};
}