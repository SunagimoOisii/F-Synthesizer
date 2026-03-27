# Codex 修正プロンプト集: 3-Layer UX 実装レビュー後の差分修正

元実装: `codex-prompts-3layer-ux.md`
対象ブランチ: `main`

各プロンプトは独立して実行可能。依存がある場合は「前提」欄に記載。
修正後は必ず `./F-Synthesizer/scripts/check.ps1` でビルドを確認すること。

---

## Fix A-1: 左カラムの旧 Preset Combo を削除（重複排除）

### 問題
`src/gui/main/MainWindow.inl` の Sound タブ左カラムに `ImGui::Combo("Preset", ...)` と
`Apply Preset Paths` ボタンが残存している。右カラムの `DrawLayer1Discovery` が
同じ `applyPresetByIndex` を呼ぶプリセット選択 UI を提供しているため重複している。

### 修正
`src/gui/main/MainWindow.inl` の左カラム（`layout_split` テーブルの column 0）から
以下のブロックを削除する。

**削除対象 1: `presetGetter` ラムダと `Combo("Preset", ...)` ブロック**

```cpp
// ↓ここから削除
auto presetGetter = [](void* data, int idx, const char** outText) -> bool
{
    auto* items = static_cast<std::vector<std::string>*>(data);
    if (items == nullptr || idx < 0 || idx >= static_cast<int>(items->size()))
    {
        return false;
    }
    *outText = (*items)[idx].c_str();
    return true;
};
const int beforePresetIndex = state.presetIndex;
if (ImGui::Combo("Preset", &state.presetIndex, presetGetter, &state.presetItems, static_cast<int>(state.presetItems.size())))
{
    if (state.presetDirty)
    {
        pendingPresetOriginalIndex = beforePresetIndex;
        pendingPresetIndex = state.presetIndex;
        openUnsavedPopupNextFrame = true;
    }
    else
    {
        applyPresetByIndex(state.presetIndex);
    }
}
updateHoverHelp(
    "読み込むPresetを選択します。",
    "Sound設定の読み込み対象が変わります。",
    "未保存変更がある場合は確認ダイアログを表示します。");
// ↑ここまで削除
```

**削除対象 2: `Apply Preset Paths` ボタン**（上記の直後 `ImGui::SameLine()` から始まるブロック）

```cpp
// ↓ここから削除
ImGui::SameLine();
if (ImGui::Button("Apply Preset Paths"))
{
    if (state.presetDirty)
    {
        pendingPresetOriginalIndex = state.presetIndex;
        pendingPresetIndex = state.presetIndex;
        openUnsavedPopupNextFrame = true;
    }
    else
    {
        applyPresetByIndex(state.presetIndex);
    }
}
updateHoverHelp(
    "選択中Presetを再適用します。",
    "Preset由来の設定パスを現在状態へ反映します。");
// ↑ここまで削除（直後の ImGui::SameLine(); は次の Reset Defaults ボタンのものなので残す）
```

### 確認
- 左カラムに `Preset` コンボが表示されないこと
- 右カラムの Layer1 でプリセットを選択・ロードできること
- `Save Preset As` / `Duplicate Preset` / `Reset Defaults` / `Reset Sound Slot` は左カラムに残ること

---

## Fix A-2: 左カラムの Source Type Combo を Layer1 と連動させる

### 問題
左カラムの `Source Type (Preset Scope)` Combo が `RefreshPresetItems` を呼んで
`state.presetItems` を更新している。Layer1 はこの `state.presetItems` をそのまま
表示するため、Source Type を変えれば Layer1 にも反映される。
しかし `Source Type (Preset Scope)` というラベルが Layer1 の存在を前提としない
旧来の文言になっており、Fix A-1 後に文言が不適切になる。
また、現在選択中のソースタイプが Layer1 のヘッダに表示されないため、
Layer1 がフィルタ済みリストを表示していることがユーザーに伝わらない。

### 修正

**1. ラベルを変更する**

`src/gui/main/MainWindow.inl` の左カラムにある Combo のラベルを変更する:

```cpp
// 変更前
ImGui::BeginCombo("Source Type (Preset Scope)", ...)

// 変更後
ImGui::BeginCombo("Source Type", ...)
```

`updateHoverHelp` の第三引数（caution）も更新する:
```cpp
// 変更前
"切替時は選択中スロットを該当sourceTypeの初期値で再初期化します。"

// 変更後
"切替時は選択中スロットを該当sourceTypeの初期値で再初期化し、Layer1のプリセット一覧も更新されます。"
```

**2. Layer1 のヘッダにフィルタ中のソースタイプを表示する**

