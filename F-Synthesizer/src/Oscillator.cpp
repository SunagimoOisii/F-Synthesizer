#include "Oscillator.h"

#include <cmath>
#include <random>

const double kPi = std::acos(-1.0);

double SampleWavePhase(WaveType type, double phase)
{
    switch (type)
    {
    case WaveType::Sine:
        return std::sin(2.0 * kPi * phase);

    case WaveType::Square:
        return (phase < 0.5) ? 1.0 : -1.0;
        
    case WaveType::Saw:
        return 2.0 * phase - 1.0;

    case WaveType::Triangle:
        return 1.0 - 4.0 * std::fabs(phase - 0.5);

    default:
        return 0.0;
    }
}

double SampleFmPhase(double carrierPhase, double modPhase, double modIndex)
{
    double mod = std::sin(2.0 * kPi * modPhase);
    return std::sin(2.0 * kPi * carrierPhase + modIndex * mod);
}

double SampleNoise(NoiseType type)
{
    static thread_local std::mt19937 rng{ std::random_device{}() };
    switch (type)
    {
    case NoiseType::White:
    default:
    {
        static thread_local std::uniform_real_distribution<double> dist(-1.0, 1.0);
        return dist(rng);
    }
    }
}

double NoteNumberToFreq(int noteNumber)
{
    return 440.0 * std::pow(2.0, (noteNumber - 69) / 12.0);
}

double VelocityToGain(int velocity)
{
    if (velocity <= 0) return 0.0;
    if (velocity >= 127) return 1.0;
    return velocity / 127.0;
}
