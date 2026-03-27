# Codex Prompts: 3-Layer UX Implementation

## 背景・共通コンテキスト

F-Synthesizer は C++/ImGui ベースのシンセサイザー GUI。
Sound タブ右カラム（チャンネルエディタ）に3層パネルを追加する。

```
[Layer 1] プリセット + タグフィルター     ← 折りたたみ可
[Layer 2] マクロスライダー（4本）          ← 折りたたみ可
[Layer 3] 既存チャンネルエディタ           ← 変更なし
```

### 画面の全体像（`ux-improvement-roadmap.md` Phase 0「遊ぶ」モードより）

このタスクは「遊ぶ」モードの最終形の一部を先行実装する。
最終的には下図のレイアウトを目指すが、今回は **右カラム（音色エディタ部分）の3層化のみ** を実装する。

```
┌─────────────────────────────────────────────────────────────┐
│  [遊ぶ] [作る] [書き出す]                     Status: ●  │
├───────────────────┬─────────────────────────────────────────┤
│  プリセットブラウザ │  ▼ Layer 1: プリセット + タグ           │
│  [カテゴリ ▼]    │  [ゲーム的] [暗い] [明るい] ...          │
│  ─────────────── │  ─────────────────────────────────────── │
│  FC_Triangle_Bass │  ▼ Layer 2: マクロスライダー             │
│  FC_Square_Lead   │  明るさ ━━●━━  荒さ ━●━━━               │
│  FM_Bell        ← │  揺れ   ●━━━━  鳴り方 ━━━●             │
│  ─────────────── │  ─────────────────────────────────────── │
│  [Randomize]      │  Layer 3: 既存チャンネルエディタ          │
│  Subtle/Med/Wild  │    Waveform ▼                           │
│                   │    Filter / Envelope / LFO / Mod ...    │
├───────────────────┴─────────────────────────────────────────┤
│              仮想キーボード (C2 〜 C6)                       │
├─────────────────────────────────────────────────────────────┤
│  [Tone Preview]  [Stop]  Chord: [OFF ▼]  VU: ████░░ -3dB  │
└─────────────────────────────────────────────────────────────┘
```

**今回の実装スコープ（右カラム内の3層のみ）:**

```
┌─ Sound タブ ──────────────────────────────────────────────────┐
│ [ch list]  │ ▼ Layer 1: プリセット + タグ  [∧ 折りたたみ]  │
│            │   [ゲーム的] [暗い] [金属] ...  [Search]       │
│            ├──────────────────────────────────────────────────┤
│            │ ▼ Layer 2: マクロスライダー   [∧ 折りたたみ]  │
│            │   明るさ ━━●━━  荒さ ━●━━━  揺れ ●━━━━  鳴り方 ━━━●│
│            ├──────────────────────────────────────────────────┤
│            │ Layer 3: 既存チャンネルエディタ（変更なし）       │
└────────────┴──────────────────────────────────────────────────┘
```

**同期ポリシー**
- 単一パラメータスライダー: Layer3 編集時に逆算してスライダーを即時更新
- マルチパラメータスライダー（`[γ]` マーク）: Layer2 で最後に操作した値を保持。Layer3 個別編集では更新しない

**マクロスライダー マッピング表**

| スライダー | Waveform / Analog | FM | Noise | Drum |
|---|---|---|---|---|
| 明るさ | `filterCutoffHz` 200..18000 Hz（log） | `feedback` 0..1 | `filterCutoffHz` 200..18000 Hz（log） | `baseFreq` 30..300 Hz |
| 荒さ | `filterResonance` 0.5..6.0 + `drive` 0..1 `[γ]` | `ops[0].index` 0..8 | `filterResonance` 0.5..6.0 | `noiseLevel` 0..1 |
| 揺れ | `modulation.lfo1.depth` 0..1 | `modulation.lfo1.depth` 0..1 | （非表示・グレーアウト） | `pitchDrop` 0..100 Hz |
| 鳴り方 | `channelConfig.attackSec` 0.001..0.5 + `releaseSec` 0.05..2.0 `[γ]` | 同左 `[γ]` | 同左 `[γ]` | `pitchDecaySec` 0.01..0.5 + `releaseSec` 0.01..0.5 `[γ]` |

