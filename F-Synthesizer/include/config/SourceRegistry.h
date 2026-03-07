#pragma once

#include <string_view>

#include "SynthEngine/SynthEngine.h"

namespace config
{
enum class SourceKind
{
    Waveform = 0,
    Noise = 1,
    Fm = 2,
    Drum = 3,
    DrumKit = 4,
    Count
};

constexpr int kSourceKindCount = static_cast<int>(SourceKind::Count);

// 各 source 種別の方式能力。方式分岐は SourceKind 直書きではなく本定義を参照する。
struct SourceCapability
{
    bool hasPitch = false;
    bool hasAmpEnv = false;
    bool hasFilterIn = false;
    bool hasModTargets = false;
    bool supportsPolyphony = false;
    bool isOneShot = false;
    bool isPercussion = false;
};

// typeName が既知なら true を返し、outKind を更新する。
bool TryParseSourceKind(std::string_view typeName, SourceKind& outKind);
// 不正値が来た場合は waveform を返す（読み込み互換を優先した代替値）。
const char* SourceKindToTypeName(SourceKind kind);
// GUI表示向けラベル。不正値は waveform を返す。
const char* SourceKindToDisplayName(SourceKind kind);
// 範囲外 index は Waveform へ丸める。
SourceKind SourceKindFromIndex(int index);
int SourceKindToIndex(SourceKind kind);

// variant の実体型から SourceKind を判定する。未知の型は Waveform を返す。
SourceKind SourceConfigKind(const SourceConfig& src);
// 種別ごとの capability を返す。不正値は Waveform を返す。
SourceCapability SourceCapabilityOf(SourceKind kind);
// SourceConfig の実体型に対応する capability を返す。
SourceCapability SourceCapabilityOf(const SourceConfig& src);
// 種別ごとの既定値を返す。DrumKit は 36(Kick) 以外を None 初期化する。
SourceConfig DefaultSourceConfig(SourceKind kind);
} // namespace config
