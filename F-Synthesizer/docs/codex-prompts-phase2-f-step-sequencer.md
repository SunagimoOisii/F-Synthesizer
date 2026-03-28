# Codex 実装プロンプト — Phase 2-F: ドラムチャンネル ステップシーケンサービュー

> **前提**: Phase 1-A (仮想キーボード) 完了済み。本タスクは Music タブの ch10 表示時に
> Piano Roll の代替ビューとしてステップシーケンサーを追加する。

---

## 背景・設計方針

Music タブで `pianoRoll.displayChannel == 9`（MIDI ch10）かつ
`state.drumChannelSpecialHandling == true` のとき、ピアノロールの下に
`[Piano Roll] / [Step Seq]` 切替ボタンを出す。

Step Seq ビューでは 7 行 × 16 ステップのグリッドを表示し、
ステップを ON にすると `pianoRoll.notes` の ch9 ノートが即座に書き換わり、
既存の `BuildOverrideNoteTicksFromPianoRoll` 経由で Play Preview / Export WAV に反映される。

### データフロー

```
[StepSeq UI] toggle step
    → SyncStepSeqToPianoRoll()
        → pianoRoll.notes (ch9 を差し替え)
        → ApplyStepSeqNotes() [新規公開関数]
            → SyncProjectDataFromCurrentNotes() [内部]
    → BuildOverrideNoteTicksFromPianoRoll() が ch9 ノートを包含
    → Play Preview / Export WAV に反映
```

### 行と MIDI ノートの対応（固定）

| row | label     | MIDI note |
|-----|-----------|-----------|
| 0   | Kick      | 36        |
| 1   | Snare     | 38        |
| 2   | ClosedHH  | 42        |
| 3   | OpenHH    | 46        |
| 4   | LowTom    | 45        |
| 5   | MidTom    | 48        |
| 6   | Crash     | 49        |

---

## タスク一覧

| # | ファイル | 内容 |
|---|---|---|
| T1 | `include/gui/GUIState.h` | `GUIStepSeqState` 構造体 + `GUIState::stepSeq` フィールド追加 |
| T2 | `include/gui/GUIStateStorage.h` | `GUIStateStorageData` に永続化フィールド追加 |
| T3 | `src/gui/GUIStateStorage.cpp` | JSON 読み書きに step seq フィールドを追加 |
| T4 | `src/gui/GUIStatePersistence.cpp` | `ToStorage` / `FromStorage` に step seq 変換を追加 |
| T5 | `include/gui/GUIPianoRoll.h` | `ApplyStepSeqNotes` を公開 API として追加 |
| T6 | `src/gui/GUIPianoRoll.cpp` | `ApplyStepSeqNotes` の実装を追加 |
| T7 | 新規 `src/gui/main/StepSequencer.inl` | ステップグリッド描画 + 行ラベル + sync ヘルパー |
| T8 | `src/gui/GUIMain.cpp` | `StepSequencer.inl` を include |
| T9 | `src/gui/main/MainWindow.inl` | Music タブに切替ボタン + 条件分岐描画 |

---

## T1 — `include/gui/GUIState.h`

### 追加するコード

`GUIState` 構造体の先頭（`struct GUIRunObserver` の前）に新しい構造体を追加し、
その後に `GUIState` のフィールドとして宣言する。

```cpp
// ステップシーケンサー状態 (ch10 / drumChannel 専用)
struct GUIStepSeqState
{
    static constexpr int kRows  = 7;
    static constexpr int kSteps = 16;
    bool steps[kRows][kSteps]{};       // アクティブなステップ
    int  velocity[kRows]{};            // 行ごとのベロシティ (1-127, 初期値 100)
    bool viewActive = false;           // true=StepSeq表示, false=PianoRoll表示
};
```

`GUIState` 末尾（`pianoRoll` フィールドの直後）に追加:

```cpp
GUIStepSeqState stepSeq{};
```

**初期化**: `GUIStepSeqState` のコンストラクタは `steps` を全 `false`、
`velocity` を全 `100` に初期化する。
`bool steps[kRows][kSteps]{}` のゼロ初期化と `velocity` 初期化は
`GUIStepSeqState` のデフォルトコンストラクタを追加して行う:

```cpp
GUIStepSeqState()
{
    for (int r = 0; r < kRows; ++r) { velocity[r] = 100; }
}
```