DrumKit の場合: `selectedDrumNote` が示す `DrumConfig` に対して Drum と同じマッピングを適用。
PSG の場合: 明るさ→`duty` 0..7（float 0..1 にスケール）, 荒さ→非表示, 揺れ→非表示, 鳴り方→`ChannelConfig::attackSec + releaseSec [γ]`。

---

## T1: MacroSliderState データモデル

### 目標
`GUIState` にマクロスライダー値を保持するデータモデルを追加する。

### 新規ファイル: `include/gui/GUIMacroSliders.h`

```cpp
#pragma once

// 1チャンネル分のマクロスライダー状態。
// brightness/roughness/movement/envelope は 0..1 の正規化値。
// lastLayer2* はγポリシー用: Layer2 で最後に操作した値。
// Layer3 個別編集後もこの値を保持し続ける。
struct MacroSliderState
{
    float brightness = 0.5f;
    float roughness  = 0.0f;
    float movement   = 0.0f;
    float envelope   = 0.3f;

    // γポリシー用保持値（マルチパラメータスライダーのみ使用）
    float lastLayer2Roughness = 0.0f;
    float lastLayer2Envelope  = 0.3f;
    // Analog の 揺れ は lfo1.depth + drift の複合なのでγ
    float lastLayer2Movement  = 0.0f;
};
```

### 変更ファイル: `include/gui/GUIState.h`

`GUIState` 構造体の末尾（`gui::PianoRollState pianoRoll` の直前）に以下を追加する。

```cpp
#include "gui/GUIMacroSliders.h"

// Layer2 マクロスライダーの状態（チャンネル数分）
std::array<MacroSliderState, 16> macroSliders{};
// Layer1 タグフィルターの選択状態（タグ数分）
// タグ定義は Layer1Discovery.inl で管理する
std::array<bool, 16> macroTagFilters{};
bool layer1Expanded = true;
bool layer2Expanded = true;
```

### 制約
- 既存フィールドは一切変更しない
- `MacroSliderState` のデフォルト値はプリセット未ロード時に中立な音になる値にする

---

## T2: 順方向マッピング（Layer2 → パラメータ）

### 目標
マクロスライダー値をソースタイプに応じて `ChannelConfig` の各パラメータへ書き込む関数を実装する。

### 新規ファイル: `include/gui/GUIMacroMapping.h`

```cpp
#pragma once
#include "SynthEngine/SynthEngine.h"
#include "gui/GUIMacroSliders.h"

// スライダー値 → パラメータ書き込み。
// sliders の brightness/roughness/movement/envelope (0..1) を ch のソース設定へ反映する。
// マッピング定義はマッピング表（codex-prompts-3layer-ux.md）に従う。
void ApplyMacroSliders(ChannelConfig& ch, const MacroSliderState& sliders);

// パラメータ → スライダー逆算。
// 単一パラメータスライダーのみ逆算して返す。
// マルチパラメータスライダー（γポリシー）は current の lastLayer2* 値をそのまま返す。
MacroSliderState ReadMacroSliders(const ChannelConfig& ch, const MacroSliderState& current);
```

### 新規ファイル: `src/gui/GUIMacroMapping.cpp`

実装方針:

