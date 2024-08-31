//
// Created by Orgest on 8/26/2024.
//

#include "Application.h"


Application::Application() :
	win32_(&windowContext_), engine_(&windowContext_), shouldQuit(false), stopRendering_(false), resizeRequested(false)
{
	LoadAudio();
	audio_.StartBackgroundPlayback(sound);
}


void Application::Run()
{
	engine_.Run();
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


void Application::HandleMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			shouldQuit = true;
			return;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_SYSCOMMAND)
		{
			if (msg.wParam == SC_MINIMIZE)
			{
				stopRendering_ = true;
			}
			else if (msg.wParam == SC_RESTORE)
			{
				stopRendering_ = false;
				resizeRequested = true;
			}
		}

		// Handle window size changes
		if (msg.message == WM_SIZE)
		{
			if (msg.wParam != SIZE_MINIMIZED)
			{
				resizeRequested = true;
			}
		}
	}
}

void Application::Update()
{
	// Start ImGui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();

	// Render ImGui components
	engine_.RenderUI();
	ImGui::Render();
}

void Application::Render()
{
	// Draw the frame
	engine_.Draw();
}
