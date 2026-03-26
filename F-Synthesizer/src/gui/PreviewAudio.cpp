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
    if (!playback->playing.load(std::memory_order_relaxed))
    {
        std::fill(out, out + static_cast<size_t>(frameCount) * channels, 0.0f);
        return;
    }

    const ma_uint64 totalFrames = playback->pcm.empty()
        ? 0
        : static_cast<ma_uint64>(playback->pcm.size() / channels);

    ma_uint32 written = 0;
    // コールバックスレッドではロックを持たず、frameCursorの原子的更新で追従する。
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

    // sampleRate変更時はデバイスを作り直し、再生ピッチずれを防ぐ。
    if (playback.deviceReady &&
        (playback.sampleRate != static_cast<ma_uint32>(sampleRate) ||
            playback.device.playback.channels != playback.channels))
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
    playback.playStartTick.store(0, std::memory_order_relaxed);
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
    if (sound.length <= 0 || (sound.dataL.empty() && sound.data.empty()))
    {
        err = "Preview buffer is empty.";
        return false;
    }
    playback.channels = (sound.channels >= 2) ? 2 : 1;
    if (!EnsurePreviewAudioDevice(playback, sound.fs, err))
    {
        return false;
    }

    // PCM書き換え中だけロックし、コールバック側は読み取り専用で動かす。
    std::lock_guard<std::mutex> lock(playback.mutex);
    playback.pcm.resize(static_cast<size_t>(sound.length) * playback.channels);
    for (int i = 0; i < sound.length; i++)
    {
        const double lRaw = (i < static_cast<int>(sound.dataL.size())) ? sound.dataL[i] : sound.data[i];
        const double rRaw = (i < static_cast<int>(sound.dataR.size())) ? sound.dataR[i] : sound.data[i];
        const double l = (std::max)(-1.0, (std::min)(1.0, lRaw));
        const double r = (std::max)(-1.0, (std::min)(1.0, rRaw));
        if (playback.channels == 1)
        {
            playback.pcm[i] = static_cast<float>((l + r) * 0.5);
        }
        else
        {
            const size_t base = static_cast<size_t>(i) * 2;
            playback.pcm[base + 0] = static_cast<float>(l);
            playback.pcm[base + 1] = static_cast<float>(r);
        }
    }
    playback.frameCursor.store(0, std::memory_order_relaxed);
    playback.loop.store(loop, std::memory_order_relaxed);
    playback.playing.store(true, std::memory_order_relaxed);
    return true;
}
