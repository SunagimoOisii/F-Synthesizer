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

double SampleFmPhase(WaveType carrierWave, double carrierPhase, double modPhase, double modIndex)
{
    double mod = std::sin(2.0 * kPi * modPhase);
    double phaseOffset = (modIndex * mod) / (2.0 * kPi);
    double phase = carrierPhase + phaseOffset;
    phase -= std::floor(phase);
    return SampleWavePhase(carrierWave, phase);
}

double SampleNoise(NoiseType type)
{
    static thread_local std::mt19937 rng{ std::random_device{}() };
    static thread_local std::uniform_real_distribution<double> dist(-1.0, 1.0);
    static thread_local double pinkState = 0.0;
    static thread_local double brownState = 0.0;
    static thread_local double prevWhite = 0.0;

    double white = dist(rng);
    switch (type)
    {
    case NoiseType::White:
        return white;

    case NoiseType::Pink:
        // Simple 1/f approximation via leaky integrator
        pinkState = 0.98 * pinkState + 0.02 * white;
        return pinkState * 3.0;

    case NoiseType::Brown:
        // 1/f^2 approximation by integrating white noise
        brownState += 0.02 * white;
        if (brownState > 1.0) brownState = 1.0;
        if (brownState < -1.0) brownState = -1.0;
        return brownState;

    case NoiseType::Blue:
        // High-frequency emphasis via differentiator
        {
            double blue = white - prevWhite;
            prevWhite = white;
            return blue * 0.7;
        }

    default:
        return white;
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
