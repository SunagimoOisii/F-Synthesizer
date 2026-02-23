#include "gui/GUIConfigUtils.h"

#include <cmath>
#include <type_traits>
#include <variant>

#include "config/SourceRegistry.h"

namespace
{
bool NearlyEq(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

bool DrumConfigEquals(const DrumConfig& a, const DrumConfig& b)
{
    return a.type == b.type &&
        NearlyEq(a.gain, b.gain) &&
        NearlyEq(a.baseFreq, b.baseFreq) &&
        NearlyEq(a.pitchDrop, b.pitchDrop) &&
        NearlyEq(a.pitchDecaySec, b.pitchDecaySec) &&
        NearlyEq(a.toneFreq, b.toneFreq) &&
        NearlyEq(a.toneLevel, b.toneLevel) &&
        NearlyEq(a.noiseLevel, b.noiseLevel) &&
        NearlyEq(a.hpCut, b.hpCut) &&
        NearlyEq(a.lpCut, b.lpCut) &&
        a.toneWave == b.toneWave &&
        a.noiseType == b.noiseType;
}

bool SourceConfigEquals(const SourceConfig& a, const SourceConfig& b)
{
    if (a.index() != b.index())
    {
        return false;
    }
    // variant型比較は型一致を先に確認してから分岐し、比較漏れを防ぐ。
    return std::visit([&](const auto& av) -> bool
        {
            using T = std::decay_t<decltype(av)>;
            const auto* bv = std::get_if<T>(&b);
            if (bv == nullptr) return false;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                return av.wave == bv->wave;
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                return av.noise == bv->noise;
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                return av.carrierWave == bv->carrierWave &&
                    av.modWave == bv->modWave &&
                    NearlyEq(av.carrierRatio, bv->carrierRatio) &&
                    NearlyEq(av.modRatio, bv->modRatio) &&
                    NearlyEq(av.index, bv->index) &&
                    NearlyEq(av.outLevel, bv->outLevel);
            }
            else if constexpr (std::is_same_v<T, DrumConfig>)
            {
                return DrumConfigEquals(av, *bv);
            }
            else if constexpr (std::is_same_v<T, DrumKitConfig>)
            {
                for (int i = 0; i < 128; i++)
                {
                    if (!DrumConfigEquals(av.map[i], bv->map[i])) return false;
                }
                return true;
            }
            return false;
        }, a);
}
} // namespace

namespace gui
{
WaveType WaveFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return WaveType::Sine;
    case 1: return WaveType::Square;
    case 2: return WaveType::Saw;
    case 3: return WaveType::Triangle;
    default: return WaveType::Saw;
    }
}

int WaveToIndex(WaveType w)
{
    switch (w)
    {
    case WaveType::Sine: return 0;
    case WaveType::Square: return 1;
    case WaveType::Saw: return 2;
    case WaveType::Triangle: return 3;
    }
    return 2;
}

NoiseType NoiseFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return NoiseType::White;
    case 1: return NoiseType::Pink;
    case 2: return NoiseType::Brown;
    case 3: return NoiseType::Blue;
    default: return NoiseType::White;
    }
}

int NoiseToIndex(NoiseType n)
{
    switch (n)
    {
    case NoiseType::White: return 0;
    case NoiseType::Pink: return 1;
    case NoiseType::Brown: return 2;
    case NoiseType::Blue: return 3;
    }
    return 0;
}

int SourceTypeIndex(const SourceConfig& src)
{
    return config::SourceKindToIndex(config::SourceConfigKind(src));
}

SourceConfig DefaultSourceByType(int idx)
{
    return config::DefaultSourceConfig(config::SourceKindFromIndex(idx));
}

bool ChannelConfigEquals(const ChannelConfig& a, const ChannelConfig& b)
{
    return NearlyEq(a.amp, b.amp) &&
        NearlyEq(a.attackSec, b.attackSec) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.sustainLevel, b.sustainLevel) &&
        NearlyEq(a.releaseSec, b.releaseSec) &&
        SourceConfigEquals(a.source, b.source);
}

bool ChannelMixStateEquals(const ChannelMixState& a, const ChannelMixState& b)
{
    return a.mute == b.mute &&
        a.solo == b.solo &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.pan, b.pan) &&
        NearlyEq(a.gain, b.gain);
}

void WriteJsonEscaped(std::ostream& out, const std::string& s)
{
    for (char c : s)
    {
        if (c == '\\') out << "\\\\";
        else if (c == '"') out << "\\\"";
        else if (c == '\n') out << "\\n";
        else out << c;
    }
}

} // namespace gui
