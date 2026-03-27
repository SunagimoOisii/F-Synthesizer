#include <algorithm>
#include <climits>
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

    const int channels = (sound.channels >= 2) ? 2 : 1;
    std::vector<short> wdata(static_cast<size_t>(sound.length) * channels);
    for (int i = 0; i < sound.length; i++)
    {
        const double l = (i < static_cast<int>(sound.dataL.size())) ? sound.dataL[i] : sound.data[i];
        const double r = (i < static_cast<int>(sound.dataR.size())) ? sound.dataR[i] : sound.data[i];
        auto toPcm16 = [](double x) -> short
        {
            const double amp = std::clamp(x, -1.0, 1.0);
            return static_cast<short>(amp * SHRT_MAX);
        };
        if (channels == 1)
        {
            wdata[i] = toPcm16(sound.data[i]);
        }
        else
        {
            const size_t base = static_cast<size_t>(i) * 2;
            wdata[base + 0] = toPcm16(l);
            wdata[base + 1] = toPcm16(r);
        }
    }

    int32_t wsize = static_cast<int32_t>(sizeof(short) * wdata.size());
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
