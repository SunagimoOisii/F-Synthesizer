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
    White
};

double SampleWavePhase(WaveType type, double phase);
double SampleFmPhase(double carrierPhase, double modPhase, double modIndex);
double SampleNoise(NoiseType type);

double NoteNumberToFreq(int noteNumber);
double VelocityToGain(int velocity);
