#include "core/AudioBuffer.h"

SoundData::SoundData()
    : SoundData(44100, 16, 44100)
{
}

SoundData::SoundData(int length, int bits, int fs)
    : length(length)
    , bits(bits)
    , fs(fs)
    , data(length)
{
}
