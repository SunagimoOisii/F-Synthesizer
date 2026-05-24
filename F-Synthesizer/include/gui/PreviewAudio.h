#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/AudioBuffer.h"
#include "third_party/miniaudio.h"

struct PreviewPCMBuffer
{
    std::vector<float> pcm{};
    ma_uint32 channels = 2;
    ma_uint32 sampleRate = 44100;
    ma_uint64 frameCount = 0;
    uint64_t session = 0;
};

// GUIプレビュー再生のデバイス/PCM状態を保持する。
// コールバックは immutable PCM buffer の shared_ptr を掴んで読み出す。
struct PreviewPlaybackState
{
    ma_device device{};
    bool deviceReady = false;
    std::mutex mutex{};
    std::atomic<std::shared_ptr<const PreviewPCMBuffer>> pcmBuffer{};
    std::atomic<uint64_t> frameCursor{ 0 };
    std::atomic<int> playStartTick{ 0 };
    std::atomic<bool> playing{ false };
    std::atomic<bool> loop{ false };
    std::atomic<uint64_t> sessionGeneration{ 0 };
    ma_uint32 channels = 2;
    ma_uint32 sampleRate = 44100;
};

bool EnsurePreviewAudioDevice(PreviewPlaybackState& playback, int sampleRate, std::string& err);
void StopPreviewAudio(PreviewPlaybackState& playback);
void ShutdownPreviewAudio(PreviewPlaybackState& playback);
bool PlayPreviewAudio(PreviewPlaybackState& playback, const SoundData& sound, bool loop, std::string& err);
