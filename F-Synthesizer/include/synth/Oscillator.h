#pragma once

//位相で定義できる周期波形
enum class WaveType
{
    Sine,
    Square,
    Saw,
    Triangle
};

enum class NoiseType
{
    White,
    Pink,
    Brown,
    Blue
};

double SampleWavePhase(
    WaveType type,
    double phase,
    double phaseInc = 0.0);
// 目的: FM位相変調つきで1サンプルを生成する。
// 前提: carrierPhase/modPhase は周期位相。modIndex はラジアン換算の変調量として扱う。
double SampleFmPhase(
    WaveType carrierWave,
    WaveType modWave,
    double carrierPhase,
    double modPhase,
    double carrierPhaseInc,
    double modPhaseInc,
    double modIndex);
// スレッドごとに独立した乱数状態を使うため、同じ呼び出し列でもスレッド間で値は一致しない。
double SampleNoise(NoiseType type);

double NoteNumberToFreq(int noteNumber);
double VelocityToGain(int velocity);
