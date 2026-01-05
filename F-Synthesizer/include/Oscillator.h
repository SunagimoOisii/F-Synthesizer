#pragma once

//位相で定義できる周期波形
enum class WaveType
{
    Sine,
    Square,
    Saw,
    Triangle,
    Noise
};

double SampleWavePhase(WaveType type, double phase);
double SampleFmPhase(double carrierPhase, double modPhase, double modIndex);

double NoteNumberToFreq(int noteNumber);
double VelocityToGain(int velocity);
