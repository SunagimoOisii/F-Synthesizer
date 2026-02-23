#include <cmath> //std::acos

#include "core/AudioBuffer.h"

const double kPi = std::acos(-1.0);

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
    for (size_t i = 0; i < data.size(); i++)
    {
        data[i] = 0.0;
    }
}