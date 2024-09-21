//
// Created by Orgest on 8/31/2024.
//

#include "OpenGLMain.h"


using namespace GraphicsAPI::OpenGL;

OpenGLMain::OpenGLMain(Platform::WindowContext* wc) : wc_(wc)
{
}

OpenGLMain::~OpenGLMain()
{
	// Cleanup OpenGL context and SDL window
	if (glContext_)
	{
		SDL_GL_DestroyContext(glContext_);
	}
	if (window_)
	{
		SDL_DestroyWindow(window_);
	}
	SDL_Quit();
}



bool OpenGLMain::Init()
{
	// Initialize SDL video subsystem
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		LOG(ERR, "Failed to initialize SDL: " + std::string(SDL_GetError()));
		return false;
	}


	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	window_ = SDL_CreateWindow(wc_->appName,
		wc_->screenWidth,
		wc_->screenHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	glContext_ = SDL_GL_CreateContext(window_);
	if (!glContext_)
	{
		LOG(ERR, "Failed to create OpenGL context: " + std::string(SDL_GetError()));
	}

	// Enable V-Sync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		LOG(WARN, "Failed to set V-Sync: " + std::string(SDL_GetError()));
	}

	// Load OpenGL function pointers using GLAD
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
	{
		LOG(ERR, "Failed to initialize GLAD.");
		return false;
	}

	// Log OpenGL version
	const GLubyte* version = glGetString(GL_VERSION);
	LOG(INFO, "OpenGL Version: " + std::string(reinterpret_cast<const char*>(version)));

	// Set initial viewport size
	glViewport(0, 0, wc_->screenWidth, wc_->screenHeight);

	return true;
}

void OpenGLMain::FramebufferSizeCallback(Platform::WindowContext* window, int width, int height) const
{
	glViewport(0, 0, width, height);
}

void OpenGLMain::Render() const
{
	// Clear the screen with a color
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	// Present the back buffer (swap the buffers)
	SDL_GL_SwapWindow(window_);
}

void OpenGLMain::RenderLoop() const
{
	bool running = true;
	SDL_Event event;

	// Main loop
	while (running)
	{
		// Event processing
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
			else if (event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				int width = event.window.data1;
				int height = event.window.data2;
				FramebufferSizeCallback(wc_, width, height);
			}
		}

		// Rendering
		Render();
	}
}