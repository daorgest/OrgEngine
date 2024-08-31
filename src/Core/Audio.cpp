#include "Audio.h"
#include "Logger.h"
#include <tracy/Tracy.hpp>

Audio::Audio()
{
	// ZoneScopedC(tracy::Color::Red);
    InitAudioSystem();
}

Audio::~Audio()
{
    StopBackgroundPlayback();
    CleanUp();
}

void Audio::InitAudioSystem()
{
    result = FMOD::System_Create(&system);
    if (result != FMOD_OK)
    {
        LOG(ERR, "FMOD error! ({}) {}\n", result, FMOD_ErrorString(result));
        exit(-1);
    }

    result = system->init(512, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK)
    {
        LOG(ERR, "FMOD error! ({}) {}\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}

void Audio::LoadSound(const std::filesystem::path& filePath)
{
    if (!exists(filePath))
    {
    	return;
        throw std::runtime_error("File not found: " + filePath.string());
    }

    FMOD::Sound* sound = nullptr;
    result = system->createSound(filePath.string().c_str(), FMOD_DEFAULT, nullptr, &sound);
    if (result != FMOD_OK)
    {
        throw std::runtime_error("Failed to load sound: " + std::string(FMOD_ErrorString(result)));
    }

    sounds.push_back(sound);
}

void Audio::PlayGameSound(FMOD::Sound* sound)
{
    if (!system)
    {
        LOG(ERR, "FMOD SYSTEM NOT INITIALIZED!!");
        return;
    }

    result = system->playSound(sound, nullptr, false, &channel);
    if (result != FMOD_OK)
    {
        LOG(ERR, "Failed to play sound: {}\n", FMOD_ErrorString(result));
    }
    else
    {
        LOG(INFO, "Audio file is playing.");
    }
}

void Audio::StartBackgroundPlayback(FMOD::Sound* sound)
{
    shouldStopPlayback = false;
    audioThread = std::thread(&Audio::BackgroundPlayback, this, sound);

	tracy::SetThreadName("Background playback");
}

void Audio::StopBackgroundPlayback()
{
    shouldStopPlayback = true;
    if (audioThread.joinable())
    {
        audioThread.join();
    }
}

void Audio::BackgroundPlayback(FMOD::Sound* sound)
{
    while (!shouldStopPlayback)
    {
        bool isPlaying = false;
        if (channel == nullptr || channel->isPlaying(&isPlaying) != FMOD_OK || !isPlaying)
        {
            PlayGameSound(sound);
        }

        // Sleep for a short duration to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Audio::CleanUp()
{
    for (const auto sound : sounds)
    {
        if (sound)
        {
            sound->release();
        }
    }

    sounds.clear();

    if (system)
    {
        system->close();
        system->release();
    }
}
