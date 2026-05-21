#include "synth/Oscillator.h"

#include <algorithm>
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
        // 位相増分が無効な条件では補正を無効化し、元波形をそのまま返す。
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

double SampleWavePhase(WaveType type, double phase, double phaseInc, double pulseWidth)
{
    const double p = NormalizePhase(phase);
    // polyBLEP補正は位相増分を0..1の周期比として受け取る想定。
    const double dt = std::fabs(phaseInc);

    switch (type)
    {
    case WaveType::Sine:
        return std::sin(2.0 * kPi * p);

    case WaveType::Square:
    {
        const double pw = std::clamp(pulseWidth, 0.05, 0.95);
        double w = (p < pw) ? 1.0 : -1.0;
        w += PolyBlep(p, dt);
        w -= PolyBlep(NormalizePhase(p + 1.0 - pw), dt);
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

double SampleFmPhase(
    WaveType outputWave,
    WaveType inputWave,
    double carrierPhase,
    double modPhase,
    double carrierPhaseInc,
    double modPhaseInc,
    double modIndex)
{
    double mod = SampleWavePhase(inputWave, modPhase, modPhaseInc);
    double phaseOffset = (modIndex * mod) / (2.0 * kPi);
    double phase = carrierPhase + phaseOffset;
    phase -= std::floor(phase);
    return SampleWavePhase(outputWave, phase, carrierPhaseInc);
}

double SampleNoise(NoiseType type)
{
    // ノイズ状態は thread_local に保持し、オーディオ処理の並列化時に状態競合を避ける。
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
