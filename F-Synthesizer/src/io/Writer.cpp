#include <algorithm>
#include <cstdint>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "io/Writer.h"
#include "io/PlatformPaths.h"

struct Chunk
{
    char id[4];
};

namespace
{
void FillError(
    WAVWriteError* outError,
    const char* code,
    const std::filesystem::path& path,
    int err,
    unsigned long lastError,
    const char* cause,
    const char* hint)
{
    if (outError == nullptr)
    {
        return;
    }
    outError->code = code;
    outError->path = path;
    outError->errnoValue = err;
    outError->systemError = lastError;
    outError->cause = cause;
    outError->hint = hint;
}
} // namespace

bool SaveWAVFilePath(const SoundData& sound, const std::filesystem::path& filePath)
{
    return SaveWAVFilePath(sound, filePath, nullptr);
}

bool SaveWAVFilePath(const SoundData& sound, const std::filesystem::path& filePath, WAVWriteError* outError)
{
    std::cout << "[SaveWAVFilePath] begin: " << PathToUtf8(filePath) << std::endl;
    SetLastError(0);
    errno = 0;

    if (sound.bits != 16 && sound.bits != 24)
    {
        FillError(
            outError,
            "wav_invalid_bits",
            filePath,
            0,
            0,
            "bits must be 16 or 24",
            "set sound.bits to 16 or 24 before writing");
        return false;
    }
    if (sound.fs <= 0)
    {
        FillError(
            outError,
            "wav_invalid_samplerate",
            filePath,
            0,
            0,
            "sample rate must be greater than zero",
            "set sound.fs to a valid Hz value such as 44100");
        return false;
    }
    if (sound.channels < 1 || sound.channels > 2)
    {
        FillError(
            outError,
            "wav_invalid_channels",
            filePath,
            0,
            0,
            "channels must be 1 or 2",
            "set sound.channels to mono(1) or stereo(2)");
        return false;
    }
    if (sound.length <= 0)
    {
        FillError(
            outError,
            "wav_empty",
            filePath,
            0,
            0,
            "sound buffer has no samples",
            "render at least one frame before writing");
        return false;
    }

    std::ofstream fout(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout.is_open())
    {
        std::cout << "[SaveWAVFilePath] open failed"
            << " fail=" << fout.fail()
            << " bad=" << fout.bad()
            << " errno=" << errno
            << std::endl;
        const int err = errno;
        // errno が取れないケースでも、呼び出し側で判別できるよう固定の識別コードを補う。
        const unsigned long lastError = (unsigned long)((err != 0) ? err : 0xE001);
        SetLastError(lastError);
        FillError(
            outError,
            "wav_open_failed",
            filePath,
            err,
            lastError,
            "failed to open destination file",
            "close applications locking the file and verify the output folder is writable");
        return false;
    }

    const int channels = sound.channels;
    const int bytesPerSample = sound.bits / 8;
    std::vector<std::uint8_t> wdata(static_cast<size_t>(sound.length) * static_cast<size_t>(channels) * static_cast<size_t>(bytesPerSample));
    size_t writeOffset = 0;
    auto writeSample = [&](double x)
    {
        const double sample = std::clamp(x, -1.0, 1.0);
        if (sound.bits == 24)
        {
            const int32_t pcm24 = static_cast<int32_t>(std::clamp(sample * 8388607.0, -8388608.0, 8388607.0));
            wdata[writeOffset + 0] = static_cast<std::uint8_t>(pcm24 & 0xFF);
            wdata[writeOffset + 1] = static_cast<std::uint8_t>((pcm24 >> 8) & 0xFF);
            wdata[writeOffset + 2] = static_cast<std::uint8_t>((pcm24 >> 16) & 0xFF);
            writeOffset += 3;
        }
        else
        {
            const int32_t pcm16 = static_cast<int32_t>(std::clamp(sample * 32767.0, -32768.0, 32767.0));
            const int16_t s = static_cast<int16_t>(pcm16);
            wdata[writeOffset + 0] = static_cast<std::uint8_t>(s & 0xFF);
            wdata[writeOffset + 1] = static_cast<std::uint8_t>((s >> 8) & 0xFF);
            writeOffset += 2;
        }
    };
    for (int i = 0; i < sound.length; i++)
    {
        const size_t index = static_cast<size_t>(i);
        writeSample(sound.SampleL(index));
        if (channels >= 2)
        {
            writeSample(sound.SampleR(index));
        }
    }

    const int32_t wsize = static_cast<int32_t>(wdata.size());
    Chunk riffChunk = { 'R', 'I', 'F', 'F' };
    int32_t riffSize = 12 + (int32_t)sizeof(PCMWAVEFORMAT) + 8 + wsize;
    Chunk waveChunk = { 'W', 'A', 'V', 'E' };
    Chunk formatChunk = { 'f', 'm', 't', ' ' };
    int32_t fsize = (int32_t)sizeof(PCMWAVEFORMAT);
    PCMWAVEFORMAT wform{};
    wform.wf.wFormatTag = 1;
    wform.wf.nChannels = static_cast<WORD>(channels);
    wform.wf.nSamplesPerSec = (DWORD)sound.fs;
    wform.wf.nAvgBytesPerSec = (DWORD)(sound.bits * sound.fs * channels / 8);
    wform.wf.nBlockAlign = static_cast<WORD>((sound.bits / 8) * channels);
    wform.wBitsPerSample = (WORD)sound.bits;
    Chunk dataChunk = { 'd', 'a', 't', 'a' };
    int32_t dsize = wsize;

    fout.write((const char*)&riffChunk, sizeof(Chunk));
    fout.write((const char*)&riffSize, sizeof(int32_t));
    fout.write((const char*)&waveChunk, sizeof(Chunk));
    fout.write((const char*)&formatChunk, sizeof(Chunk));
    fout.write((const char*)&fsize, sizeof(int32_t));
    fout.write((const char*)&wform, sizeof(PCMWAVEFORMAT));
    fout.write((const char*)&dataChunk, sizeof(Chunk));
    fout.write((const char*)&dsize, sizeof(int32_t));
    fout.write((const char*)wdata.data(), (std::streamsize)wsize);

    fout.flush();
    if (!fout.good())
    {
        std::cout << "[SaveWAVFilePath] write failed"
            << " fail=" << fout.fail()
            << " bad=" << fout.bad()
            << " errno=" << errno
            << " wsize=" << wsize
            << std::endl;
        const int err = errno;
        // 書き込み失敗側の固定の識別コード。open失敗と区別して上位で表示を分ける。
        const unsigned long lastError = (unsigned long)((err != 0) ? err : 0xE002);
        SetLastError(lastError);
        FillError(
            outError,
            "wav_write_failed",
            filePath,
            err,
            lastError,
            "stream write failed while emitting WAV payload",
            "check disk free space and file permission, then retry");
        return false;
    }
    fout.close();
    if (outError != nullptr)
    {
        *outError = WAVWriteError{};
    }
    std::cout << "[SaveWAVFilePath] success" << std::endl;
    return true;
}
