#include "SynthEngine/Filter.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kMinQ = 0.1;
constexpr double kMaxQ = 18.0;
constexpr double kLog2 = 0.69314718055994530942;
constexpr double kCutoffUpdateThresholdCents = 1.0;
constexpr double kMinCutoffUpdateStepHz = 0.25;

double CalcCutoffUpdateEpsilonHz(double cutoffHz)
{
    // 係数再計算は trig を伴うため高コスト。
    // cutoff 更新を 1 cent 相当未満で間引き、ホットパスの再計算頻度を下げる。
    const double safeCutoff = (cutoffHz > 0.0) ? cutoffHz : 10.0;
    const double centsRatio = (kCutoffUpdateThresholdCents * kLog2) / 1200.0;
    return (std::max)(kMinCutoffUpdateStepHz, safeCutoff * centsRatio);
}

double ClampCutoffHz(double cutoffHz, int sampleRate)
{
    const int safeSampleRate = (sampleRate > 0) ? sampleRate : 44100;
    const double nyquist = static_cast<double>(safeSampleRate) * 0.5;
    const double minCutoff = 10.0;
    const double maxCutoff = (std::max)(minCutoff, nyquist * 0.49);
    return std::clamp(cutoffHz, minCutoff, maxCutoff);
}
} // namespace

FilterParams ClampFilterParams(const FilterParams& params, int sampleRate)
{
    FilterParams clamped = params;
    clamped.cutoffHz = ClampCutoffHz(params.cutoffHz, sampleRate);
    clamped.resonance = std::clamp(params.resonance, kMinQ, kMaxQ);
    return clamped;
}

void SetFilterSampleRate(FilterInstance& filter, int sampleRate)
{
    const int safeSampleRate = (sampleRate > 0) ? sampleRate : 44100;
    if (filter.sampleRate == safeSampleRate)
    {
        return;
    }
    filter.sampleRate = safeSampleRate;
    filter.dirty = true;
}

void SetFilterMode(FilterInstance& filter, FilterMode mode)
{
    if (filter.params.mode == mode)
    {
        return;
    }
    filter.params.mode = mode;
    filter.dirty = true;
}

void SetFilterCutoffHz(FilterInstance& filter, double cutoffHz)
{
    const double clamped = ClampCutoffHz(cutoffHz, filter.sampleRate);
    const double epsilonHz = CalcCutoffUpdateEpsilonHz((std::max)(filter.params.cutoffHz, clamped));
    if (std::fabs(filter.params.cutoffHz - clamped) < epsilonHz)
    {
        return;
    }
    filter.params.cutoffHz = clamped;
    filter.dirty = true;
}

void SetFilterResonance(FilterInstance& filter, double resonance)
{
    const double clamped = std::clamp(resonance, kMinQ, kMaxQ);
    if (std::fabs(filter.params.resonance - clamped) < 1e-12)
    {
        return;
    }
    filter.params.resonance = clamped;
    filter.dirty = true;
}

void ResetFilterState(FilterInstance& filter)
{
    filter.state = BiquadState{};
}

void UpdateFilterCoefficients(FilterInstance& filter)
{
    if (!filter.dirty)
    {
        return;
    }

    const FilterParams p = ClampFilterParams(filter.params, filter.sampleRate);
    filter.params = p;

    if (p.mode == FilterMode::Bypass)
    {
        filter.coeffs = BiquadCoefficients{};
        filter.dirty = false;
        return;
    }

    const double w0 = 2.0 * kPi * p.cutoffHz / static_cast<double>(filter.sampleRate);
    const double cosW0 = std::cos(w0);
    const double sinW0 = std::sin(w0);
    const double alpha = sinW0 / (2.0 * p.resonance);

    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosW0;
    double a2 = 1.0 - alpha;

    switch (p.mode)
    {
    case FilterMode::LowPass:
        b0 = (1.0 - cosW0) * 0.5;
        b1 = 1.0 - cosW0;
        b2 = (1.0 - cosW0) * 0.5;
        break;
    case FilterMode::HighPass:
        b0 = (1.0 + cosW0) * 0.5;
        b1 = -(1.0 + cosW0);
        b2 = (1.0 + cosW0) * 0.5;
        break;
    case FilterMode::BandPass:
        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
        break;
    case FilterMode::Bypass:
    default:
        break;
    }

    const double invA0 = (std::fabs(a0) > 1e-12) ? (1.0 / a0) : 1.0;
    filter.coeffs.b0 = b0 * invA0;
    filter.coeffs.b1 = b1 * invA0;
    filter.coeffs.b2 = b2 * invA0;
    filter.coeffs.a1 = a1 * invA0;
    filter.coeffs.a2 = a2 * invA0;
    filter.dirty = false;
}

double ProcessFilterSample(FilterInstance& filter, double input)
{
    UpdateFilterCoefficients(filter);
    if (filter.params.mode == FilterMode::Bypass)
    {
        return input;
    }

    // Transposed Direct Form II:
    // 低コストで状態更新でき、汎用フィルタ層の共通実装に向く。
    const BiquadCoefficients& c = filter.coeffs;
    double y = c.b0 * input + filter.state.z1;
    filter.state.z1 = c.b1 * input - c.a1 * y + filter.state.z2;
    filter.state.z2 = c.b2 * input - c.a2 * y;
    return y;
}
