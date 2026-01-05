#pragma once

#include <vector>

class SoundData
{
public:
    SoundData();
    SoundData(int length, int bits, int fs);

    int length;
    int bits;
    int fs; //サンプリングレート(Hz)
    std::vector<double> data;
};
