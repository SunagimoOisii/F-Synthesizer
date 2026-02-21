#include "gui/GUIConfigUtils.h"

#include <cmath>
#include <type_traits>
#include <variant>

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
    if (std::holds_alternative<WaveformConfig>(src)) return 0;
    if (std::holds_alternative<NoiseConfig>(src)) return 1;
    if (std::holds_alternative<FmConfig>(src)) return 2;
    if (std::holds_alternative<DrumConfig>(src)) return 3;
    if (std::holds_alternative<DrumKitConfig>(src)) return 4;
    return 0;
}

SourceConfig DefaultSourceByType(int idx)
{
    switch (idx)
    {
    case 0: return WaveformConfig{ WaveType::Saw };
    case 1: return NoiseConfig{ NoiseType::White };
    case 2: return FmConfig{ WaveType::Sine, WaveType::Sine, 1.0, 2.0, 1.0, 1.0 };
    case 3: return DrumConfig{ DrumType::Kick };
    case 4:
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map) d.type = DrumType::None;
        kit.map[36] = DrumConfig{ DrumType::Kick };
        return kit;
    }
    default: return WaveformConfig{ WaveType::Saw };
    }
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

std::string WaveToText(WaveType w)
{
    switch (w)
    {
    case WaveType::Sine: return "sine";
    case WaveType::Square: return "square";
    case WaveType::Saw: return "saw";
    case WaveType::Triangle: return "triangle";
    }
    return "saw";
}

std::string NoiseToText(NoiseType n)
{
    switch (n)
    {
    case NoiseType::White: return "white";
    case NoiseType::Pink: return "pink";
    case NoiseType::Brown: return "brown";
    case NoiseType::Blue: return "blue";
    }
    return "white";
}

std::string DrumTypeToText(DrumType d)
{
    switch (d)
    {
    case DrumType::None: return "none";
    case DrumType::Kick: return "kick";
    case DrumType::Snare: return "snare";
    case DrumType::Hat: return "hat";
    }
    return "none";
}

void WriteSourceJson(std::ostream& out, const SourceConfig& src, int indent)
{
    const std::string sp(indent, ' ');
    out << sp << "\"source\": {\n";
    std::visit([&](const auto& v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                out << sp << "  \"type\": \"waveform\",\n";
                out << sp << "  \"wave\": \"" << WaveToText(v.wave) << "\"\n";
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                out << sp << "  \"type\": \"noise\",\n";
                out << sp << "  \"noise\": \"" << NoiseToText(v.noise) << "\"\n";
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                out << sp << "  \"type\": \"fm\",\n";
                out << sp << "  \"carrierWave\": \"" << WaveToText(v.carrierWave) << "\",\n";
                out << sp << "  \"modWave\": \"" << WaveToText(v.modWave) << "\",\n";
                out << sp << "  \"carrierRatio\": " << v.carrierRatio << ",\n";
                out << sp << "  \"modRatio\": " << v.modRatio << ",\n";
                out << sp << "  \"index\": " << v.index << ",\n";
                out << sp << "  \"outLevel\": " << v.outLevel << "\n";
            }
            else if constexpr (std::is_same_v<T, DrumConfig>)
            {
                out << sp << "  \"type\": \"drum\",\n";
                out << sp << "  \"drumType\": \"" << DrumTypeToText(v.type) << "\",\n";
                out << sp << "  \"gain\": " << v.gain << ",\n";
                out << sp << "  \"baseFreq\": " << v.baseFreq << ",\n";
                out << sp << "  \"pitchDrop\": " << v.pitchDrop << ",\n";
                out << sp << "  \"pitchDecaySec\": " << v.pitchDecaySec << ",\n";
                out << sp << "  \"toneFreq\": " << v.toneFreq << ",\n";
                out << sp << "  \"toneLevel\": " << v.toneLevel << ",\n";
                out << sp << "  \"noiseLevel\": " << v.noiseLevel << ",\n";
                out << sp << "  \"hpCut\": " << v.hpCut << ",\n";
                out << sp << "  \"lpCut\": " << v.lpCut << ",\n";
                out << sp << "  \"toneWave\": \"" << WaveToText((WaveType)v.toneWave) << "\",\n";
                out << sp << "  \"noiseType\": \"" << NoiseToText((NoiseType)v.noiseType) << "\"\n";
            }
            else if constexpr (std::is_same_v<T, DrumKitConfig>)
            {
                out << sp << "  \"type\": \"drumkit\",\n";
                out << sp << "  \"map\": {\n";
                bool first = true;
                for (int note = 0; note < 128; note++)
                {
                    const auto& d = v.map[note];
                    if (d.type == DrumType::None) continue;
                    if (!first) out << ",\n";
                    first = false;
                    out << sp << "    \"" << note << "\": {\n";
                    out << sp << "      \"drumType\": \"" << DrumTypeToText(d.type) << "\",\n";
                    out << sp << "      \"gain\": " << d.gain << ",\n";
                    out << sp << "      \"baseFreq\": " << d.baseFreq << "\n";
                    out << sp << "    }";
                }
                out << "\n" << sp << "  }\n";
            }
        }, src);
    out << sp << "}";
}
} // namespace gui