---

## T2 — `include/gui/GUIStateStorage.h`

`GUIStateStorageData` 末尾に追加（`drumChannelSpecialHandling` の後）:

```cpp
bool stepSeqViewActive = false;
// row ごとに 16 ビットのステップビットマスク (bit0 = step0)
std::array<uint16_t, 7> stepSeqStepBits{};
// row ごとのベロシティ (1-127)
std::array<int, 7> stepSeqVelocity{ 100, 100, 100, 100, 100, 100, 100 };
```

---

## T3 — `src/gui/GUIStateStorage.cpp`

### 書き込み (`SaveGUIStateStorageFile` 内)

既存の `prDrumNameMode` 書き込みの直後に追加:

```cpp
fout << "  \"stepSeqViewActive\": " << (data.stepSeqViewActive ? "true" : "false") << ",\n";
for (int r = 0; r < 7; ++r)
{
    fout << "  \"stepSeqBits" << r << "\": " << data.stepSeqStepBits[r] << ",\n";
    fout << "  \"stepSeqVel"  << r << "\": " << data.stepSeqVelocity[r] << ",\n";
}
```

### 読み込み (`LoadGUIStateStorageFile` 内)

既存の `prDrumNameMode` 読み込みの直後に追加:

```cpp
if (auto v = ReadJSONBool(text, "stepSeqViewActive")) data.stepSeqViewActive = *v;
for (int r = 0; r < 7; ++r)
{
    const std::string bitsKey = "stepSeqBits" + std::to_string(r);
    const std::string velKey  = "stepSeqVel"  + std::to_string(r);
    if (auto v = ReadJSONInt(text, bitsKey.c_str()))
    {
        data.stepSeqStepBits[r] = static_cast<uint16_t>(std::clamp(*v, 0, 65535));
    }
    if (auto v = ReadJSONInt(text, velKey.c_str()))
    {
        data.stepSeqVelocity[r] = std::clamp(*v, 1, 127);
    }
}
```

> `ReadJSONInt` がない場合は既存の `ReadJSONDouble` を `static_cast<int>` して代替するか、
> プロジェクト内の `ReadJSONInt` 相当関数を使用する。

---

## T4 — `src/gui/GUIStatePersistence.cpp`

### `ToStorage` に追加

`data.prDrumNameMode = state.pianoRoll.drumNameMode;` の直後:

```cpp
data.stepSeqViewActive = state.stepSeq.viewActive;
for (int r = 0; r < GUIStepSeqState::kRows; ++r)
{
    uint16_t bits = 0;
    for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
    {
        if (state.stepSeq.steps[r][s]) { bits |= static_cast<uint16_t>(1u << s); }
    }
    data.stepSeqStepBits[r] = bits;
    data.stepSeqVelocity[r] = state.stepSeq.velocity[r];
}
```

### `FromStorage` に追加

`state.pianoRoll.drumNameMode = data.prDrumNameMode;` の直後:

```cpp
state.stepSeq.viewActive = data.stepSeqViewActive;
for (int r = 0; r < GUIStepSeqState::kRows; ++r)
{
    const uint16_t bits = data.stepSeqStepBits[r];
    for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
    {
        state.stepSeq.steps[r][s] = (bits >> s) & 1u;
    }
    state.stepSeq.velocity[r] = std::clamp(data.stepSeqVelocity[r], 1, 127);
}
```

---

## T5 — `include/gui/GUIPianoRoll.h`

`DrawPianoRollPanel` の宣言の直後に追加:

```cpp
// ステップシーケンサーから生成した ch9 ノートを pianoRoll.notes に適用し、
// プロジェクトデータを同期する。既存の ch9 ノートは全て置き換えられる。
void ApplyStepSeqNotes(PianoRollState& state,
                       const std::vector<PianoRollNote>& ch9Notes);
```

---

## T6 — `src/gui/GUIPianoRoll.cpp`

`DrawPianoRollPanel` の実装の後（または前）に追加:

