# Codex Prompts: 「書き出す」専用モード (Phase 2-E)

## 背景・共通コンテキスト

F-Synthesizer は C++/ImGui ベースのシンセサイザー GUI。
現在 Sound / Music の 2 タブ構成（`UIModeTab = 0/1`）を 3 タブに拡張し、
Export に特化した軽量ビューを追加する。

現行の Export WAV ボタンは Music タブの奥深くに埋もれており初心者が見つけにくい。
Export タブを独立させることで「書き出す」操作への導線を明確化する。

---

### 現行タブ構成

| UIModeTab | タブ名 | 役割 |
|---|---|---|
| 0 | Sound | 音色編集・Tone Preview |
| 1 | Music | MIDI/ミックス設定・Master Effects・Export WAV |
| 2 | Export | **（新規）書き出し専用ビュー** |

Music タブの既存 Export WAV ボタンは **削除しない**（後方互換維持）。
Export タブは Music タブのサブセットとして、書き出しに必要な最小項目のみを提示する。

---

### 画面上の位置

```
┌─ Export タブ（UIModeTab == 2）────────────────────────────────────┐
│  出力先: [/path/to/output.wav          ] [Browse...] [Copy]       │
│  (コンパクトパス表示)                                               │
│                                                                    │
│  Format: 44100 Hz / 16bit   Serial Save: OFF                      │
│  (読み取り専用サマリー。詳細は Music タブで変更)                     │
│  ──────────────────────────────────────                            │
│  Output Target:                                                    │
│  (●) All Channels  ( ) Single Channel  [Target Ch: 0 ▲▼]         │
│  ──────────────────────────────────────                            │
│  [Export WAV]   [Play Preview]   [Stop]                            │
│                                                                    │
│  Level (Preview)                                                   │
│  ████████░░░░░░░░ │  CLIP                                          │
│  ──────────────────────────────────────                            │
│  最近の書き出し:                                                     │
│  • /path/output_20260327_184334.wav (3.2 MB)                      │
│  • ...                                                             │
└────────────────────────────────────────────────────────────────────┘
```

---

### 関連する既存コード

| シンボル | 場所 | 役割 |
|---|---|---|
| `UIModeTab` | `GUIState.h` L54 | タブ識別子（0/1 → 0/1/2 に拡張） |
| `soundLogs` / `musicLogs` | `GUIState.h` L102-103 | タブ別ログバッファ |
| `state.wavPath` | `GUIState.h` (char[1024]) | WAV 出力先パス |
| `state.sampleRate` / `bits` / `serialSave` | `GUIState.h` | フォーマット設定 |
| `state.targetChannel` | `GUIState.h` | -1=All / 0〜15=Single |
| `LogsByTab(state, tab)` | `GUIActions.cpp` L11-18 | tab 番号でログ配列を返す |
| `TryFinalizeCompletedRun` | `GUIRunActions.cpp` L355 | 完了 run の後処理 |
| `state.runIsPreview` | `GUIState.h` | preview か WAV 書き出しかのフラグ |
| `state.lastOutputPath` | `GUIState.h` | 最後の出力パス文字列 |
| `DrawVUMeter(state)` | `main/VUMeter.inl` | VU メーター描画（既存 inl、再利用） |
| `BrowseSavePath` / `CompactPathForUI` | `GUIMain.cpp` 匿名名前空間 | ファイル選択ダイアログ・パス短縮表示 |
| `StartGUIRun(state, false)` | `GUIActions.h` | WAV 書き出し開始（false=WAV write） |
| `StartGUIRun(state, true)` | `GUIActions.h` | Play Preview 開始 |
| `StopGUIRunAndPreview(state)` | `GUIActions.h` | レンダ/再生を停止 |

### `GUIMain.cpp` の `.inl` インクルード順序（現行）

```
GUIMain.cpp (anonymous namespace)
  TopBar.inl
  Layer1Discovery.inl
  Layer2Macros.inl
  VirtualKeyboard.inl
  VUMeter.inl
  MainWindow.inl   ← DrawVUMeter を呼ぶ
```

`ExportView.inl` は `VUMeter.inl` の後、`MainWindow.inl` の前に挿入する。

---

## 設計方針

- **新規ファイル 1 本**（`ExportView.inl`）の追加のみ。既存ファイルへの変更は最小限。
- Export タブは Music タブの内容を **複製しない**。出力パス・書き出し対象・ボタン・VU・履歴のみ。
  フォーマット設定（サンプルレート・ビット深度・Extra Release 等）の編集は Music タブ専任のまま。
- `recentWavPaths` は GUIState に持ち `TryFinalizeCompletedRun` で更新する（最大 5 件）。
- ファイルサイズ取得には `<filesystem>` を使用し、エラー時は表示を省略する（例外を伝搬しない）。
- `LogsByTab` を tab==2 に対応させ、Export タブ操作中のログが `exportLogs` に行くようにする。

