#include "SynthEngine/Smoothing.h"

#include <algorithm>
#include <cmath>
#include <limits>

// 波形パラメータ平滑化の共通実装。
// GUI/JSON由来の異常値が入っても、レンダリング側で破綻しないことを優先する。
namespace
{
double SanitizeFinite(double value, double fallback = 0.0)
{
    // NaN/Inf は演算連鎖で拡散しやすいため、入口で有限値へ正規化する。
    return std::isfinite(value) ? value : fallback;
}

double ClampWithRange(double value, const SmoothingRange& range)
{
    const double minV = SanitizeFinite(range.minValue, -1.0e300);
    const double maxV = SanitizeFinite(range.maxValue, 1.0e300);
    if (minV > maxV)
    {
        // 設定不整合(min>max)でも無音化や発散を避けるため、順序を入れ替えて処理する。
        return std::clamp(value, maxV, minV);
    }
    return std::clamp(value, minV, maxV);
}

double ComputeAlpha(double timeMs, int sampleRate)
{
    if (!std::isfinite(timeMs) || timeMs <= 0.0 || sampleRate <= 0)
    {
        // 不正な時定数/サンプルレートでは平滑化を無効化し、即時反映へフォールバックする。
        return 1.0;
    }
    // 1-pole low-pass の離散化係数。timeMs が大きいほど追従を遅くする。
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
    // 範囲変更時に current/target も同時補正し、次フレームでの跳ねを防ぐ。
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
        // bypass 時は current を target に揃えて、後続の比較条件を単純化する。
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
