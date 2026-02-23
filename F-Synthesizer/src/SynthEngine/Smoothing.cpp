#include "SynthEngine/Smoothing.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
double SanitizeFinite(double value, double fallback = 0.0)
{
    return std::isfinite(value) ? value : fallback;
}

double ClampWithRange(double value, const SmoothingRange& range)
{
    const double minV = SanitizeFinite(range.minValue, -1.0e300);
    const double maxV = SanitizeFinite(range.maxValue, 1.0e300);
    if (minV > maxV)
    {
        return std::clamp(value, maxV, minV);
    }
    return std::clamp(value, minV, maxV);
}

double ComputeAlpha(double timeMs, int sampleRate)
{
    if (!std::isfinite(timeMs) || timeMs <= 0.0 || sampleRate <= 0)
    {
        return 1.0;
    }
    const double dt = 1.0 / static_cast<double>(sampleRate);
    const double tau = timeMs * 0.001;
    const double alpha = 1.0 - std::exp(-dt / tau);
    return std::clamp(alpha, 0.0, 1.0);
}

void RecomputeAlpha(SmoothedParam& param)
{
    param.alpha = ComputeAlpha(param.timeMs, param.sampleRate);
}
} // namespace

void SetSmoothingRange(SmoothedParam& param, double minValue, double maxValue)
{
    param.range.minValue = SanitizeFinite(minValue, -1.0e300);
    param.range.maxValue = SanitizeFinite(maxValue, 1.0e300);
    param.current = ClampWithRange(SanitizeFinite(param.current), param.range);
    param.target = ClampWithRange(SanitizeFinite(param.target), param.range);
}

void SetSmoothingSampleRate(SmoothedParam& param, int sampleRate)
{
    param.sampleRate = sampleRate;
    RecomputeAlpha(param);
}

void SetSmoothingTimeMs(SmoothedParam& param, double timeMs)
{
    param.timeMs = std::isfinite(timeMs) ? (std::max)(0.0, timeMs) : 0.0;
    RecomputeAlpha(param);
}

void ResetSmoothedParam(SmoothedParam& param, double value)
{
    const double v = ClampWithRange(SanitizeFinite(value), param.range);
    param.current = v;
    param.target = v;
}

void SetSmoothedTarget(SmoothedParam& param, double target)
{
    param.target = ClampWithRange(SanitizeFinite(target), param.range);
}

double StepSmoothedParam(SmoothedParam& param)
{
    if (IsSmoothingBypassed(param))
    {
        param.current = param.target;
        return param.current;
    }

    param.current += param.alpha * (param.target - param.current);
    param.current = ClampWithRange(SanitizeFinite(param.current), param.range);
    return param.current;
}

bool IsSmoothingBypassed(const SmoothedParam& param)
{
    if (param.alpha >= 1.0)
    {
        return true;
    }
    const double diff = std::fabs(param.target - param.current);
    return diff <= std::numeric_limits<double>::epsilon();
}