```cpp
#include "gui/GUIMacroMapping.h"
#include <cmath>
#include <variant>

// log スケール変換ヘルパー
static double SliderToLogHz(float t, double minHz, double maxHz)
{
    // t=0 → minHz, t=1 → maxHz（対数補間）
    return minHz * std::pow(maxHz / minHz, static_cast<double>(t));
}
static float LogHzToSlider(double hz, double minHz, double maxHz)
{
    return static_cast<float>(std::log(hz / minHz) / std::log(maxHz / minHz));
}

void ApplyMacroSliders(ChannelConfig& ch, const MacroSliderState& s)
{
    std::visit([&](auto& src) {
        using T = std::decay_t<decltype(src)>;

        if constexpr (std::is_same_v<T, WaveformConfig> || std::is_same_v<T, AnalogConfig>)
        {
            // 明るさ: filterCutoffHz (log 200..18000)
            src.filterCutoffHz = SliderToLogHz(s.brightness, 200.0, 18000.0);
            // 荒さ: filterResonance 0.5..6.0 + drive 0..1 [γ: lastLayer2Roughness で比例配分]
            src.filterResonance = 0.5 + s.roughness * 5.5;
            src.drive           = static_cast<double>(s.roughness);
            // 揺れ: lfo1.depth 0..1
            src.modulation.lfo1.depth = static_cast<double>(s.movement);
            // Analog の drift も揺れに連動
            if constexpr (std::is_same_v<T, AnalogConfig>)
            {
                src.driftDepthCents = s.movement * 20.0;
            }
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            // 明るさ: feedback 0..1
            src.feedback = static_cast<double>(s.brightness);
            // 荒さ: ops[0].index 0..8
            src.ops[0].index = s.roughness * 8.0;
            // 揺れ: lfo1.depth
            src.modulation.lfo1.depth = static_cast<double>(s.movement);
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            // 明るさ: filterCutoffHz
            src.filterCutoffHz  = SliderToLogHz(s.brightness, 200.0, 18000.0);
            // 荒さ: filterResonance 0.5..6.0
            src.filterResonance = 0.5 + s.roughness * 5.5;
            // 揺れ: Noise は非対応（何もしない）
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            // 明るさ: baseFreq 30..300 Hz（線形）
            src.baseFreq   = 30.0 + s.brightness * 270.0;
            // 荒さ: noiseLevel 0..1
            src.noiseLevel = static_cast<double>(s.roughness);
            // 揺れ: pitchDrop 0..100 Hz
            src.pitchDrop  = s.movement * 100.0;
        }
        // DrumKit: 呼び出し元で selectedDrumNote の DrumConfig を取り出して DrumConfig 経路へ
        // PSG: 明るさ→duty(0..7), 鳴り方のみ ChannelConfig 経路
    }, ch.source);

    // 鳴り方: ChannelConfig レベルの ADSR（全ソース共通）
    // attackSec: 0.001..0.5（log補間）
    ch.attackSec  = 0.001 * std::pow(0.5 / 0.001, static_cast<double>(s.envelope));
    // releaseSec: 0.05..2.0（log補間）
    ch.releaseSec = 0.05  * std::pow(2.0  / 0.05,  static_cast<double>(s.envelope));
}

MacroSliderState ReadMacroSliders(const ChannelConfig& ch, const MacroSliderState& current)
{
    MacroSliderState out = current; // γポリシー値を引き継ぐ

    std::visit([&](const auto& src) {
        using T = std::decay_t<decltype(src)>;

        if constexpr (std::is_same_v<T, WaveformConfig> || std::is_same_v<T, AnalogConfig>)
        {
            out.brightness = LogHzToSlider(src.filterCutoffHz, 200.0, 18000.0);
            out.movement   = static_cast<float>(src.modulation.lfo1.depth);
            // 荒さ・鳴り方はγ → out は current の lastLayer2* をそのまま保持
            out.roughness  = current.lastLayer2Roughness;
            out.envelope   = current.lastLayer2Envelope;
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            out.brightness = static_cast<float>(src.feedback);
            out.roughness  = static_cast<float>(src.ops[0].index / 8.0);
            out.movement   = static_cast<float>(src.modulation.lfo1.depth);
            out.envelope   = current.lastLayer2Envelope; // γ
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            out.brightness = LogHzToSlider(src.filterCutoffHz, 200.0, 18000.0);
            out.roughness  = static_cast<float>((src.filterResonance - 0.5) / 5.5);
            out.movement   = current.lastLayer2Movement; // γ（非対応）
            out.envelope   = current.lastLayer2Envelope; // γ
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            out.brightness = static_cast<float>((src.baseFreq - 30.0) / 270.0);
            out.roughness  = static_cast<float>(src.noiseLevel);
            out.movement   = static_cast<float>(src.pitchDrop / 100.0);
            out.envelope   = current.lastLayer2Envelope; // γ
        }
    }, ch.source);

    // 鳴り方（ChannelConfig）は γ なので current を返す
    // ただし attackSec から逆算も可能なため、必要なら以下を有効化:
    // out.envelope = static_cast<float>(std::log(ch.attackSec / 0.001) / std::log(500.0));

    return out;
}
```

