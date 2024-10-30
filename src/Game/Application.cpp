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
    // glEngine_.Cleanup();
#endif
}

void Application::Run()
{
    if (!initialized_)
    {
        LOG(ERR, "Engine not initialized. Exiting...");
        return;
    }
#ifdef VULKAN_BUILD
	ShowWindow(windowContext_.hwnd, SW_SHOW);
	SetForegroundWindow(windowContext_.hwnd);
	SetFocus(windowContext_.hwnd);

    vkEngine_.Run();
#elif defined(OPENGL_BUILD)
    glEngine_.RenderLoop();
#endif
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