```cpp
void gui::ApplyStepSeqNotes(PianoRollState& state,
                             const std::vector<PianoRollNote>& ch9Notes)
{
    // 既存の ch9 ノートを除去
    state.notes.erase(
        std::remove_if(state.notes.begin(), state.notes.end(),
            [](const PianoRollNote& n) { return n.channel == 9; }),
        state.notes.end());

    // 新しい ch9 ノートを追加
    state.notes.insert(state.notes.end(), ch9Notes.begin(), ch9Notes.end());

    // tick 順にソート
    std::sort(state.notes.begin(), state.notes.end(),
        [](const PianoRollNote& a, const PianoRollNote& b) {
            return a.startTick < b.startTick;
        });

    // maxTick を更新
    state.maxTick = 0;
    for (const auto& n : state.notes)
    {
        state.maxTick = (std::max)(state.maxTick, n.endTick);
    }

    // プロジェクトデータに反映 (BuildOverrideNoteTicksFromPianoRoll が参照する)
    SyncProjectDataFromCurrentNotes(state);
    ++state.notesVersion;
}
```

**`SyncProjectDataFromCurrentNotes`** は `pianoRollEdit.inl` 内の無名名前空間か
`GUIPianoRoll.cpp` に含まれる関数。`GUIPianoRoll.cpp` が `pianoroll/PianoRollEdit.inl` を
`#include` していれば直接呼べる。ヘッダ宣言がない場合は `GUIPianoRoll.cpp` 内に
前方宣言を追加するか、`PianoRollEdit.inl` の `SyncProjectDataFromCurrentNotes` を
`GUIPianoRoll.h` に公開する。

> **確認事項**: `GUIPianoRoll.cpp` の include 構造を確認し、
> `SyncProjectDataFromCurrentNotes` がスコープ内にあることを確認してから実装する。

---

## T7 — 新規 `src/gui/main/StepSequencer.inl`

`GUIMain.cpp` の匿名名前空間に `#include` されるファイル。

```cpp
// StepSequencer.inl
// DrawStepSeqPanel: GUIMain.cpp 匿名名前空間から呼び出す。
// ApplyStepSeqNotes を介して pianoRoll.notes を更新する。

// 行定義
struct StepSeqRowDef { int midiNote; const char* label; };
static constexpr StepSeqRowDef kSSRows[GUIStepSeqState::kRows] = {
    { 36, "Kick"  },
    { 38, "Snare" },
    { 42, "C.HH"  },
    { 46, "O.HH"  },
    { 45, "LTom"  },
    { 48, "MTom"  },
    { 49, "Crash" },
};

// pianoRoll.notes の ch9 ノートからグリッドを復元する
static void LoadStepSeqFromPianoRoll(GUIStepSeqState& ss,
                                     const gui::PianoRollState& pr)
{
    // 既存のグリッドをリセット
    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
            ss.steps[r][s] = false;

    const int tpq = (pr.ticksPerQuarter > 0) ? pr.ticksPerQuarter : 480;
    const int stepLen = tpq / 4; // 1/16 note
    if (stepLen <= 0) { return; }

    for (const auto& n : pr.notes)
    {
        if (n.channel != 9) { continue; }
        // どの row に対応するか
        for (int r = 0; r < GUIStepSeqState::kRows; ++r)
        {
            if (kSSRows[r].midiNote != n.note) { continue; }
            // どの step か (measure 0 のみ: 0 〜 16*stepLen-1)
            const int step = n.startTick / stepLen;
            if (step >= 0 && step < GUIStepSeqState::kSteps)
            {
                ss.steps[r][step] = true;
            }
            break;
        }
    }
}

// グリッドから PianoRollNote リストを生成して ApplyStepSeqNotes を呼ぶ
static void FlushStepSeqToPianoRoll(const GUIStepSeqState& ss,
                                     gui::PianoRollState& pr)
{
    const int tpq = (pr.ticksPerQuarter > 0) ? pr.ticksPerQuarter : 480;
    const int stepLen = tpq / 4; // 1/16 note

    std::vector<gui::PianoRollNote> ch9Notes;
    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            if (!ss.steps[r][s]) { continue; }
            gui::PianoRollNote n{};
            n.channel  = 9;
            n.note     = kSSRows[r].midiNote;
            n.velocity = ss.velocity[r];
            n.startTick = s * stepLen;
            n.endTick   = n.startTick + stepLen - 1;
            ch9Notes.push_back(n);
        }
    }
    gui::ApplyStepSeqNotes(pr, ch9Notes);
}

static void DrawStepSeqPanel(GUIState& state)
{
    GUIStepSeqState& ss = state.stepSeq;
    gui::PianoRollState& pr = state.pianoRoll;

    constexpr float kRowH   = 22.0f; // 行の高さ
    constexpr float kLabelW = 52.0f; // 行ラベル幅
    constexpr float kStepW  = 22.0f; // 1 ステップ幅
    constexpr float kVelW   = 48.0f; // ベロシティ欄幅

    // --- ヘッダ: ステップ番号 ---
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kLabelW + 4.0f);
    for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
    {
        if (s > 0) { ImGui::SameLine(0.0f, 2.0f); }
        // 4 拍子グループ境界に薄い区切り
        if (s % 4 == 0 && s > 0) { ImGui::SameLine(0.0f, 6.0f); }
        ImGui::TextDisabled("%d", s + 1);
    }

    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        ImGui::PushID(r);

        // --- 行ラベル ---
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(kSSRows[r].label);
        ImGui::SameLine(kLabelW + 4.0f);

        // --- ステップボタン ---
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            if (s > 0) { ImGui::SameLine(0.0f, 2.0f); }
            if (s % 4 == 0 && s > 0) { ImGui::SameLine(0.0f, 6.0f); }

            ImGui::PushID(s);
            const bool on = ss.steps[r][s];
            if (on)
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(180, 120, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(210, 150, 60, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(150, 90, 20, 255));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(55, 55, 55, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(90, 90, 90, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(35, 35, 35, 255));
            }
            if (ImGui::Button("##s", ImVec2(kStepW, kRowH)))
            {
                ss.steps[r][s] = !ss.steps[r][s];
                FlushStepSeqToPianoRoll(ss, pr);
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }

        // --- ベロシティ ---
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetNextItemWidth(kVelW);
        if (ImGui::SliderInt("##vel", &ss.velocity[r], 1, 127, "%d"))
        {
            FlushStepSeqToPianoRoll(ss, pr);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Row velocity: %d", ss.velocity[r]);
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    if (ImGui::Button("Clear All"))
    {
        for (int r = 0; r < GUIStepSeqState::kRows; ++r)
            for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
                ss.steps[r][s] = false;
        FlushStepSeqToPianoRoll(ss, pr);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("全ステップをクリアします。");
    }
}
```

