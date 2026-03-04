#include "core/AudioBuffer.h"

SoundData::SoundData()
    : SoundData(44100, 16, 44100)
{
    // 既定構築時も有効なWAV出力条件(44.1kHz/16bit)を満たす初期値にそろえる。
}

SoundData::SoundData(int length, int bits, int fs)
    : length(length)
    , bits(bits)
    , fs(fs)
    , data(length)
{
}
