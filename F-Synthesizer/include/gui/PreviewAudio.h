#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "AudioBuffer.h"
#include "third_party/miniaudio.h"

struct PreviewPlaybackState
{
    ma_device device{};
    bool deviceReady = false;
    std::mutex mutex{};
    std::vector<float> pcm{};
    std::atomic<uint64_t> frameCursor{ 0 };
    std::atomic<bool> playing{ false };
    std::atomic<bool> loop{ false };
    ma_uint32 channels = 1;
    ma_uint32 sampleRate = 44100;
};

bool EnsurePreviewAudioDevice(PreviewPlaybackState& playback, int sampleRate, std::string& err);
void StopPreviewAudio(PreviewPlaybackState& playback);
void ShutdownPreviewAudio(PreviewPlaybackState& playback);
bool PlayPreviewAudio(PreviewPlaybackState& playback, const SoundData& sound, bool loop, std::string& err);