---

## T1: `include/gui/GUIState.h` にフィールドを追加

**挿入位置:** L103 の `std::vector<std::string> musicLogs{};` の直後

```cpp
    std::vector<std::string> exportLogs{};
    std::vector<std::string> recentWavPaths{}; // Export 成功時に最大 5 件記録
```

変更量: 2 行追加。

---

## T2: `src/gui/GUIActions.cpp` の `LogsByTab` を tab==2 対応に拡張

**変更位置:** L11-18 の両 `LogsByTab` オーバーロード

```cpp
// 変更前
std::vector<std::string>& LogsByTab(GUIState& state, int tab)
{
    return (tab == 1) ? state.musicLogs : state.soundLogs;
}
const std::vector<std::string>& LogsByTab(const GUIState& state, int tab)
{
    return (tab == 1) ? state.musicLogs : state.soundLogs;
}

// 変更後
std::vector<std::string>& LogsByTab(GUIState& state, int tab)
{
    if (tab == 1) { return state.musicLogs; }
    if (tab == 2) { return state.exportLogs; }
    return state.soundLogs;
}
const std::vector<std::string>& LogsByTab(const GUIState& state, int tab)
{
    if (tab == 1) { return state.musicLogs; }
    if (tab == 2) { return state.exportLogs; }
    return state.soundLogs;
}
```

---

## T3: `src/gui/GUIStateModel.cpp` に `exportLogs` クリアを追加

**変更位置:** L103 の `state.musicLogs.clear();` の直後

```cpp
    state.exportLogs.clear();
```

変更量: 1 行追加。

---

## T4: `src/gui/GUIRunActions.cpp` に最近の書き出し記録を追加

**変更位置:** `TryFinalizeCompletedRun` 内の `if (state.runIsPreview)` ブロック終了後（`if (state.restorePreviewOnRunComplete)` の直前）

```cpp
    // WAV 書き出し完了時に履歴へ記録（最大 5 件）
    if (!state.runIsPreview &&
        state.lastRunExitCode == 0 &&
        !state.lastOutputPath.empty() &&
        state.lastOutputPath != "[memory preview]")
    {
        state.recentWavPaths.insert(state.recentWavPaths.begin(), state.lastOutputPath);
        if (state.recentWavPaths.size() > 5)
        {
            state.recentWavPaths.resize(5);
        }
    }
```

**注意:** `state.runIsPreview` は `if (state.runIsPreview)` ブロック末尾（L420）で `false` に更新済みのため、
このコードは `state.runIsPreview` が `false` にリセットされた後に挿入する必要がある。
`state.lastOutputPath` は run 開始時（`StartGUIRun`）に設定済みなので参照可能。

---

## T5: `src/gui/main/ExportView.inl` を新規作成

