#define MINIAUDIO_IMPLEMENTATION

#include "gui/PreviewAudio.h"

#include <algorithm>

namespace
{
void PreviewAudioCallback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount)
{
    auto* playback = reinterpret_cast<PreviewPlaybackState*>(device->pUserData);
    float* out = reinterpret_cast<float*>(output);
    if (out == nullptr || playback == nullptr)
    {
        return;
    }

    const ma_uint32 channels = (playback->channels > 0) ? playback->channels : 1;
    const ma_uint64 totalFrames = playback->pcm.empty()
        ? 0
        : static_cast<ma_uint64>(playback->pcm.size() / channels);

    ma_uint32 written = 0;
    while (written < frameCount)
    {
        ma_uint64 cursor = playback->frameCursor.load(std::memory_order_relaxed);
        if (cursor >= totalFrames)
        {
            if (playback->loop.load(std::memory_order_relaxed) && totalFrames > 0)
            {
                playback->frameCursor.store(0, std::memory_order_relaxed);
                cursor = 0;
            }
            else
            {
                std::fill(out + written * channels, out + frameCount * channels, 0.0f);
                playback->playing.store(false, std::memory_order_relaxed);
                return;
            }
        }

        const ma_uint64 remain = totalFrames - cursor;
        const ma_uint32 chunk = static_cast<ma_uint32>((std::min<uint64_t>)(remain, frameCount - written));
        const size_t srcOffset = static_cast<size_t>(cursor * channels);
        const size_t dstOffset = static_cast<size_t>(written * channels);
        const size_t sampleCount = static_cast<size_t>(chunk * channels);
        std::copy_n(playback->pcm.data() + srcOffset, sampleCount, out + dstOffset);
        playback->frameCursor.store(cursor + chunk, std::memory_order_relaxed);
        written += chunk;
    }
}
} // namespace

bool EnsurePreviewAudioDevice(PreviewPlaybackState& playback, int sampleRate, std::string& err)
{
    if (sampleRate <= 0)
    {
        err = "invalid sample rate";
        return false;
    }

    if (playback.deviceReady && playback.sampleRate != static_cast<ma_uint32>(sampleRate))
    {
        ma_device_uninit(&playback.device);
        playback.deviceReady = false;
    }
    if (playback.deviceReady)
    {
        return true;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = playback.channels;
    config.sampleRate = static_cast<ma_uint32>(sampleRate);
    config.dataCallback = PreviewAudioCallback;
    config.pUserData = &playback;

    if (ma_device_init(nullptr, &config, &playback.device) != MA_SUCCESS)
    {
        err = "failed to initialize preview audio device";
        return false;
    }
    if (ma_device_start(&playback.device) != MA_SUCCESS)
    {
        ma_device_uninit(&playback.device);
        err = "failed to start preview audio device";
        return false;
    }

    playback.sampleRate = static_cast<ma_uint32>(sampleRate);
    playback.deviceReady = true;
    return true;
}

void StopPreviewAudio(PreviewPlaybackState& playback)
{
    playback.playing.store(false, std::memory_order_relaxed);
    playback.frameCursor.store(0, std::memory_order_relaxed);
}

void ShutdownPreviewAudio(PreviewPlaybackState& playback)
{
    StopPreviewAudio(playback);
    if (playback.deviceReady)
    {
        ma_device_uninit(&playback.device);
        playback.deviceReady = false;
    }
}

bool PlayPreviewAudio(PreviewPlaybackState& playback, const SoundData& sound, bool loop, std::string& err)
{
    if (sound.data.empty())
    {
        err = "Preview buffer is empty.";
        return false;
    }
    if (!EnsurePreviewAudioDevice(playback, sound.fs, err))
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(playback.mutex);
    playback.pcm.resize(sound.data.size());
    for (size_t i = 0; i < sound.data.size(); i++)
    {
        const double clamped = (std::max)(-1.0, (std::min)(1.0, sound.data[i]));
        playback.pcm[i] = static_cast<float>(clamped);
    }
    playback.frameCursor.store(0, std::memory_order_relaxed);
    playback.loop.store(loop, std::memory_order_relaxed);
    playback.playing.store(true, std::memory_order_relaxed);
    return true;
}