`src/gui/main/Layer1Discovery.inl` の `DrawLayer1Discovery` 関数内、
`ImGui::CollapsingHeader` の直後に現在のソースタイプ名を表示する行を追加する:

```cpp
// CollapsingHeader が展開されているブロック内の先頭に追加
// state.channelConfigs が有効な場合のみ表示
if (state.channelConfigs)
{
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    const config::SourceKind kind = config::SourceConfigKind((*state.channelConfigs)[slot].source);
    ImGui::TextDisabled("Source: %s  (%d presets)",
        config::SourceKindToDisplayName(kind),
        static_cast<int>(state.presetItems.size()));
}
```

このために `Layer1Discovery.inl` の先頭に以下のインクルードを追加する:
```cpp
#include "config/SourceRegistry.h"
```

### 確認
- Source Type Combo を変えると Layer1 のリストが絞り込まれること
- Layer1 ヘッダ直下に現在のソースタイプ名とプリセット件数が表示されること

---

## Fix B-1: Layer2 スライダーのデバウンスタイマー未更新

### 問題
`src/gui/main/Layer2Macros.inl` の `DrawLayer2Macros` 関数末尾で
`state.autoTonePreviewPending = true` を直接代入している。

`MainWindow.inl` の `requestAutoTonePreview()` ラムダは
`autoTonePreviewEnabled` のチェックと `autoTonePreviewLastEditSec` の更新を
同時に行うが、Layer2Macros.inl からはこのラムダにアクセスできない。
結果として Layer2 スライダー操作時にデバウンスタイマーが更新されず、
スライダーをドラッグするたびに即時再生が発火し続ける。

### 修正
`src/gui/main/Layer2Macros.inl` の以下の行を変更する:

```cpp
// 変更前
state.autoTonePreviewPending = true;

// 変更後
if (state.autoTonePreviewEnabled)
{
    state.autoTonePreviewPending = true;
    state.autoTonePreviewLastEditSec = ImGui::GetTime();
}
```

### 確認
- Layer2 スライダーをゆっくりドラッグしているあいだ、再生が連続発火しないこと
- スライダーを止めてから約 0.4 秒後（`kAutoTonePreviewDebounceSec`）に再生が発火すること
- `Auto Tone Preview` が OFF のときは Layer2 スライダー変更で再生が発火しないこと

---

## Fix C-1: `layer3Changed` の同フレーム編集取りこぼし

### 問題
`src/gui/GUIChannelEditor.cpp` の `DrawChannelEditor` 内で:

```cpp
bool layer3Changed = false;
if (ImGui::CollapsingHeader("Source Details", ImGuiTreeNodeFlags_DefaultOpen))
{
    const bool changedBeforeSourceDetails = changed;  // ← Envelope/Gain 変更が already true の場合がある
    ...
    layer3Changed = (changed != changedBeforeSourceDetails);
}
```

`Envelope / Gain` セクション（Attack/Release 等）を編集した同フレームに
`Source Details` セクションも編集された場合、
`changedBeforeSourceDetails = true`, `changed = true` となり
`changed != changedBeforeSourceDetails = false` → `ReadMacroSliders` が呼ばれない。

γポリシー対象（荒さ/鳴り方）は影響ないが、単一パラメータの `明るさ`（filterCutoffHz）
が同フレームで更新されない場合がある。

### 修正

```cpp
// 変更前
layer3Changed = (changed != changedBeforeSourceDetails);

// 変更後
layer3Changed = changed && !changedBeforeSourceDetails;
```

意味: 「Source Details に入る前は変更なし（false）かつ、Source Details 処理後に
変更あり（true）になった」ときのみ Layer2 を更新する。

### 確認
- Layer3 の Filter Cutoff スライダーを動かすと Layer2 の `明るさ` が追従すること
- Layer3 で Envelope の Attack を変更しても Layer2 の `鳴り方` が変化しないこと（γポリシー維持）

---

## Fix D-1: `macroTagFilters` のサイズ誤り

### 前提
Fix E-1 と同時実施を推奨（`kMacroTagCount` の移動を共有するため）。

### 問題
`include/gui/GUIState.h` の `macroTagFilters` が `std::array<bool, 16>` になっているが、
タグ数は `kMacroTagCount = 14` が正しい。
`kMacroTagCount` は現在 `Layer1Discovery.inl` の無名 namespace 内で定義されており、
`GUIState.h` からアクセスできないため `16` というマジックナンバーになっている。

### 修正

**1. `kMacroTagCount` を `GUIMacroSliders.h` に移動する**

