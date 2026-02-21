#include <climits>
#include <cstdint>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "Writer.h"

struct Chunk
{
    char id[4];
};

namespace
{
void FillError(
    WavWriteError* outError,
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

bool SaveWavFilePath(const SoundData& sound, const std::filesystem::path& filePath)
{
    return SaveWavFilePath(sound, filePath, nullptr);
}

bool SaveWavFilePath(const SoundData& sound, const std::filesystem::path& filePath, WavWriteError* outError)
{
    std::cout << "[SaveWavFilePath] begin: " << filePath.string() << std::endl;
    SetLastError(0);
    errno = 0;
    std::ofstream fout(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout.is_open())
    {
        std::cout << "[SaveWavFilePath] open failed"
            << " fail=" << fout.fail()
            << " bad=" << fout.bad()
            << " errno=" << errno
            << std::endl;
        const int err = errno;
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

    std::vector<short> wdata(sound.data.size());
    for (int i = 0; i < sound.length; i++)
    {
        double amp = (sound.data[i] > 1.0) ? 1.0 : sound.data[i];
        amp = (amp < -1.0) ? -1.0 : amp;
        wdata[i] = (short)(amp * SHRT_MAX);
    }

    int32_t wsize = (int32_t)(sizeof(short) * sound.length);
    Chunk riffChunk = { 'R', 'I', 'F', 'F' };
    int32_t riffSize = 12 + (int32_t)sizeof(PCMWAVEFORMAT) + 8 + wsize;
    Chunk waveChunk = { 'W', 'A', 'V', 'E' };
    Chunk formatChunk = { 'f', 'm', 't', ' ' };
    int32_t fsize = (int32_t)sizeof(PCMWAVEFORMAT);
    PCMWAVEFORMAT wform{};
    wform.wf.wFormatTag = 1;
    wform.wf.nChannels = 1;
    wform.wf.nSamplesPerSec = (DWORD)sound.fs;
    wform.wf.nAvgBytesPerSec = (DWORD)(sound.bits * sound.fs / 8);
    wform.wf.nBlockAlign = (WORD)(sound.bits / 8);
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
        std::cout << "[SaveWavFilePath] write failed"
            << " fail=" << fout.fail()
            << " bad=" << fout.bad()
            << " errno=" << errno
            << " wsize=" << wsize
            << std::endl;
        const int err = errno;
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
        *outError = WavWriteError{};
    }
    std::cout << "[SaveWavFilePath] success" << std::endl;
    return true;
}