### CMakeLists への追加
`src/gui/GUIMacroMapping.cpp` をビルド対象に追加する。

---

## T3: Layer3 編集検知フック

### 目標
既存のチャンネルエディタ `.inl` ファイルが編集を行った後、`ReadMacroSliders` を呼んでスライダーを更新するフックを挿入する。

### 変更ファイル
`src/gui/channeleditor/ChannelEditorCommon.inl`
`src/gui/channeleditor/ChannelEditorWaveform.inl`
`src/gui/channeleditor/ChannelEditorFm.inl`
`src/gui/channeleditor/ChannelEditorNoise.inl`
`src/gui/channeleditor/ChannelEditorDrum.inl`

### 方針

各 `.inl` の冒頭付近に以下を追加（既に `#include "gui/GUIMacroMapping.h"` があれば不要）:

```cpp
#include "gui/GUIMacroMapping.h"
```

各ファイルで ImGui ウィジェットが `true` を返したとき（＝値が変化したとき）の直後に以下を挿入する:

```cpp
// Layer2 マクロスライダーを Layer3 編集に追従させる
{
    int ch = state.selectedSoundSlot;
    auto& cfg = (*state.channelConfigs)[ch];
    state.macroSliders[ch] = ReadMacroSliders(cfg, state.macroSliders[ch]);
}
```

#### 挿入箇所の特定方法

各 `.inl` ファイルで `ImGui::SliderDouble` / `ImGui::SliderInt` / `ImGui::Checkbox` / `ImGui::Combo` などのウィジェット呼び出しが `if (ImGui::...)` で囲まれているブロックを探し、その `if` ブロックの末尾に挿入する。

大量の挿入を避けるため、各 `.inl` の最後に共通ガード変数を用意してもよい:

```cpp
bool layer3Changed = false;
// ...各ウィジェットの変更検知で layer3Changed = true; を立てる...
if (layer3Changed)
{
    int ch = state.selectedSoundSlot;
    state.macroSliders[ch] = ReadMacroSliders((*state.channelConfigs)[ch], state.macroSliders[ch]);
}
```

---

## T4: Layer1 UI（プリセット + タグフィルター）

### 目標
Sound タブ右カラム上部に、タグフィルター付きプリセット一覧パネルを描画する。

### 新規ファイル: `src/gui/main/Layer1Discovery.inl`