---

## T8 — `src/gui/GUIMain.cpp`

`#include "main/VUMeter.inl"` の直後（`ExportView.inl` より前）に追加:

```cpp
#include "main/StepSequencer.inl"
```

---

## T9 — `src/gui/main/MainWindow.inl`

### 変更箇所: `DrawPianoRollPanel` 呼び出しの前後

現在は Music タブの末尾近くで無条件に `DrawPianoRollPanel(...)` を呼んでいる。
これを以下のように変更する。

**変更前 (L1346付近)**:
```cpp
ImGui::Separator();
DrawPianoRollPanel(
    state.pianoRoll,
    state.midiPath,
    &state.playback,
    [&](const std::string& line) { AppendGUILog(state, line); },
    [&]() { StartGUIRun(state, true); },
    [&]() { StopGUIRunAndPreview(state); });
```

**変更後**:
```cpp
ImGui::Separator();

// ch10 ドラムチャンネル表示中はビュー切替ボタンを表示する
const bool isShowingDrumCh = (state.pianoRoll.displayChannel == drumMidiChannel)
                              && state.drumChannelSpecialHandling;
if (isShowingDrumCh)
{
    // [Piano Roll] トグルボタン
    const bool prSelected = !state.stepSeq.viewActive;
    if (prSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(50, 90, 170, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(70, 110, 200, 255));
    }
    if (ImGui::Button("Piano Roll"))
    {
        if (state.stepSeq.viewActive)
        {
            state.stepSeq.viewActive = false;
        }
    }
    if (prSelected) { ImGui::PopStyleColor(2); }
    updateHoverHelp("Piano Roll ビューへ切り替えます。", "通常のピアノロールが表示されます。");

    ImGui::SameLine();

    // [Step Seq] トグルボタン
    const bool ssSelected = state.stepSeq.viewActive;
    if (ssSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(50, 90, 170, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(70, 110, 200, 255));
    }
    if (ImGui::Button("Step Seq"))
    {
        if (!state.stepSeq.viewActive)
        {
            // ビュー切替時に pianoRoll.notes の ch9 ノートをグリッドへ読み込む
            LoadStepSeqFromPianoRoll(state.stepSeq, state.pianoRoll);
            state.stepSeq.viewActive = true;
        }
    }
    if (ssSelected) { ImGui::PopStyleColor(2); }
    updateHoverHelp("Step Seq ビューへ切り替えます。", "16ステップのドラムグリッドが表示されます。");
}

if (isShowingDrumCh && state.stepSeq.viewActive)
{
    DrawStepSeqPanel(state);
}
else
{
    DrawPianoRollPanel(
        state.pianoRoll,
        state.midiPath,
        &state.playback,
        [&](const std::string& line) { AppendGUILog(state, line); },
        [&]() { StartGUIRun(state, true); },
        [&]() { StopGUIRunAndPreview(state); });
}
```

