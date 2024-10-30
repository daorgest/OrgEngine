#include "OpenGLMain.h"

using namespace GraphicsAPI::OpenGL;

OpenGLMain::OpenGLMain(Platform::WindowContext* wc) : wc_(wc) {}

OpenGLMain::~OpenGLMain()
{
	Cleanup();
}

void OpenGLMain::Cleanup()
{
	// Clean up the OpenGL context and window
	if (wc_->sdlContext)
	{
		SDL_GL_DestroyContext(wc_->sdlContext);
		wc_->sdlContext = nullptr;
	}

	if (wc_->sdlWindow)
	{
		SDL_DestroyWindow(wc_->sdlWindow);
		wc_->sdlWindow = nullptr;
	}

	// Quit SDL subsystems
	SDL_Quit();
}

bool OpenGLMain::Init() const
{
	// Initialize SDL for video and OpenGL
	SDL_Init( SDL_INIT_VIDEO );

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Create the SDL window with OpenGL context
	wc_->sdlWindow = SDL_CreateWindow(
		"OpenGL with SDL3",
		1920,
		1080,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!wc_->sdlWindow)
	{
		LOG(ERR, "Failed to create SDL window: " + std::string(SDL_GetError()));
		return false;
	}

	// Create the OpenGL context using SDL
	wc_->sdlContext = SDL_GL_CreateContext(wc_->sdlWindow);
	if (!wc_->sdlContext)
	{
		LOG(ERR, "Failed to create OpenGL context: " + std::string(SDL_GetError()));
		return false;
	}

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		LOG(ERR, "Failed to initialize GLAD");
		return false;
	}

	// Enable V-Sync
	if (SDL_GL_SetSwapInterval(1) != 0)
	{
		LOG(WARN, "Warning: Unable to set V-Sync: " + std::string(SDL_GetError()));
	}

	glViewport(0, 0, wc_->screenWidth, wc_->screenHeight);
	return true;
}

void OpenGLMain::FramebufferSizeCallback(int width, int height) const
{
	if (width > 0 && height > 0)
	{
		wc_->screenWidth = width;
		wc_->screenHeight = height;
		glViewport(0, 0, width, height);
	}
}

void OpenGLMain::Render() const
{
	// Set the clear color before clearing the screen
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	// Clear the screen with the set color
	glClear(GL_COLOR_BUFFER_BIT);

	// Present the back buffer (swap the buffers)
	SDL_GL_SwapWindow(wc_->sdlWindow);
}

void OpenGLMain::RenderLoop() const
{
	bool running = true;
	SDL_Event event;

	// Main rendering loop
	while (running)
	{
		// Poll events (e.g., window close, resize, etc.)
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}

			// Handle window resize events
			if (event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				int newWidth = event.window.data1;
				int newHeight = event.window.data2;
				FramebufferSizeCallback(newWidth, newHeight);
			}
		}

		// Render the scene
		Render();
	}
}

