#include "gui/GUIMacroMapping.h"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <variant>

namespace
{
constexpr double kBrightnessMinHz = 200.0;
constexpr double kBrightnessMaxHz = 18000.0;
constexpr double kAttackMinSec = 0.001;
constexpr double kAttackMaxSec = 0.5;
constexpr double kReleaseMinSec = 0.05;
constexpr double kReleaseMaxSec = 2.0;

double ClampDouble(double value, double lo, double hi)
{
    return std::clamp(value, lo, hi);
}

float Clamp01(float value)
{
    return static_cast<float>(ClampDouble(static_cast<double>(value), 0.0, 1.0));
}

double SliderToLogValue(float t, double minValue, double maxValue)
{
    const double clamped = static_cast<double>(Clamp01(t));
    return minValue * std::pow(maxValue / minValue, clamped);
}

float LogValueToSlider(double value, double minValue, double maxValue)
{
    const double clampedValue = ClampDouble(value, minValue, maxValue);
    const double normalized = std::log(clampedValue / minValue) / std::log(maxValue / minValue);
    return Clamp01(static_cast<float>(normalized));
}

float LinearToSlider(double value, double minValue, double maxValue)
{
    const double clampedValue = ClampDouble(value, minValue, maxValue);
    const double span = maxValue - minValue;
    if (span <= 0.0)
    {
        return 0.0f;
    }
    return Clamp01(static_cast<float>((clampedValue - minValue) / span));
}

double SliderToLinear(float t, double minValue, double maxValue)
{
    return minValue + static_cast<double>(Clamp01(t)) * (maxValue - minValue);
}

void ApplyDrumBrightnessRoughnessMovement(DrumConfig& src, const MacroSliderState& s)
{
    src.baseFreq = SliderToLinear(s.brightness, 30.0, 300.0);
    src.noiseLevel = static_cast<double>(Clamp01(s.roughness));
    src.pitchDrop = SliderToLinear(s.movement, 0.0, 100.0);
}
} // namespace

void ApplyMacroSliders(ChannelConfig& ch, const MacroSliderState& sliders)
{
    const float brightness = Clamp01(sliders.brightness);
    const float roughness = Clamp01(sliders.roughness);
    const float movement = Clamp01(sliders.movement);
    const float envelope = Clamp01(sliders.envelope);

    std::visit([&](auto& src) {
        using T = std::decay_t<decltype(src)>;

        if constexpr (std::is_same_v<T, WaveformConfig> || std::is_same_v<T, AnalogConfig>)
        {
            src.filterCutoffHz = SliderToLogValue(brightness, kBrightnessMinHz, kBrightnessMaxHz);
            src.filterResonance = SliderToLinear(roughness, 0.5, 6.0);
            src.drive = static_cast<double>(roughness);
            src.modulation.lfo1.depth = static_cast<double>(movement);
            if constexpr (std::is_same_v<T, AnalogConfig>)
            {
                // Analog は揺れを LFO + Drift に連動させる。
                src.driftDepthCents = SliderToLinear(movement, 0.0, 20.0);
            }
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            src.feedback = static_cast<double>(brightness);
            src.ops[0].index = SliderToLinear(roughness, 0.0, 8.0);
            src.modulation.lfo1.depth = static_cast<double>(movement);
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            src.filterCutoffHz = SliderToLogValue(brightness, kBrightnessMinHz, kBrightnessMaxHz);
            src.filterResonance = SliderToLinear(roughness, 0.5, 6.0);
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            ApplyDrumBrightnessRoughnessMovement(src, sliders);
        }
        else if constexpr (std::is_same_v<T, DrumKitConfig>)
        {
            // selectedDrumNote の DrumConfig 適用は呼び出し元で行う契約。
        }
        else if constexpr (std::is_same_v<T, PsgConfig>)
        {
            const int duty = static_cast<int>(std::lround(SliderToLinear(brightness, 0.0, 7.0)));
            src.duty = std::clamp(duty, 0, 7);
        }
    }, ch.source);

    ch.attackSec = SliderToLogValue(envelope, kAttackMinSec, kAttackMaxSec);
    ch.releaseSec = SliderToLogValue(envelope, kReleaseMinSec, kReleaseMaxSec);
}

MacroSliderState ReadMacroSliders(const ChannelConfig& ch, const MacroSliderState& current)
{
    MacroSliderState out = current;

    std::visit([&](const auto& src) {
        using T = std::decay_t<decltype(src)>;

        if constexpr (std::is_same_v<T, WaveformConfig>)
        {
            out.brightness = LogValueToSlider(src.filterCutoffHz, kBrightnessMinHz, kBrightnessMaxHz);
            out.roughness = current.lastLayer2Roughness;
            out.movement = Clamp01(static_cast<float>(src.modulation.lfo1.depth));
            out.envelope = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            out.brightness = LogValueToSlider(src.filterCutoffHz, kBrightnessMinHz, kBrightnessMaxHz);
            out.roughness = current.lastLayer2Roughness;
            out.movement = current.lastLayer2Movement;
            out.envelope = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            out.brightness = Clamp01(static_cast<float>(src.feedback));
            out.roughness = LinearToSlider(src.ops[0].index, 0.0, 8.0);
            out.movement = Clamp01(static_cast<float>(src.modulation.lfo1.depth));
            out.envelope = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            out.brightness = LogValueToSlider(src.filterCutoffHz, kBrightnessMinHz, kBrightnessMaxHz);
            out.roughness = LinearToSlider(src.filterResonance, 0.5, 6.0);
            out.movement = current.lastLayer2Movement;
            out.envelope = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            out.brightness = LinearToSlider(src.baseFreq, 30.0, 300.0);
            out.roughness = Clamp01(static_cast<float>(src.noiseLevel));
            out.movement = LinearToSlider(src.pitchDrop, 0.0, 100.0);
            out.envelope = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, DrumKitConfig>)
        {
            out.brightness = current.brightness;
            out.roughness = current.lastLayer2Roughness;
            out.movement = current.lastLayer2Movement;
            out.envelope = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, PsgConfig>)
        {
            out.brightness = LinearToSlider(static_cast<double>(src.duty), 0.0, 7.0);
            out.roughness = current.lastLayer2Roughness;
            out.movement = current.lastLayer2Movement;
            out.envelope = current.lastLayer2Envelope;
        }
    }, ch.source);

    return out;
}