```cpp
// ExportView.inl
// 「書き出す」専用ビュー（UIModeTab == 2 用）。
// DrawExportView(state, updateHoverHelp) を MainWindow.inl の Export ブランチから呼び出す。
// GUIMain.cpp 匿名名前空間に VUMeter.inl の後に #include して使用する。

#include <filesystem>
#include <functional>

static void DrawExportView(
    GUIState& state,
    const std::function<void(const char*, const char*, const char*)>& updateHoverHelp)
{
    ImGui::BeginDisabled(state.running);

    // ---- 出力先パス ----
    state.presetDirty |= ImGui::InputText("Output Path", state.wavPath, IM_ARRAYSIZE(state.wavPath));
    updateHoverHelp(
        "WAVの書き出し先パスを指定します。",
        "WAVの出力先が変わります。",
        "Serial Save が無効だと既存ファイルを上書きする場合があります。");
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        std::string selected;
        const wchar_t* wavFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
        if (BrowseSavePath(state.wavPath, wavFilter, L"wav", selected))
        {
            strncpy_s(state.wavPath, sizeof(state.wavPath), selected.c_str(), _TRUNCATE);
            state.presetDirty = true;
        }
    }
    updateHoverHelp(
        "出力先WAVパスを選択するダイアログを開きます。",
        "Output Pathを更新します。",
        "保存権限のない場所は失敗します。");
    ImGui::SameLine();
    if (ImGui::Button("Copy##wavPath"))
    {
        ImGui::SetClipboardText(state.wavPath);
    }
    updateHoverHelp("Output Path をコピーします。", "パスをクリップボードへコピーします。", nullptr);
    {
        const std::string compact = CompactPathForUI(state.wavPath);
        ImGui::TextDisabled("%s", compact.c_str());
        if (ImGui::IsItemHovered() && std::strlen(state.wavPath) > 0)
        {
            ImGui::SetTooltip("%s", state.wavPath);
        }
    }

    // ---- フォーマットサマリー（読み取り専用） ----
    ImGui::Separator();
    ImGui::TextDisabled("Format: %d Hz / %dbit   Serial Save: %s",
        state.sampleRate, state.bits, state.serialSave ? "ON" : "OFF");
    updateHoverHelp(
        "現在のフォーマット設定のサマリーです。",
        "詳細設定は Music タブの Render Settings で変更できます。",
        nullptr);

    // ---- 出力対象 ----
    ImGui::Separator();
    ImGui::TextUnformatted("Output Target");
    const int outputMode = (state.targetChannel < 0) ? 0 : 1;
    if (ImGui::RadioButton("All Channels", outputMode == 0))
    {
        state.targetChannel = -1;
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を All Channels にします。",
        "全MIDIチャンネルを出力します。",
        nullptr);
    ImGui::SameLine();
    if (ImGui::RadioButton("Single Channel", outputMode == 1))
    {
        state.targetChannel = std::clamp(state.targetChannel, 0, 15);
        if (state.targetChannel < 0)
        {
            state.targetChannel = 0;
        }
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を Single Channel にします。",
        "指定chのみを出力します。",
        nullptr);
    if (state.targetChannel >= 0)
    {
        ImGui::SetNextItemWidth(220.0f);
        int singleTarget = std::clamp(state.targetChannel, 0, 15);
        if (ImGui::SliderInt("Target Ch##export", &singleTarget, 0, 15))
        {
            state.targetChannel = singleTarget;
            state.presetDirty = true;
        }
        updateHoverHelp(
            "書き出し対象のチャンネルを変更します。",
            "Export対象chが変わります。",
            nullptr);
    }

    ImGui::EndDisabled();

    // ---- ボタン ----
    ImGui::Separator();
    ImGui::BeginDisabled(state.running);
    if (ImGui::Button("Export WAV"))
    {
        StartGUIRun(state, false);
    }
    updateHoverHelp(
        "Export WAVを実行します。",
        "現在設定でWAVを書き出します。",
        "再生中の場合は停止してから書き出しを開始します。");
    ImGui::SameLine();
    if (ImGui::Button("Play Preview"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "Play Previewを実行します。",
        "現在のMIDI設定でメモリ再生します。",
        "WAVファイルは出力しません。");
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool canStop = state.running || state.playback.playing.load(std::memory_order_relaxed);
    ImGui::BeginDisabled(!canStop);
    if (ImGui::Button("Stop##export"))
    {
        StopGUIRunAndPreview(state);
    }
    updateHoverHelp(
        "レンダまたは再生を停止します。",
        "処理を中断します。",
        "未実行時は無効です。");
    ImGui::EndDisabled();

    // ---- VU メーター（Phase 1-C の DrawVUMeter を流用） ----
    DrawVUMeter(state);

    // ---- 最近の書き出し ----
    if (!state.recentWavPaths.empty())
    {
        ImGui::Separator();
        ImGui::TextDisabled("最近の書き出し:");
        for (const std::string& path : state.recentWavPaths)
        {
            std::string label = CompactPathForUI(path);
            std::error_code ec;
            const auto sz = std::filesystem::file_size(path, ec);
            if (!ec && sz > 0)
            {
                const double mb = static_cast<double>(sz) / (1024.0 * 1024.0);
                char sizeBuf[32];
                snprintf(sizeBuf, sizeof(sizeBuf), " (%.1f MB)", mb);
                label += sizeBuf;
            }
            ImGui::TextUnformatted(label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", path.c_str());
            }
        }
    }
}
```

---

## T6: `src/gui/GUIMain.cpp` に `ExportView.inl` をインクルード

**変更位置:** L65 の `#include "main/VUMeter.inl"` の直後、`#include "main/MainWindow.inl"` の直前

```cpp
#include "main/VUMeter.inl"
#include "main/ExportView.inl"   // ← 追加
#include "main/MainWindow.inl"
```

---

## T7: `src/gui/main/MainWindow.inl` に Export タブを追加

### 7-1. タブバー（L134-158 付近）

現行の `BeginTabBar` ブロックを以下に変更する（`musicFlags` 宣言の直後に `exportFlags` を追加し、タブ項目を 1 つ追加）。

```cpp
    if (ImGui::BeginTabBar("mode_tabs"))
    {
        const bool needSync = (syncedTab != state.UIModeTab);
        ImGuiTabItemFlags soundFlags =
            (needSync && state.UIModeTab == 0) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        ImGuiTabItemFlags musicFlags =
            (needSync && state.UIModeTab == 1) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        ImGuiTabItemFlags exportFlags =
            (needSync && state.UIModeTab == 2) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        if (ImGui::BeginTabItem("Sound", nullptr, soundFlags))
        {
            state.UIModeTab = 0;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Soundモードへ切り替えます。",
            "Sound編集と試聴の操作を表示します。");
        if (ImGui::BeginTabItem("Music", nullptr, musicFlags))
        {
            state.UIModeTab = 1;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Musicモードへ切り替えます。",
            "MIDIとミックス設定の操作を表示します。");
        if (ImGui::BeginTabItem("Export", nullptr, exportFlags))
        {
            state.UIModeTab = 2;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Exportモードへ切り替えます。",
            "WAV書き出しに特化した操作を表示します。");
        ImGui::EndTabBar();
        syncedTab = state.UIModeTab;
    }
```

