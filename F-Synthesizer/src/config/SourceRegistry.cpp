#include "config/SourceRegistry.h"

#include <array>

namespace config
{
namespace
{
// SourceKind ごとの固定メタ情報。
// SourceRegistry の公開関数はこの表だけを参照し、定義の重複を避ける。
struct SourceKindInfo
{
    SourceKind kind;
    const char* typeName;
    const char* displayName;
    SourceCapability capability;
    SourceLifecyclePolicy lifecycle;
};

constexpr std::array<SourceKindInfo, kSourceKindCount> kSourceKinds{ {
    {
        SourceKind::Waveform,
        "waveform",
        "waveform",
        SourceCapability{ true, true, true, true, true, false, false },
        SourceLifecyclePolicy{ SourceLifecycleRetrigger::Restart, SourceLifecycleSteal::Oldest, true, false }
    },
    {
        SourceKind::Noise,
        "noise",
        "noise",
        SourceCapability{ false, true, false, false, true, false, false },
        SourceLifecyclePolicy{ SourceLifecycleRetrigger::Restart, SourceLifecycleSteal::Oldest, true, false }
    },
    {
        SourceKind::Fm,
        "fm",
        "fm",
        SourceCapability{ true, true, true, true, true, false, false },
        SourceLifecyclePolicy{ SourceLifecycleRetrigger::Restart, SourceLifecycleSteal::Oldest, true, false }
    },
    {
        SourceKind::Drum,
        "drum",
        "drum",
        SourceCapability{ false, true, false, false, false, true, true },
        SourceLifecyclePolicy{ SourceLifecycleRetrigger::Stack, SourceLifecycleSteal::RejectNew, false, true }
    },
    {
        SourceKind::DrumKit,
        "drumkit",
        "drumkit",
        SourceCapability{ false, true, false, false, false, true, true },
        SourceLifecyclePolicy{ SourceLifecycleRetrigger::Stack, SourceLifecycleSteal::RejectNew, false, true }
    },
} };

constexpr std::array<SourceParameterSchemaEntry, 6> kWaveformParameterSchema{ {
    { "unisonVoices", SourceParameterType::Int, 1.0, 8.0, 1.0 },
    { "unisonDetuneCents", SourceParameterType::Float, 0.0, 120.0, 0.0 },
    { "unisonSpread", SourceParameterType::Float, 0.0, 1.0, 0.0 },
    { "subOscLevel", SourceParameterType::Float, 0.0, 2.0, 0.0 },
    { "filterCutoffHz", SourceParameterType::Float, 10.0, 20000.0, 8000.0 },
    { "filterResonance", SourceParameterType::Float, 0.1, 18.0, 0.707 },
} };

constexpr std::array<SourceParameterSchemaEntry, 1> kNoiseParameterSchema{ {
    { "noise", SourceParameterType::Int, 0.0, 3.0, 0.0 },
} };

constexpr std::array<SourceParameterSchemaEntry, 6> kFmParameterSchema{ {
    { "carrierWave", SourceParameterType::Int, 0.0, 3.0, 0.0 },
    { "modWave", SourceParameterType::Int, 0.0, 3.0, 0.0 },
    { "carrierRatio", SourceParameterType::Float, 0.0, 32.0, 1.0 },
    { "modRatio", SourceParameterType::Float, 0.0, 32.0, 2.0 },
    { "index", SourceParameterType::Float, 0.0, 20.0, 1.0 },
    { "outLevel", SourceParameterType::Float, 0.0, 4.0, 1.0 },
} };

constexpr std::array<SourceParameterSchemaEntry, 12> kDrumParameterSchema{ {
    { "drumType", SourceParameterType::Int, 0.0, 3.0, 0.0 },
    { "gain", SourceParameterType::Float, 0.0, 4.0, 1.0 },
    { "baseFreq", SourceParameterType::Float, 20.0, 20000.0, 60.0 },
    { "pitchDrop", SourceParameterType::Float, 0.1, 16.0, 3.0 },
    { "pitchDecaySec", SourceParameterType::Float, 0.001, 2.0, 0.06 },
    { "toneFreq", SourceParameterType::Float, 20.0, 20000.0, 200.0 },
    { "toneLevel", SourceParameterType::Float, 0.0, 2.0, 0.5 },
    { "noiseLevel", SourceParameterType::Float, 0.0, 2.0, 0.5 },
    { "hpCut", SourceParameterType::Float, 20.0, 20000.0, 600.0 },
    { "lpCut", SourceParameterType::Float, 20.0, 20000.0, 9000.0 },
    { "toneWave", SourceParameterType::Int, 0.0, 3.0, 0.0 },
    { "noiseType", SourceParameterType::Int, 0.0, 3.0, 0.0 },
} };

// DrumKit は note(0..127) ごとに DrumConfig を持つ可変構造のため、
// 1本の schema 配列では表現しない。
constexpr std::array<SourceParameterSchemaEntry, 0> kDrumKitParameterSchema{};
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
    // 範囲外入力は Waveform へ倒し、既存設定との互換を優先する。
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

SourceCapability SourceCapabilityOf(SourceKind kind)
{
    for (const auto& k : kSourceKinds)
    {
        if (k.kind == kind)
        {
            return k.capability;
        }
    }
    return kSourceKinds[0].capability;
}

SourceCapability SourceCapabilityOf(const SourceConfig& src)
{
    return SourceCapabilityOf(SourceConfigKind(src));
}

SourceLifecyclePolicy SourceLifecycleOf(SourceKind kind)
{
    for (const auto& k : kSourceKinds)
    {
        if (k.kind == kind)
        {
            return k.lifecycle;
        }
    }
    return kSourceKinds[0].lifecycle;
}

SourceLifecyclePolicy SourceLifecycleOf(const SourceConfig& src)
{
    return SourceLifecycleOf(SourceConfigKind(src));
}

bool TryGetParameterSchema(
    SourceKind kind,
    const SourceParameterSchemaEntry*& outEntries,
    size_t& outCount)
{
    // 返却先は static 配列を指すため、呼び出し側で解放しない。
    outEntries = nullptr;
    outCount = 0;
    switch (kind)
    {
    case SourceKind::Waveform:
        outEntries = kWaveformParameterSchema.data();
        outCount = kWaveformParameterSchema.size();
        return true;
    case SourceKind::Noise:
        outEntries = kNoiseParameterSchema.data();
        outCount = kNoiseParameterSchema.size();
        return true;
    case SourceKind::Fm:
        outEntries = kFmParameterSchema.data();
        outCount = kFmParameterSchema.size();
        return true;
    case SourceKind::Drum:
        outEntries = kDrumParameterSchema.data();
        outCount = kDrumParameterSchema.size();
        return true;
    case SourceKind::DrumKit:
        outEntries = kDrumKitParameterSchema.data();
        outCount = 0;
        return true;
    case SourceKind::Count:
        return false;
    }
    return false;
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
        return FmConfig{ WaveType::Sine, WaveType::Sine, 1.0, 2.0, 1.0, 1.0, {} };
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