`include/gui/GUIMacroSliders.h` に追加:
```cpp
// Layer1 タグ数。GUIState と Layer1Discovery で共有する。
static constexpr int kMacroTagCount = 14;
```

**2. `Layer1Discovery.inl` の重複定義を削除する**

`src/gui/main/Layer1Discovery.inl` の無名 namespace 内にある
`static constexpr int kMacroTagCount = 14;` の行を削除する。
（`GUIMacroSliders.h` は `GUIState.h` 経由でインクルード済みのため定義は見える）

**3. `GUIState.h` の配列サイズを修正する**

```cpp
// 変更前
std::array<bool, 16> macroTagFilters{};

// 変更後
std::array<bool, kMacroTagCount> macroTagFilters{};
```

### 確認
- ビルドが通ること
- タグボタン 14 個が正しく toggle できること

---

## Fix E-1: Layer1 タグボタンの折り返し

### 前提
Fix D-1 完了後（`kMacroTagCount` が `GUIMacroSliders.h` に移動済みであること）。

### 問題
`src/gui/main/Layer1Discovery.inl` のタグボタン描画で
`if (i > 0) ImGui::SameLine()` により全 14 タグが 1 行に並ぶ。
日本語タグ 14 個はほぼすべてのウィンドウ幅で右端を超える。

### 修正

タグボタンループを以下の折り返しロジックに置き換える:

```cpp
// 変更前
ImGui::TextDisabled("タグ:");
for (int i = 0; i < kMacroTagCount; ++i)
{
    if (i > 0)
    {
        ImGui::SameLine();
    }
    // ... ボタン描画
}

// 変更後
ImGui::TextDisabled("タグ:");
const float availWidth = ImGui::GetContentRegionAvail().x;
float rowWidth = 0.0f;
for (int i = 0; i < kMacroTagCount; ++i)
{
    const float btnWidth = ImGui::CalcTextSize(kMacroTags[i]).x
                         + ImGui::GetStyle().FramePadding.x * 2.0f
                         + ImGui::GetStyle().ItemSpacing.x;
    if (i > 0 && rowWidth + btnWidth <= availWidth)
    {
        ImGui::SameLine();
        rowWidth += btnWidth;
    }
    else
    {
        rowWidth = btnWidth;
    }

    const bool active = state.macroTagFilters[i];
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (ImGui::SmallButton(kMacroTags[i]))
    {
        state.macroTagFilters[i] = !state.macroTagFilters[i];
    }
    if (active)
    {
        ImGui::PopStyleColor();
    }
}
```

### 確認
- タグボタンがウィンドウ幅に応じて複数行に折り返されること
- ウィンドウを狭めても全タグが表示・操作できること
- タグ ON/OFF の挙動が変わらないこと

---

## Fix B-2: `荒さ` スライダーと Layer3 表示値の範囲不一致（低優先度・説明追加）

### 問題
Layer2 の `荒さ` が書き込む `filterResonance` の範囲（0.5〜6.0）と、
Layer3 の `Filter Resonance (Q)` スライダーの範囲（0.1〜18.0）が異なる。
Layer2 で `荒さ=1.0` にした後 Layer3 を見ると resonance=6.0 に見え、
スライダー中央付近に位置するため「最大にしたはずなのに半分？」と見える。

γポリシーにより Layer3 編集後も `荒さ` スライダーは動かないため、
UI 上の不整合は生じないが視覚的な混乱の原因になる。

### 修正
Layer3 の `Filter Resonance` スライダー表示に Layer2 の範囲を超えた値であることを
注記として追加する。

`src/gui/channeleditor/ChannelEditorCommon.inl` の
`sliderWaveParam("Filter Resonance (Q)", ...)` の直後に以下を追加:

```cpp
localChanged |= sliderWaveParam("Filter Resonance (Q)", src.filterResonance, 0.1f, 18.0f, "%.2f");
if (updateHoverHelp)
{
    updateHoverHelp(
        "Filter Resonance を調整します。",
        "カットオフ付近の強調量が変わります。Layer2「荒さ」スライダーの書き込み範囲は 0.5〜6.0 です。",
        nullptr);
}
```

### 確認
- `Filter Resonance` のホバーヘルプに Layer2 との範囲差が記載されること

---

## 実施順序の推奨

```
Fix D-1（kMacroTagCount 移動）
  → Fix E-1（タグ折り返し、D-1 前提）
Fix B-1（デバウンス修正）独立
Fix C-1（layer3Changed）独立
Fix A-1（Preset Combo 削除）
  → Fix A-2（Source Type ラベル更新、A-1 後に自然）
Fix B-2（ホバーテキスト追加）最後でよい
```