### 7-2. タブ別コンテンツ（`if (state.UIModeTab == 0)` 〜 `else` ブロック付近）

現行の構造:
```cpp
if (state.UIModeTab == 0)
{
    // Sound タブ
}
else
{
    // Music タブ (UIModeTab == 1)
}
```

変更後:
```cpp
if (state.UIModeTab == 0)
{
    // Sound タブ（変更なし）
}
else if (state.UIModeTab == 2)
{
    DrawExportView(state, updateHoverHelp);
}
else
{
    // Music タブ（UIModeTab == 1、変更なし）
}
```

**挿入位置:** `if (state.UIModeTab == 0)` ブロックの終わり（`}`）の直後、`else {` の直前に `else if` ブロックを挿入する。

### 7-3. ログ表示（L1371 付近）

```cpp
// 変更前
const std::vector<std::string>& visibleLogs = (state.UIModeTab == 1) ? state.musicLogs : state.soundLogs;

// 変更後
const std::vector<std::string>& visibleLogs =
    (state.UIModeTab == 1) ? state.musicLogs :
    (state.UIModeTab == 2) ? state.exportLogs :
    state.soundLogs;
```

---

## 統合チェックリスト

1. - [ ] `GUIState` に `exportLogs` と `recentWavPaths` フィールドが追加されている
2. - [ ] `LogsByTab(state, 2)` が `state.exportLogs` を返す
3. - [ ] Export タブに切り替えると UIModeTab == 2 になる
4. - [ ] Export タブで `Export WAV` を押すと WAV が書き出される
5. - [ ] Export タブで `Play Preview` を押すとメモリ内再生が始まる
6. - [ ] Export タブで `Stop` ボタンが再生/レンダ中のみ有効になる
7. - [ ] Export 成功後に `recentWavPaths` に 1 件追加される（最大 5 件）
8. - [ ] 最近の書き出し一覧にファイルサイズが表示される（ファイルが存在する場合のみ）
9. - [ ] フォーマットサマリーが現在の `sampleRate`/`bits`/`serialSave` を反映する
10. - [ ] VU メーターが Export タブでも動作する（DrawVUMeter を流用）
11. - [ ] Sound/Music タブの既存動作が変わらない
12. - [ ] タブ切替時に再生が停止する（既存 stop-on-tab-switch ロジックは tab==2 でも機能する）

---

## 実装上の注意

### `std::filesystem::file_size` のエラー処理

`std::filesystem::file_size(path, ec)` は `ec` に `std::error_code` を受け取るオーバーロードを使う。
ファイルが存在しない場合や権限エラー時は `ec` が non-zero になり、サイズは 0 として無視する。
例外版（`ec` なし）は使わないこと（書き出し直後にファイルが消えている可能性がある）。

### 既存 Music タブの Export WAV ボタンは削除しない

ロードマップは「分離」であり「廃止」ではない。
Music タブの Export WAV ボタンはそのまま残す。
この実装では Music タブへの変更は一切行わない。

### `SingleChannel` ラジオの初期化

`SingleChannel` を ON にした時点で `state.targetChannel` が -1 のままだと後続の `SliderInt` が不正になる。
`RadioButton` クリック時に `targetChannel` を 0〜15 の範囲にクランプする処理を必ず入れること（T5 参照）。

### タブ切替時の停止ロジック

`MainWindow.inl` L180-189 の stop-on-tab-switch は `(state.UIModeTab != lastFrameTab)` で判定するため、
tab==2 が追加されても追加変更なしに機能する。

### `hoverHelp` の初期化（L32）

`MainWindow.inl` 先頭の:
```cpp
std::string hoverHelp = (state.UIModeTab == 0)
    ? "Sound タブ: 音色を編集します。"
    : "Music タブ: MIDIと書き出しを設定します。";
```
は tab==2 時に "Music タブ..." と表示されるが、フレーム内で `updateHoverHelp` が上書きするため実用上は問題ない。
厳密にしたい場合のみ以下に変更する:

```cpp
std::string hoverHelp = (state.UIModeTab == 0) ? "Sound タブ: 音色を編集します。"
    : (state.UIModeTab == 2) ? "Export タブ: WAVを書き出します。"
    : "Music タブ: MIDIと書き出しを設定します。";
```
