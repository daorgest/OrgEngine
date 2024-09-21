//
// Created by Orgest on 8/26/2024.
//

#ifndef APPLICATION_H
#define APPLICATION_H


#include "../Core/Audio.h"
#include "../Core/InputHandler.h"
#include "../Platform/PlatformWindows.h"
#ifdef USE_OPENGL
#include "../Renderer/OpenGL/OpenGLMain.h"
#endif

#include "../Renderer/Vulkan/VulkanMain.h"

class Application
{
public:
	Application();
	~Application() = default;

	void Run();
	static void HandleInput(Input& input);
	void LoadAudio();
	void PlaySoundIfNeeded();
	void HandleMessages();
	void Update();
	void Render();

private:
	Platform::WindowContext		  windowContext_;
#ifdef VULKAN_BUILD
	Platform::Win32				  win32_;
#endif
	Audio						  audio_;
	FMOD::Sound*				  sound{nullptr};

#ifdef VULKAN_BUILD
	GraphicsAPI::Vulkan::VkEngine vkEngine_;
#elif defined(OPENGL_BUILD)
	GraphicsAPI::OpenGL::OpenGLMain glEngine_;
#endif

	bool shouldQuit{false};
	bool stopRendering_{false};
	bool resizeRequested{false};
	bool initialized_ {false};
};



#endif //APPLICATION_H
