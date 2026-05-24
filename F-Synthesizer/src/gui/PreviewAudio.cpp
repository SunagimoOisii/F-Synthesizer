#define MINIAUDIO_IMPLEMENTATION

#include "gui/PreviewAudio.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

namespace
{
constexpr ma_uint64 kStreamingCapacityFrames = 44100 * 8;
constexpr ma_uint64 kStreamingStartupFramesAt44100 = 4410;

float ClampAudio(double v)
{
    return static_cast<float>((std::max)(-1.0, (std::min)(1.0, v)));
}

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
        const bool canResumeStream =
            playback->streamMode.load(std::memory_order_acquire) &&
            !playback->streamCompleted.load(std::memory_order_acquire) &&
            playback->streamAvailableFrames.load(std::memory_order_acquire) >= playback->streamStartupFrames;
        if (canResumeStream)
        {
            playback->streamUnderrun.store(false, std::memory_order_relaxed);
            playback->playing.store(true, std::memory_order_relaxed);
        }
        return;
    }

    if (playback->streamMode.load(std::memory_order_acquire))
    {
        ma_uint32 written = 0;
        float peak = 0.0f;
        while (written < frameCount)
        {
            ma_uint64 available = playback->streamAvailableFrames.load(std::memory_order_acquire);
            if (available == 0)
            {
                if (playback->streamCompleted.load(std::memory_order_acquire))
                {
                    const auto buffer = playback->pcmBuffer.load(std::memory_order_acquire);
                    if (playback->loop.load(std::memory_order_relaxed) && buffer && buffer->frameCount > 0)
                    {
                        playback->streamMode.store(false, std::memory_order_release);
                        playback->frameCursor.store(0, std::memory_order_relaxed);
                        break;
                    }
                    std::fill(out + written * channels, out + frameCount * channels, 0.0f);
                    playback->playing.store(false, std::memory_order_relaxed);
                    playback->streamPeak.store(peak, std::memory_order_relaxed);
                    return;
                }
                std::fill(out + written * channels, out + frameCount * channels, 0.0f);
                playback->streamUnderrun.store(true, std::memory_order_relaxed);
                playback->playing.store(false, std::memory_order_relaxed);
                playback->streamPeak.store(peak, std::memory_order_relaxed);
                return;
            }

            const ma_uint64 readFrame = playback->streamReadFrame.load(std::memory_order_relaxed);
            const ma_uint64 ringIndex = (playback->streamCapacityFrames > 0)
                ? (readFrame % playback->streamCapacityFrames)
                : 0;
            const ma_uint64 contiguous = playback->streamCapacityFrames - ringIndex;
            const ma_uint32 chunk = static_cast<ma_uint32>((std::min<uint64_t>)(
                (std::min<uint64_t>)(available, contiguous),
                frameCount - written));
            const size_t srcOffset = static_cast<size_t>(ringIndex * channels);
            const size_t dstOffset = static_cast<size_t>(written * channels);
            const size_t sampleCount = static_cast<size_t>(chunk * channels);
            std::copy_n(playback->streamRing.data() + srcOffset, sampleCount, out + dstOffset);
            for (size_t i = 0; i < sampleCount; i++)
            {
                peak = (std::max)(peak, std::abs(out[dstOffset + i]));
            }
            playback->streamReadFrame.store(readFrame + chunk, std::memory_order_relaxed);
            playback->streamAvailableFrames.fetch_sub(chunk, std::memory_order_release);
            written += chunk;
        }
        playback->streamPeak.store(peak, std::memory_order_relaxed);
        if (written >= frameCount)
        {
            return;
        }
    }

    std::shared_ptr<const PreviewPCMBuffer> buffer =
        playback->pcmBuffer.load(std::memory_order_acquire);
    const ma_uint64 activeSession = playback->sessionGeneration.load(std::memory_order_relaxed);
    if (!buffer || buffer->session != activeSession || buffer->channels != channels || buffer->pcm.empty())
    {
        std::fill(out, out + static_cast<size_t>(frameCount) * channels, 0.0f);
        playback->playing.store(false, std::memory_order_relaxed);
        return;
    }

    const ma_uint64 totalFrames = buffer->frameCount;

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
        std::copy_n(buffer->pcm.data() + srcOffset, sampleCount, out + dstOffset);
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
    playback.streamMode.store(false, std::memory_order_release);
    playback.streamCompleted.store(false, std::memory_order_release);
    playback.streamUnderrun.store(false, std::memory_order_relaxed);
    playback.frameCursor.store(0, std::memory_order_relaxed);
    playback.playStartTick.store(0, std::memory_order_relaxed);
    playback.sessionGeneration.fetch_add(1, std::memory_order_relaxed);
    playback.streamSession.fetch_add(1, std::memory_order_relaxed);
    playback.pcmBuffer.store({}, std::memory_order_release);
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

