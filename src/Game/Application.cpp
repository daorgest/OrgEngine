//
// Created by Orgest on 8/26/2024.
//

#include "Application.h"


Application::Application() :
#ifdef VULKAN_BUILD
	win32_(&windowContext_),
	vkEngine_(&windowContext_)
#elif defined(OPENGL_BUILD)
	glEngine_(&windowContext_)
#endif
{
	// if (initialized_)
	// {
	// 	LoadAudio();
	// 	audio_.StartBackgroundPlayback(sound);
	// }
}

Application::~Application() { vkEngine_.Cleanup(); }

// bool Application::InitializeEngine()
// {
// #elif defined(OPENGL_BUILD)
// 	if (!glEngine_.Init())
// 	{
// 		LOG(ERR, "Failed to initialize OpenGL engine.");
// 		return false;
// 	}
// #endif
// 	return true;
// }


void Application::Run()
{
	// if (!initialized_)
	// {
	// 	LOG(ERR, "Engine not initialized. Exiting...");
	// 	return;
	// }

#ifdef VULKAN_BUILD
	vkEngine_.Run();
#elif defined(OPENGL_BUILD)
	glEngine_.RenderLoop();
#endif
}

void Application::HandleInput(Input& input)
{
	// Update the controller usage status
	input.updateUsingController();

	// Check if the 'A' button on the controller or the 'A' key on the keyboard is pressed
	if (input.isInputActive(KeyboardButton::Keys::A, ControllerButton::XboxButton::A))
	{
		LOG(INFO, "The 'A' button/key has been pressed.");
	}

	// Check if the controller's 'B' button is pressed (controller specific)
	if (input.isControllerButtonPressed(ControllerButton::XboxButton::B))
	{
		LOG(INFO, "The 'B' button on the controller has been pressed.");
	}

	// Check if the 'Space' key is pressed (keyboard specific)
	if (input.isKeyPressed(KeyboardButton::Keys::Space))
	{
		LOG(INFO, "The 'Space' key on the keyboard has been pressed.");
	}

	// Check if the left mouse button is pressed
	if (input.isMouseButtonPressed(input.lMouseButton))
	{
		LOG(INFO, "The left mouse button has been pressed.");
	}

	// Process input after checking (reset states, etc.)
	processInputAfter(input);
}

void Application::LoadAudio()
{
	audio_.LoadSound("audio/mementomori.mp3");
	sound = audio_.sounds[0];
}

void Application::PlaySoundIfNeeded()
{
	static FMOD::Channel* channel = audio_.GetChannel();
	bool isPlaying = false;

	if (channel == nullptr || channel->isPlaying(&isPlaying) != FMOD_OK || !isPlaying)
	{
		audio_.PlayGameSound(sound);
		LOG(INFO, "Audio file is playing: mementomori.mp3");
	}
}

// void Application::Update()
// {
// 	// Start ImGui new frame
// 	ImGui_ImplVulkan_NewFrame();
// 	ImGui_ImplWin32_NewFrame();
// 	ImGui::NewFrame();
// 	ImGui::ShowDemoWindow();
//
// 	// Render ImGui components
// 	vkEngine_.RenderUI();
// 	ImGui::Render();
// }

// void Application::Render()
// {
// 	// Draw the frame
// 	vkEngine_.Draw();
// }