---

## 受け入れチェックリスト

- [ ] `ch10` 以外のチャンネルを表示しているときは `[Piano Roll] [Step Seq]` ボタンが表示されない
- [ ] `ch10` 表示中に `[Step Seq]` ボタンで切り替えられる
- [ ] `[Piano Roll]` ボタンで元のピアノロールに戻せる
- [ ] `[Piano Roll]` → `[Step Seq]` 切替時に既存 ch9 ノートがグリッドに読み込まれる
- [ ] ステップを ON にすると `pianoRoll.notes` が更新される
- [ ] ベロシティスライダーを変更すると `pianoRoll.notes` の velocity が更新される
- [ ] `[Clear All]` で全ステップが OFF になり `pianoRoll.notes` から ch9 ノートが消える
- [ ] Step Seq で編集後に Play Preview を実行すると ch9 ノートが鳴る
- [ ] Step Seq で編集後に Export WAV を実行すると ch9 ノートが書き出しに含まれる
- [ ] ワークスペース保存・読み込みでグリッド状態が復元される
- [ ] `drumChannelSpecialHandling = false` の時は切替ボタンが表示されない
- [ ] `./F-Synthesizer/scripts/check.ps1` が 0 errors で通る

---

## 実装上の注意

### `SyncProjectDataFromCurrentNotes` のスコープ問題

`SyncProjectDataFromCurrentNotes` は `pianoroll/PianoRollEdit.inl` 内で定義されており、
`GUIPianoRoll.cpp` が `#include "pianoroll/PianoRollEdit.inl"` している場合にのみスコープに入る。

`T6` の `ApplyStepSeqNotes` 実装前に `GUIPianoRoll.cpp` の include 構造を確認し、
`SyncProjectDataFromCurrentNotes` が呼べる位置に実装すること。

呼べない場合の代替:
- `SyncProjectDataFromCurrentNotes` を `namespace gui` の公開関数として `GUIPianoRoll.h` に追加する
- または `GUIPianoRoll.cpp` 内の `SyncProjectDataFromCurrentNotes` ラッパーとして `ApplyStepSeqNotes` を実装する

### `BuildOverrideNoteTicksFromPianoRoll` の前提条件

`BuildOverrideNoteTicksFromPianoRoll` は以下の条件が揃わないと `nullptr` を返す:

```cpp
if (pr.hasLoadError || pr.notes.empty() || pr.ticksPerQuarter <= 0) return nullptr;
if (currentMidiPath.empty() || pr.loadedMidiPath != currentMidiPath) return nullptr;
```

Step Seq を使う場合でもユーザーは MIDI ファイルをロードしている必要がある。
Step Seq は **MIDI ファイルの ch9 部分を上書き編集する** という位置づけ。
ロードされていない場合は `DrawStepSeqPanel` 上部に
`ImGui::TextDisabled("MIDI ファイルをロードするとステップが反映されます。")` を表示する。

### `notesVersion` インクリメント

`ApplyStepSeqNotes` 内で `++state.notesVersion` を呼ぶことで
ピアノロールの可視キャッシュが自動的に無効化される。

### ch9 以外のノートへの影響

`ApplyStepSeqNotes` は **ch9 のみ**を差し替え、他チャンネルのノートは保持する。
他チャンネルの既存ピアノロール編集と共存できる。

### ステップ数とグリッドの拡張（将来）

現在は 16 ステップ固定。将来的に 32 ステップ対応する場合は
`kSteps` を拡張し、ストレージの `uint16_t` を `uint32_t` に変更する必要がある。
この変更は後方互換性を壊すため、その際は JSON キー名を変更すること。
