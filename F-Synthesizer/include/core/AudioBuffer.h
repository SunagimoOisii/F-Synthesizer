#pragma once

#include <vector>

// レンダリング結果を保持するモノラルPCMバッファ。
// app/core/io の境界で共通利用し、data.size() は length と一致する前提で扱う。
class SoundData
{
public:
    // 既定は 1秒分(44.1kHz)の16bitバッファを確保する。
    SoundData();
    SoundData(int length, int bits, int fs);

    int length;
    int bits;
    int fs; //サンプリングレート(Hz)
    std::vector<double> data;
};
