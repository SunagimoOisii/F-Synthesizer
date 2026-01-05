#pragma once

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
