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
double SampleFmPhase(
    WaveType carrierWave,
    WaveType modWave,
    double carrierPhase,
    double modPhase,
    double carrierPhaseInc,
    double modPhaseInc,
    double modIndex);
double SampleNoise(NoiseType type);

double NoteNumberToFreq(int noteNumber);
double VelocityToGain(int velocity);