```cpp
// Layer1: プリセット発見パネル
// DrawLayer1Discovery(state) を MainWindow.inl から呼び出す。

static constexpr const char* kMacroTags[] = {
    "ゲーム的", "暗い", "明るい", "金属的", "柔らかい",
    "ノイジー", "シンプル", "複雑", "アタック", "パッド",
    "FM", "アナログ", "ドラム", "ノイズ"
};
static constexpr int kMacroTagCount = 14;

// プリセット名がタグに対応するキーワードを含むか判定（暫定マッチング）
static bool PresetMatchesTag(const std::string& name, int tagIdx)
{
    // タグごとのキーワード定義（大文字小文字無視）
    static const char* keywords[kMacroTagCount] = {
        "game",  "dark",   "bright", "metal", "soft",
        "noise", "simple", "complex","attack", "pad",
        "fm",    "analog", "drum",   "noise"
    };
    std::string lower = name;
    for (auto& c : lower) c = static_cast<char>(std::tolower(c));
    return lower.find(keywords[tagIdx]) != std::string::npos;
}

static void DrawLayer1Discovery(GUIState& state)
{
    // 折りたたみヘッダー
    ImGui::SetNextItemOpen(state.layer1Expanded, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("発見  ( プリセット + タグ )"))
    {
        state.layer1Expanded = false;
        return;
    }
    state.layer1Expanded = true;

    // タグボタン群
    ImGui::TextDisabled("タグ:");
    ImGui::SameLine();
    for (int i = 0; i < kMacroTagCount; ++i)
    {
        bool active = state.macroTagFilters[i];
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::SmallButton(kMacroTags[i])) state.macroTagFilters[i] = !state.macroTagFilters[i];
        if (active) ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // アクティブタグの有無を確認
    bool anyTag = false;
    for (int i = 0; i < kMacroTagCount; ++i) if (state.macroTagFilters[i]) { anyTag = true; break; }

    // プリセット一覧（フィルター適用）
    float listHeight = 80.0f;
    ImGui::BeginChild("##layer1_presets", ImVec2(0, listHeight), true);
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); ++i)
    {
        const std::string& name = state.presetItems[i];
        if (anyTag)
        {
            bool match = false;
            for (int t = 0; t < kMacroTagCount; ++t)
                if (state.macroTagFilters[t] && PresetMatchesTag(name, t)) { match = true; break; }
            if (!match) continue;
        }
        bool selected = (state.presetIndex == i);
        if (ImGui::Selectable(name.c_str(), selected))
        {
            state.presetIndex = i;
            // プリセットロード → 既存の LoadPresetByIndex 相当の処理を呼ぶ
            // （既存 GUIPresetIO の関数を再利用すること）
            // ロード後に autoTonePreview をトリガー
            state.autoTonePreviewPending = true;
        }
    }
    ImGui::EndChild();
}
```

### 変更ファイル: `src/gui/main/MainWindow.inl`

Sound タブ内、チャンネルエディタ描画の直前に以下を追加する:

```cpp
#include "gui/main/Layer1Discovery.inl"

// Sound タブ内:
DrawLayer1Discovery(state);
ImGui::Separator();
```

---

## T5: Layer2 UI（マクロスライダー）

### 目標
Layer1 の下にマクロスライダーパネルを描画する。スライダー操作時に `ApplyMacroSliders` を呼ぶ。

### 新規ファイル: `src/gui/main/Layer2Macros.inl`

