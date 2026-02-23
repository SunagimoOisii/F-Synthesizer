#include "config/SourceRegistry.h"

#include <array>

namespace config
{
namespace
{
struct SourceKindInfo
{
    SourceKind kind;
    const char* typeName;
    const char* displayName;
};

constexpr std::array<SourceKindInfo, kSourceKindCount> kSourceKinds{ {
    { SourceKind::Waveform, "waveform", "waveform" },
    { SourceKind::Noise, "noise", "noise" },
    { SourceKind::Fm, "fm", "fm" },
    { SourceKind::Drum, "drum", "drum" },
    { SourceKind::DrumKit, "drumkit", "drumkit" },
} };
} // namespace

bool TryParseSourceKind(std::string_view typeName, SourceKind& outKind)
{
    for (const auto& k : kSourceKinds)
    {
        if (typeName == k.typeName)
        {
            outKind = k.kind;
            return true;
        }
    }
    return false;
}

const char* SourceKindToTypeName(SourceKind kind)
{
    for (const auto& k : kSourceKinds)
    {
        if (k.kind == kind)
        {
            return k.typeName;
        }
    }
    return "waveform";
}

const char* SourceKindToDisplayName(SourceKind kind)
{
    for (const auto& k : kSourceKinds)
    {
        if (k.kind == kind)
        {
            return k.displayName;
        }
    }
    return "waveform";
}

SourceKind SourceKindFromIndex(int index)
{
    if (index < 0 || index >= kSourceKindCount)
    {
        return SourceKind::Waveform;
    }
    return static_cast<SourceKind>(index);
}

int SourceKindToIndex(SourceKind kind)
{
    return static_cast<int>(kind);
}

SourceKind SourceConfigKind(const SourceConfig& src)
{
    if (std::holds_alternative<WaveformConfig>(src)) return SourceKind::Waveform;
    if (std::holds_alternative<NoiseConfig>(src)) return SourceKind::Noise;
    if (std::holds_alternative<FmConfig>(src)) return SourceKind::Fm;
    if (std::holds_alternative<DrumConfig>(src)) return SourceKind::Drum;
    if (std::holds_alternative<DrumKitConfig>(src)) return SourceKind::DrumKit;
    return SourceKind::Waveform;
}

SourceConfig DefaultSourceConfig(SourceKind kind)
{
    switch (kind)
    {
    case SourceKind::Waveform:
        return WaveformConfig{ WaveType::Saw };
    case SourceKind::Noise:
        return NoiseConfig{ NoiseType::White };
    case SourceKind::Fm:
        return FmConfig{ WaveType::Sine, WaveType::Sine, 1.0, 2.0, 1.0, 1.0 };
    case SourceKind::Drum:
        return DrumConfig{ DrumType::Kick };
    case SourceKind::DrumKit:
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map)
        {
            d.type = DrumType::None;
        }
        kit.map[36] = DrumConfig{ DrumType::Kick };
        return kit;
    }
    case SourceKind::Count:
        break;
    }
    return WaveformConfig{ WaveType::Saw };
}
} // namespace config
