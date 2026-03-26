#include "core/AudioBuffer.h"

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
    : length(length)
    , bits(bits)
    , fs(fs)
    , channels((channels >= 1) ? channels : 1)
    , data(length)
    , dataL(length)
    , dataR(length)
{
}
