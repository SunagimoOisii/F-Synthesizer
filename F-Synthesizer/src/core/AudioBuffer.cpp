#include "core/AudioBuffer.h"

#include <cstddef>

SoundData::SoundData()
    : SoundData(44100, 16, 44100, 2)
{
    // 既定構築時も有効なWAV出力条件(44.1kHz/16bit)を満たす初期値にそろえる。
}

SoundData::SoundData(int length, int bits, int fs)
    : SoundData(length, bits, fs, 2)
{
}

SoundData::SoundData(int length, int bits, int fs, int channels)
    : length((length > 0) ? length : 0)
    , bits(bits)
    , fs(fs)
    , channels((channels >= 2) ? 2 : 1)
{
    const size_t frameCount = static_cast<size_t>(this->length);
    // source-of-truth を channels で一本化する:
    // mono(1ch) は data のみ、stereo(2ch) は dataL/dataR のみを保持する。
    if (this->channels >= 2)
    {
        dataL.resize(frameCount);
        dataR.resize(frameCount);
    }
    else
    {
        data.resize(frameCount);
    }
}
