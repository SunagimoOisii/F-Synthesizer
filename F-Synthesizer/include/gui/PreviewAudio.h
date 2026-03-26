#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "core/AudioBuffer.h"
#include "third_party/miniaudio.h"

// GUIプレビュー再生のデバイス/PCM状態を保持する。
// Render完了後のSoundDataをPCM化して、コールバックスレッドから読み出す。
struct PreviewPlaybackState
{
    ma_device device{};
    bool deviceReady = false;
    std::mutex mutex{};
    std::vector<float> pcm{};
    std::atomic<uint64_t> frameCursor{ 0 };
    std::atomic<int> playStartTick{ 0 };
    std::atomic<bool> playing{ false };
    std::atomic<bool> loop{ false };
    ma_uint32 channels = 2;
    ma_uint32 sampleRate = 44100;
};

bool EnsurePreviewAudioDevice(PreviewPlaybackState& playback, int sampleRate, std::string& err);
void StopPreviewAudio(PreviewPlaybackState& playback);
void ShutdownPreviewAudio(PreviewPlaybackState& playback);
bool PlayPreviewAudio(PreviewPlaybackState& playback, const SoundData& sound, bool loop, std::string& err);
