//
// Created by Orgest on 8/26/2024.
//

#ifndef APPLICATION_H
#define APPLICATION_H



#include "../Platform/PlatformWindows.h"
#include "../Core/InputHandler.h"
#include "../Core/Audio.h"
#include "../Renderer/Vulkan/VulkanMain.h"

class Application
{
public:
	Application();

	~Application() = default;

	void Run();
	static void HandleInput(Input& input);
	void		LoadAudio();
	void		PlaySoundIfNeeded();
	void HandleMessages();
	void Update();
	void Render();

private:
	Platform::WindowContext		  windowContext_;
	Platform::Win32				  win32_;
	Audio						  audio_;
	FMOD::Sound*				  sound{};
	GraphicsAPI::Vulkan::VkEngine engine_;

	bool shouldQuit;
	bool stopRendering_;
	bool resizeRequested;
};



#endif //APPLICATION_H