uint64_t StartStreamingPreviewAudio(
    PreviewPlaybackState& playback,
    int sampleRate,
    ma_uint32 channels,
    bool loop,
    std::string& err)
{
    playback.channels = (channels >= 2) ? 2 : 1;
    if (!EnsurePreviewAudioDevice(playback, sampleRate, err))
    {
        return 0;
    }

    StopPreviewAudio(playback);
    const ma_uint64 session = playback.streamSession.fetch_add(1, std::memory_order_relaxed) + 1;
    playback.streamCapacityFrames = (std::max<ma_uint64>)(static_cast<ma_uint64>(sampleRate) * 8, 1024);
    playback.streamStartupFrames = (std::max<ma_uint64>)(
        static_cast<ma_uint64>((static_cast<double>(sampleRate) * 0.10)),
        kStreamingStartupFramesAt44100 / 2);
    playback.streamRing.assign(static_cast<size_t>(playback.streamCapacityFrames * playback.channels), 0.0f);
    playback.streamArchive.clear();
    playback.streamArchive.reserve(static_cast<size_t>((std::min<ma_uint64>)(playback.streamCapacityFrames, kStreamingCapacityFrames) * playback.channels));
    playback.streamReadFrame.store(0, std::memory_order_relaxed);
    playback.streamWriteFrame.store(0, std::memory_order_relaxed);
    playback.streamAvailableFrames.store(0, std::memory_order_release);
    playback.streamCompleted.store(false, std::memory_order_release);
    playback.streamUnderrun.store(false, std::memory_order_relaxed);
    playback.streamPeak.store(0.0f, std::memory_order_relaxed);
    playback.loop.store(loop, std::memory_order_relaxed);
    playback.streamMode.store(true, std::memory_order_release);
    playback.sessionGeneration.store(session, std::memory_order_release);
    return session;
}

bool WriteStreamingPreviewFrame(
    PreviewPlaybackState& playback,
    uint64_t session,
    double left,
    double right)
{
    if (session == 0 ||
        playback.streamSession.load(std::memory_order_relaxed) != session ||
        !playback.streamMode.load(std::memory_order_acquire))
    {
        return false;
    }

    while (playback.streamAvailableFrames.load(std::memory_order_acquire) >= playback.streamCapacityFrames)
    {
        if (playback.streamSession.load(std::memory_order_relaxed) != session ||
            !playback.streamMode.load(std::memory_order_acquire))
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    const ma_uint64 writeFrame = playback.streamWriteFrame.load(std::memory_order_relaxed);
    const ma_uint64 ringIndex = writeFrame % playback.streamCapacityFrames;
    const size_t base = static_cast<size_t>(ringIndex * playback.channels);
    const float l = ClampAudio(left);
    const float r = ClampAudio(right);
    if (playback.channels == 1)
    {
        playback.streamRing[base] = (l + r) * 0.5f;
        playback.streamArchive.push_back(playback.streamRing[base]);
    }
    else
    {
        playback.streamRing[base + 0] = l;
        playback.streamRing[base + 1] = r;
        playback.streamArchive.push_back(l);
        playback.streamArchive.push_back(r);
    }
    playback.streamWriteFrame.store(writeFrame + 1, std::memory_order_relaxed);
    const ma_uint64 available = playback.streamAvailableFrames.fetch_add(1, std::memory_order_release) + 1;
    if (!playback.playing.load(std::memory_order_relaxed) && available >= playback.streamStartupFrames)
    {
        playback.streamUnderrun.store(false, std::memory_order_relaxed);
        playback.playing.store(true, std::memory_order_relaxed);
    }
    return true;
}

bool WriteStreamingPreviewFrames(
    PreviewPlaybackState& playback,
    uint64_t session,
    const double* interleavedStereo,
    int frameCount)
{
    if (frameCount <= 0)
    {
        return true;
    }
    if (interleavedStereo == nullptr)
    {
        return false;
    }
    for (int i = 0; i < frameCount; i++)
    {
        if (!WriteStreamingPreviewFrame(
            playback,
            session,
            interleavedStereo[static_cast<size_t>(i) * 2 + 0],
            interleavedStereo[static_cast<size_t>(i) * 2 + 1]))
        {
            return false;
        }
    }
    return true;
}

void CompleteStreamingPreviewAudio(PreviewPlaybackState& playback, uint64_t session, bool canceled)
{
    if (session == 0 || playback.streamSession.load(std::memory_order_relaxed) != session)
    {
        return;
    }
    if (canceled)
    {
        StopPreviewAudio(playback);
        return;
    }

    auto buffer = std::make_shared<PreviewPCMBuffer>();
    buffer->channels = playback.channels;
    buffer->sampleRate = playback.sampleRate;
    buffer->frameCount = static_cast<ma_uint64>(playback.streamArchive.size() / playback.channels);
    buffer->session = playback.sessionGeneration.load(std::memory_order_relaxed);
    buffer->pcm = playback.streamArchive;
    playback.pcmBuffer.store(std::static_pointer_cast<const PreviewPCMBuffer>(buffer), std::memory_order_release);
    playback.streamCompleted.store(true, std::memory_order_release);
    if (!playback.playing.load(std::memory_order_relaxed) &&
        playback.streamAvailableFrames.load(std::memory_order_acquire) > 0)
    {
        playback.playing.store(true, std::memory_order_relaxed);
    }
}

float GetPreviewPlaybackPeak(const PreviewPlaybackState& playback)
{
    return playback.streamPeak.load(std::memory_order_relaxed);
}
