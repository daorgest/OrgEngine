//
// Created by Orgest on 8/29/2024.
//

#ifndef AUDIO_H
#define AUDIO_H

#include <filesystem>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <thread>
#include <vector>

class Audio
{
public:
	Audio();
	~Audio();

	void InitAudioSystem();
	void LoadSound(const std::filesystem::path& filePath);
	void PlayGameSound(FMOD::Sound* sound);
	void StartBackgroundPlayback(FMOD::Sound* sound);
	void StopBackgroundPlayback();
	[[nodiscard]] FMOD::Channel* GetChannel() const { return channel; };
	void CleanUp();
	std::vector<FMOD::Sound*> sounds;
private:
	FMOD::System* system = nullptr;
	FMOD::Channel* channel = nullptr;
	FMOD_RESULT result = {};
	std::thread audioThread;
	std::atomic<bool> shouldStopPlayback = false;

	void BackgroundPlayback(FMOD::Sound* sound);
};



#endif //AUDIO_H
