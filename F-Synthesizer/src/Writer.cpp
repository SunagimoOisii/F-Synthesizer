#include <climits>
#include <cstdint>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "Writer.h"

struct Chunk
{
    char id[4];
};

bool SaveWavFile(const SoundData& sound, const char* filePath)
{
    //出力ファイルを開く
    std::ofstream fout;
    fout.open(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout) return false;

    //量子化
    //モノラル、2バイト量子化を前提に保存する波形を生成
    std::vector<short> wdata(sound.data.size());
    for (int i = 0; i < sound.length; i++)
    {
        //振幅を[-1.0,1.0]にクリッピング
        double amp = (sound.data[i] > 1.0) ? 1.0 : sound.data[i];
        amp = (amp < -1.0) ? -1.0 : amp;
        wdata[i] = (short)(amp * SHRT_MAX);
    }

    //WAVヘッダ構築
    int32_t wsize       = (int32_t)(sizeof(short) * sound.length);
    Chunk   riffChunk   = { 'R', 'I', 'F', 'F' };
    int32_t riffSize    = 12 + (int32_t)sizeof(PCMWAVEFORMAT) + 8 + wsize;
    Chunk   waveChunk   = { 'W', 'A', 'V', 'E' };
    Chunk   formatChunk = { 'f', 'm', 't', ' ' };
    int32_t fsize       = (int32_t)sizeof(PCMWAVEFORMAT);
    PCMWAVEFORMAT wform{};
    wform.wf.wFormatTag      = 1; //非圧縮PCM指定
    wform.wf.nChannels       = 1; //モノラルのみ対応
    wform.wf.nSamplesPerSec  = (DWORD)sound.fs;
    wform.wf.nAvgBytesPerSec = (DWORD)(sound.bits * sound.fs / 8);
    wform.wf.nBlockAlign     = (WORD)(sound.bits / 8);
    wform.wBitsPerSample     = sound.bits;
    Chunk   dataChunk        = { 'd', 'a', 't', 'a' };
    int32_t dsize            = wsize;

    //WAV書き出し
    fout.write((char*)&riffChunk, sizeof(Chunk));
    fout.write((char*)&riffSize, sizeof(int32_t));
    fout.write((char*)&waveChunk, sizeof(Chunk));
    fout.write((char*)&formatChunk, sizeof(Chunk));
    fout.write((char*)&fsize, sizeof(int32_t));
    fout.write((char*)&wform, sizeof(PCMWAVEFORMAT));
    fout.write((char*)&dataChunk, sizeof(Chunk));
    fout.write((char*)&dsize, sizeof(int32_t));
    fout.write((char*)wdata.data(), wsize);
    fout.close();

    return true;
}