```cpp
// Layer2: マクロスライダーパネル
// DrawLayer2Macros(state) を MainWindow.inl から呼び出す。

#include "gui/GUIMacroMapping.h"

// ソースタイプに応じたスライダーラベル
struct MacroLabels { const char* brightness; const char* roughness; const char* movement; const char* envelope; };

static MacroLabels GetMacroLabels(const SourceConfig& src)
{
    if (std::holds_alternative<FmConfig>(src))
        return {"明るさ (FB)", "荒さ (Index)", "揺れ (LFO)", "鳴り方 (ADSR)"};
    if (std::holds_alternative<NoiseConfig>(src))
        return {"明るさ (Filter)", "荒さ (Res)", nullptr,            "鳴り方 (ADSR)"};
    if (std::holds_alternative<DrumConfig>(src) || std::holds_alternative<DrumKitConfig>(src))
        return {"明るさ (Pitch)", "荒さ (Noise)", "揺れ (Sweep)", "鳴り方 (Decay)"};
    // Waveform / Analog / PSG
    return {"明るさ (Filter)", "荒さ (Res+Drive)", "揺れ (LFO)", "鳴り方 (ADSR)"};
}

static void DrawLayer2Macros(GUIState& state)
{
    ImGui::SetNextItemOpen(state.layer2Expanded, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("調整  ( マクロスライダー )"))
    {
        state.layer2Expanded = false;
        return;
    }
    state.layer2Expanded = true;

    int ch = state.selectedSoundSlot;
    auto& cfg    = (*state.channelConfigs)[ch];
    auto& sliders = state.macroSliders[ch];
    MacroLabels labels = GetMacroLabels(cfg.source);

    bool changed = false;

    // 明るさ
    if (ImGui::SliderFloat(labels.brightness, &sliders.brightness, 0.0f, 1.0f))
        changed = true;

    // 荒さ
    if (ImGui::SliderFloat(labels.roughness, &sliders.roughness, 0.0f, 1.0f))
    {
        sliders.lastLayer2Roughness = sliders.roughness;
        changed = true;
    }

    // 揺れ（Noise は非表示）
    if (labels.movement != nullptr)
    {
        if (ImGui::SliderFloat(labels.movement, &sliders.movement, 0.0f, 1.0f))
        {
            // Analog のみγ対象
            sliders.lastLayer2Movement = sliders.movement;
            changed = true;
        }
    }

    // 鳴り方
    if (ImGui::SliderFloat(labels.envelope, &sliders.envelope, 0.0f, 1.0f))
    {
        sliders.lastLayer2Envelope = sliders.envelope;
        changed = true;
    }

    if (changed)
    {
        ApplyMacroSliders(cfg, sliders);
        state.autoTonePreviewPending = true;
    }
}
```

### 変更ファイル: `src/gui/main/MainWindow.inl`

Layer1 の描画呼び出しの直後に追加:

```cpp
#include "gui/main/Layer2Macros.inl"

DrawLayer2Macros(state);
ImGui::Separator();
```

---

## T6: CMakeLists.txt への追加

### 変更ファイル: `CMakeLists.txt`（またはプロジェクトのビルド定義ファイル）

```cmake
# GUIMacroMapping を追加
target_sources(FSynthesizer PRIVATE
    src/gui/GUIMacroMapping.cpp
)
```

既存の `target_sources` ブロックに `src/gui/GUIMacroMapping.cpp` を追加する。

---

## T7: 統合確認チェックリスト

実装完了後に以下を手動で確認する:

1. **ビルド**: `./scripts/check.ps1` がエラーなく通る
2. **Layer1**: Sound タブを開くとプリセット一覧が表示される
3. **Layer1 タグ**: タグボタンを押すとプリセットがフィルタリングされる
4. **Layer1 選択**: プリセットを選択すると自動プレビューが起動する
5. **Layer2 明るさ**: スライダーを動かすと `filterCutoffHz` が変化しトーンが変わる
6. **Layer2 → Layer3 同期**: Layer2 で明るさを動かした後、Layer3 の Filter Cutoff スライダーが追従している
7. **Layer3 → Layer2 同期（単一パラメータ）**: Layer3 の Filter Cutoff を変更すると Layer2 の明るさが更新される
8. **Layer3 → Layer2 同期（γポリシー）**: Layer3 の filterResonance を変更しても Layer2 の荒さは変化しない
9. **ソース切替**: FM チャンネル選択時にスライダーラベルが変わる
10. **Noise**: 揺れスライダーが非表示になる
11. **折りたたみ**: Layer1/Layer2 の折りたたみが機能し、状態が `GUIState` に保持される

---

## 実装順序の推奨

```
T1（データモデル）
  → T2（順方向マッピング）
  → T5（Layer2 UI）  ← ここで基本動作確認
  → T3（Layer3 フック）
  → T4（Layer1 UI）
  → T6（ビルド登録）
  → T7（統合確認）
```

T1→T2→T5 の3ステップで「Layer2 スライダーが音を変える」が確認できる。
その後 T3 で双方向同期、T4 でプリセット発見を追加する。
