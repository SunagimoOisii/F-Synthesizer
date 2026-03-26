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
    SoundData(int length, int bits, int fs, int channels);

    // length はフレーム数（1chあたりサンプル数）。
    int length;
    int bits;
    int fs; //サンプリングレート(Hz)
    int channels;
    // 互換維持用のモノラル合成バッファ（(L+R)*0.5）。
    std::vector<double> data;
    std::vector<double> dataL;
    std::vector<double> dataR;
};
