#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "third_party/miniaudio.h"
#include "AppCore.h"

// GUIプレビュー再生のデバイス/PCM状態を保持する。
// 生成スレッドと出力コールバックの間を、小さなSPSCリングで接続する。
struct PreviewPlaybackState
{
    ma_device device{};
    bool deviceReady = false;
    std::mutex mutex{};
    std::vector<float> streamRing{};
    std::atomic<uint64_t> streamLoopFrames{ 0 };
    std::atomic<uint64_t> streamReadFrame{ 0 };
    std::atomic<uint64_t> streamWriteFrame{ 0 };
    std::atomic<uint64_t> streamAvailableFrames{ 0 };
    std::atomic<uint64_t> streamSession{ 0 };
    std::atomic<bool> streamMode{ false };
    std::atomic<bool> streamCompleted{ false };
    std::atomic<bool> streamUnderrun{ false };
    std::atomic<float> streamPeak{ 0.0f };
    ma_uint64 streamCapacityFrames = 0;
    ma_uint64 streamStartupFrames = 0;
    std::atomic<uint64_t> frameCursor{ 0 };
    std::atomic<int> playStartTick{ 0 };
    std::atomic<bool> playing{ false };
    std::atomic<uint64_t> sessionGeneration{ 0 };
    ma_uint32 channels = 2;
    ma_uint32 sampleRate = 44100;
};

bool EnsurePreviewAudioDevice(PreviewPlaybackState& playback, int sampleRate, std::string& err);
void StopPreviewAudio(PreviewPlaybackState& playback);
void ShutdownPreviewAudio(PreviewPlaybackState& playback);
uint64_t StartStreamingPreviewAudio(
    PreviewPlaybackState& playback,
    int sampleRate,
    ma_uint32 channels,
    bool loop,
    std::string& err);
bool WriteStreamingPreviewFrame(
    PreviewPlaybackState& playback,
    uint64_t session,
    double left,
    double right);
bool WriteStreamingPreviewFrames(
    PreviewPlaybackState& playback,
    uint64_t session,
    const double* interleavedStereo,
    int frameCount);
void CompleteStreamingPreviewAudio(PreviewPlaybackState& playback, uint64_t session, bool canceled);
float GetPreviewPlaybackPeak(const PreviewPlaybackState& playback);

class PreviewAudioStreamSink final : public IPreviewStreamSink
{
public:
    PreviewAudioStreamSink(PreviewPlaybackState& playback, int startTick)
        : playback_(playback), startTick_(startTick) {}
    ~PreviewAudioStreamSink() override;
    bool Begin(int sampleRate, int channels, int totalFrames, bool loop) override;
    bool WriteFrame(double left, double right) override;
    bool WriteFrames(const double* frames, int frameCount) override;
    void Complete(bool canceled) override;
private:
    PreviewPlaybackState& playback_;
    int startTick_;
    uint64_t session_ = 0;
    bool completed_ = false;
};
