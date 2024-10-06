//
// Created by Orgest on 8/26/2024.
//

#include "Application.h"

Application::Application()
{

    initialized_ = InitializeEngine();

    if (initialized_)
    {
        LoadAudio();
        audio_.StartBackgroundPlayback(sound);
    }
    else
    {
        LOG(ERR, "Failed to initialize the engine.");
    }
}

Application::~Application()
{
    CleanupEngine();
}

bool Application::InitializeEngine()
{
#ifdef VULKAN_BUILD
    if (!win32_.Init())
    {
        LOG(ERR, "Failed to initialize Win32.");
        return false;
    }

    if (!vkEngine_.Init())
    {
        LOG(ERR, "Failed to initialize Vulkan engine.");
        return false;
    }
#elif defined(OPENGL_BUILD)
    if (!glEngine_.Init())
    {
        LOG(ERR, "Failed to initialize OpenGL engine.");
        return false;
    }
#endif
    return true;
}

void Application::CleanupEngine()
{
#ifdef VULKAN_BUILD
    vkEngine_.Cleanup();
#elif defined(OPENGL_BUILD)
    glEngine_.Cleanup();
#endif
}

void Application::Run()
{
    if (!initialized_)
    {
        LOG(ERR, "Engine not initialized. Exiting...");
        return;
    }

	ShowWindow(windowContext_.hwnd, SW_SHOW);
	SetForegroundWindow(windowContext_.hwnd);
	SetFocus(windowContext_.hwnd);

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

	for (auto& controller : input.controllers)
	{
		controller.Update();
	}
    // Generalized input handler
    HandleControllerInput(input);
    HandleKeyboardAndMouseInput(input);

    // Process input after checking (reset states, etc.)
    processInputAfter(input);
}

void Application::HandleControllerInput(Input& input)
{
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
}

void Application::HandleKeyboardAndMouseInput(Input& input)
{
    // Check if the 'Space' key is pressed (keyboard specific)
    if (input.isKeyPressed(KeyboardButton::Keys::Space))
    {
        LOG(INFO, "The 'Space' key on the keyboard has been pressed.");
    }

    // Check if the left mouse button is pressed
    if (Input::isMouseButtonPressed(input.lMouseButton))
    {
        LOG(INFO, "The left mouse button has been pressed.");
    }
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

void Application::Update()
{
#ifdef VULKAN_BUILD
    // Start ImGui new frame for Vulkan
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    // Render ImGui components
    vkEngine_.RenderUI();
    ImGui::Render();
#elif defined(OPENGL_BUILD)
    // Start ImGui new frame for OpenGL
    // ImGui_ImplOpenGL3_NewFrame();
    // ImGui::NewFrame();
    // ImGui::ShowDemoWindow();
    //
    // // Render ImGui components
    // // glEngine_.RenderUI();
    // ImGui::Render();
#endif
}

void Application::Render()
{
#ifdef VULKAN_BUILD
    // Draw the frame for Vulkan
    vkEngine_.Draw();
#elif defined(OPENGL_BUILD)
    // Draw the frame for OpenGL
    // glEngine_.Draw();
#endif
}
