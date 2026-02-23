#include "Oscillator.h"

#include <cmath>
#include <random>

const double kPi = std::acos(-1.0);

namespace
{
double NormalizePhase(double phase)
{
    phase -= std::floor(phase);
    if (phase < 0.0)
    {
        phase += 1.0;
    }
    return phase;
}

double PolyBlep(double t, double dt)
{
    if (dt <= 0.0 || dt >= 1.0)
    {
        return 0.0;
    }
    if (t < dt)
    {
        const double x = t / dt;
        return x + x - x * x - 1.0;
    }
    if (t > 1.0 - dt)
    {
        const double x = (t - 1.0) / dt;
        return x * x + x + x + 1.0;
    }
    return 0.0;
}
} // namespace

double SampleWavePhase(WaveType type, double phase, double phaseInc)
{
    const double p = NormalizePhase(phase);
    const double dt = std::fabs(phaseInc);

    switch (type)
    {
    case WaveType::Sine:
        return std::sin(2.0 * kPi * p);

    case WaveType::Square:
    {
        double w = (p < 0.5) ? 1.0 : -1.0;
        w += PolyBlep(p, dt);
        w -= PolyBlep(NormalizePhase(p + 0.5), dt);
        return w;
    }
        
    case WaveType::Saw:
    {
        const double w = 2.0 * p - 1.0;
        return w - PolyBlep(p, dt);
    }

    case WaveType::Triangle:
        return 1.0 - 4.0 * std::fabs(p - 0.5);

    default:
        return 0.0;
    }
}

double SampleFmPhase(WaveType carrierWave, WaveType modWave, double carrierPhase, double modPhase, double modIndex)
{
    double mod = SampleWavePhase(modWave, modPhase);
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
