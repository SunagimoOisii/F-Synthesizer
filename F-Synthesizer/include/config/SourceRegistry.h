#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "SynthEngine/SourceConfig.h"

namespace config
{
// source.type の正規名を表す列挙。
// ConfigLoad / GUI / Renderer の分岐基準を統一する。
enum class SourceKind
{
    Waveform = 0,
    Analog = 1,
    Noise = 2,
    Fm = 3,
    Drum = 4,
    DrumKit = 5,
    Psg = 6,
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

enum class SourceLifecycleRetrigger
{
    Restart = 0,
    Stack = 1
};

enum class SourceLifecycleSteal
{
    Oldest = 0,
    RejectNew = 1
};

// source 種別ごとの lifecycle 契約（最小受け皿）。
// 実レンダ実装とは独立に、設定/文書整合の基準値として使う。
struct SourceLifecyclePolicy
{
    SourceLifecycleRetrigger retrigger = SourceLifecycleRetrigger::Restart;
    SourceLifecycleSteal steal = SourceLifecycleSteal::Oldest;
    bool noteOffEntersRelease = true;
    bool oneShotEndsAutomatically = false;
};

enum class SourceParameterType
{
    Int,
    Float
};

// sourceパラメータ検証の最小契約。
// min/max/default は ConfigLoad の値検証と既定値提示で共通利用する。
struct SourceParameterSchemaEntry
{
    const char* id = "";
    SourceParameterType type = SourceParameterType::Float;
    double minValue = 0.0;
    double maxValue = 0.0;
    double defaultValue = 0.0;
};

// 目的: source.type 文字列を SourceKind へ変換する。
// 前提: typeName は小文字の type 名（例: waveform / fm）を想定する。
bool TryParseSourceKind(std::string_view typeName, SourceKind& outKind);
// 目的: SourceKind から保存用の type 名を返す。
// 副作用: なし。不正値は読み込み互換のため waveform を返す。
const char* SourceKindToTypeName(SourceKind kind);
// GUI表示向けラベル。不正値は waveform を返す。
const char* SourceKindToDisplayName(SourceKind kind);
// 範囲外 index は Waveform へ丸める。
SourceKind SourceKindFromIndex(int index);
int SourceKindToIndex(SourceKind kind);

// 目的: SourceConfig の実体型から SourceKind を判定する。
// 副作用: なし。未知の型は Waveform を返す。
SourceKind SourceConfigKind(const SourceConfig& src);
// 種別ごとの capability を返す。不正値は Waveform を返す。
SourceCapability SourceCapabilityOf(SourceKind kind);
// SourceConfig の実体型に対応する capability を返す。
SourceCapability SourceCapabilityOf(const SourceConfig& src);
// 種別ごとの lifecycle 契約を返す。不正値は Waveform を返す。
SourceLifecyclePolicy SourceLifecycleOf(SourceKind kind);
// SourceConfig の実体型に対応する lifecycle 契約を返す。
SourceLifecyclePolicy SourceLifecycleOf(const SourceConfig& src);
// 種別ごとの最小パラメータschema（id/type/range/default）を返す。未定義種別は false を返す。
bool TryGetParameterSchema(
    SourceKind kind,
    const SourceParameterSchemaEntry*& outEntries,
    size_t& outCount);
// 種別ごとの既定値を返す。DrumKit は 36(Kick) 以外を None 初期化する。
SourceConfig DefaultSourceConfig(SourceKind kind);
// GUI の source selector に表示する種別かどうかを返す。
bool IsGuiSelectableSourceKind(SourceKind kind);
// GUI の source selector に表示する種別一覧を返す。
std::array<SourceKind, kSourceKindCount> GuiSelectableSourceKinds(size_t& outCount);
// GUI で DrumKit note 単位の編集/試聴対象にする source かどうかを返す。
bool UsesDrumKitNoteSelection(const SourceConfig& src);
} // namespace config
