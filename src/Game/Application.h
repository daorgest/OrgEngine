//
// Created by Orgest on 8/26/2024.
//

#ifndef APPLICATION_H
#define APPLICATION_H


#include "../Core/Audio.h"
#include "../Platform/PlatformWindows.h"

#ifdef OPENGL_BUILD
#include "../Renderer/OpenGL/OpenGLMain.h"
#endif

#ifdef VULKAN_BUILD
#include "../Renderer/Vulkan/VulkanMain.h"
#endif

#ifdef DIRECTX11_BUILD
#include "../Renderer/DirectX11/DirectX11Main.h"
#endif


class Application
{
public:
	Application();
	~Application();

	void Run();
	void Render();

private:
	bool InitializeEngine();
	void CleanupEngine();
	void LoadAudio();
	void PlaySoundIfNeeded();

	Platform::WindowContext windowContext_;
#ifdef VULKAN_BUILD
	Platform::Win32 win32_{&windowContext_};  // Stack allocation for Win32, i hope this happens first LMAO
	GraphicsAPI::Vulkan::VkEngine vkEngine_{&windowContext_};  // Stack allocation for Vulkan
#elif defined(OPENGL_BUILD)
	GraphicsAPI::OpenGL::OpenGLMain glEngine_{&windowContext_};  // Stack allocation for OpenGL
#endif
	Audio audio_;
	FMOD::Sound* sound{nullptr};
	bool initialized_{false};
};


#endif //APPLICATION_H
