# Codex Prompts: コードモード（仮想キーボード拡張）(Phase 2-C)

## 背景・共通コンテキスト

F-Synthesizer は C++/ImGui ベースのシンセサイザー GUI。
Phase 1-A で実装した仮想キーボード (`VirtualKeyboard.inl`) に Chord モードを追加し、
1 クリックで和音の構成音を同時 Tone Preview する機能を実装する。

Phase 2-C は Phase 1-A（仮想キーボード）完了後の機能拡張である。
`VirtualKeyboard.inl` 単体の変更と、`StartGUISoundTonePreview` への分岐追加が主な作業となる。

---

### 画面上の位置

```
┌─ Sound タブ 左カラム下部 ────────────────────────────────────┐
│  [■] Waveform ビューア  │ スペクトラム  │ VU メーター         │
│─────────────────────────────────────────────────────────────│
│  Preview Note: [60 ▼]                                        │
│─────────────────────────────────────────────────────────────│
│  [✓] Chord  [Major       ▼]          ← NEW（仮想キーボード上）│
│─────────────────────────────────────────────────────────────│
│  ┌──────────────────── 仮想キーボード ───────────────────┐   │
│  │  [白][黒][白][白][黒][白][黒][白][白][黒][白][黒][白]  │   │
│  │  C2  C#2  D2  ...  選択鍵 = 青、コード構成音 = 緑     │   │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

**Chord OFF（従来動作）:** クリックした鍵 1 音だけを Tone Preview
**Chord ON:** ルート音 + コード構成音を同時に Tone Preview（青=ルート、緑=構成音）

---

### コード種別と半音オフセット

| `chordType` | ラベル | 半音オフセット | 構成音数 |
|---|---|---|---|
| 0 | Major | {0, 4, 7} | 3 |
| 1 | Minor | {0, 3, 7} | 3 |
| 2 | 7th | {0, 4, 7, 10} | 4 |
| 3 | Minor 7th | {0, 3, 7, 10} | 4 |
| 4 | Sus4 | {0, 5, 7} | 3 |

---

### 関連する既存コード

| シンボル | 場所 | 役割 |
|---|---|---|
| `DrawVirtualKeyboard(state)` | `src/gui/main/VirtualKeyboard.inl` | 仮想キーボード描画・クリック処理 |
| `DrawVirtualKeyboard` 呼び出し | `src/gui/main/MainWindow.inl` L859 | Sound タブ下部 |
| `state.tonePreviewNoteNumber` | `include/gui/GUIState.h` L64 | クリックされた鍵のノート番号 |
| `StartGUISoundTonePreview(state)` | `src/gui/GUIPreviewActions.cpp` L193 | 単音プレビュー発火 |
| `BuildOverrideNoteTicksForSoundTone` | `src/gui/GUIPreviewActions.cpp` L50 | 単音 MIDIEventTick ベクタ生成 |
| `ResolveSoundTonePreviewNote` | `src/gui/GUIPreviewActions.cpp` L88 | DrumKit 対応ノート解決 |
| `cfg.overrideNoteTicks` | `AppCore.h` | Run に注入する MIDI イベント列 |

### `.inl` インクルード・コールスタック

```
MainWindow.inl
  └─ DrawVirtualKeyboard(state)   ← VirtualKeyboard.inl 内に定義
       └─ StartGUISoundTonePreview(state)   ← GUIPreviewActions.cpp に定義
            └─ BuildOverrideNoteTicksForSoundTone(...)  または
               BuildOverrideNoteTicksForChord(...)      ← 本 phase で追加
```

---

## 設計方針

- **状態は `GUIState` に 2 フィールド追加**するだけ。新ファイルは不要。
- **`StartGUISoundTonePreview` に分岐を追加**して chord 時の tick 生成を切り替える。
  新関数 `StartGUISoundChordPreview` を別途作らない（呼び出し側の変更が不要になる）。
- **コード構成音のハイライトは `VirtualKeyboard.inl` 内でローカルに計算**する。
  GUIState に構成音リストは持たせない。
- DrumKit 時は従来通り `DrawVirtualKeyboard` 先頭で `return` するため Chord UI も非表示になる。
- オフセット適用後のノート番号は `std::clamp(0, 127)` で MIDI 範囲を保証する。

---

## T1: `include/gui/GUIState.h` に chord フィールドを追加

**挿入位置:** L64 の `int tonePreviewNoteNumber = 60;` の直後

```cpp
    bool chordModeEnabled = false;
    int chordType = 0; // 0=Major 1=Minor 2=7th 3=Minor7th 4=Sus4
```

変更量: 2 行追加。初期値は OFF（単音モード）。

---

## T2: `src/gui/GUIPreviewActions.cpp` にコード tick 生成を追加

### 2-1. `BuildOverrideNoteTicksForChord` ヘルパーを追加

**挿入位置:** 既存の `BuildOverrideNoteTicksForSoundTone` 関数（L50）の直前（無名名前空間内）

```cpp
std::shared_ptr<const std::vector<MIDIEventTick>> BuildOverrideNoteTicksForChord(
    int channel,
    const std::array<int, 4>& notes,
    int noteCount,
    int velocity,
    int ticksPerQuarter)
{
    auto ticks = std::make_shared<std::vector<MIDIEventTick>>();
    ticks->reserve(static_cast<size_t>(noteCount) * 2);

    for (int i = 0; i < noteCount; ++i)
    {
        MIDIEventTick on{};
        on.type = MIDIEventType::Note;
        on.tick = 0;
        on.noteNumber = std::clamp(notes[i], 0, 127);
        on.velocity = std::clamp(velocity, 1, 127);
        on.channel = std::clamp(channel, 0, 15);
        on.controller = 0;
        on.value = 0;
        on.noteInstanceID = i + 1;
        on.order = i;
        on.isNoteOn = true;
        ticks->push_back(on);
    }
    for (int i = 0; i < noteCount; ++i)
    {
        MIDIEventTick off{};
        off.type = MIDIEventType::Note;
        off.tick = (std::max)(1, ticksPerQuarter);
        off.noteNumber = std::clamp(notes[i], 0, 127);
        off.velocity = 0;
        off.channel = std::clamp(channel, 0, 15);
        off.controller = 0;
        off.value = 0;
        off.noteInstanceID = i + 1;
        off.order = noteCount + i;
        off.isNoteOn = false;
        ticks->push_back(off);
    }
    return ticks;
}
```

**ポイント:**
- tick=0 に noteCount 個のノートオンを `order=0,1,...` で積む（同 tick の順序付け）
- tick=ticksPerQuarter に全ノートオフを `order=noteCount,noteCount+1,...` で積む
- `noteInstanceID` を 1-indexed で割り当てる（0 は "None" 扱いの慣例に合わせる）

### 2-2. `StartGUISoundTonePreview` の tick 生成部分に分岐を追加

**変更位置:** `StartGUISoundTonePreview` 内の `cfg.overrideNoteTicks = BuildOverrideNoteTicksForSoundTone(...)` 行（L224）を以下に置き換える。

```cpp
    // コードモードが有効な場合は複数ノートを同時発音
    if (state.chordModeEnabled)
    {
        constexpr int kChordOffsets[5][4] = {
            { 0,  4,  7, -1 }, // Major
            { 0,  3,  7, -1 }, // Minor
            { 0,  4,  7, 10 }, // 7th
            { 0,  3,  7, 10 }, // Minor7th
            { 0,  5,  7, -1 }, // Sus4
        };
        constexpr int kChordSizes[5] = { 3, 3, 4, 4, 3 };
        const int ct = std::clamp(state.chordType, 0, 4);
        const int sz = kChordSizes[ct];
        std::array<int, 4> notes{};
        for (int i = 0; i < sz; ++i)
        {
            notes[i] = previewNote + kChordOffsets[ct][i];
        }
        cfg.overrideNoteTicks = BuildOverrideNoteTicksForChord(
            previewChannel, notes, sz, 110, cfg.overrideTicksPerQuarter);
    }
    else
    {
        cfg.overrideNoteTicks = BuildOverrideNoteTicksForSoundTone(
            previewChannel, previewNote, 110, cfg.overrideTicksPerQuarter);
    }
```

**ポイント:**
- `-1` オフセットは未使用スロット（sz で使用数を制限するため notes[] に積まれない）
- `previewNote` は `ResolveSoundTonePreviewNote` で解決済みのルート音
- Auto Tone Preview のデバウンス経路も同じ `StartGUISoundTonePreview` を呼ぶため、
  Chord モード ON 時は自動プレビューでも和音が鳴る（意図的）

---

## T3: `src/gui/main/VirtualKeyboard.inl` に Chord UI を追加

### 変更の概要

1. `ImGui::Separator()` の直後に Chord チェックボックス + コード種別コンボを挿入
2. キー描画時に構成音鍵をルート（青）と区別して緑でハイライト
3. 色定数 `cChord` を追加

### コード全体（差分ではなく最終形）

```cpp
// VirtualKeyboard.inl
// 仮想キーボード: C2(36)〜C6(84) の鍵盤を DrawList で描画し、
// クリック時に tonePreviewNoteNumber を更新して即時 Tone Preview を発火する。
// DrawVirtualKeyboard(state) を MainWindow.inl の Sound タブ内から呼び出す。
//
// 前提: GUIMain.cpp の using gui::StartGUISoundTonePreview; が参照可能であること。

static void DrawVirtualKeyboard(GUIState& state)
{
    // DrumKit の場合は非表示（既存 Preview Note スライダーと同じ条件）
    if (state.channelConfigs)
    {
        const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
        if (std::holds_alternative<DrumKitConfig>((*state.channelConfigs)[slot].source))
        {
            return;
        }
    }

    constexpr int kFirstNote = 36;    // C2
    constexpr int kLastNote  = 84;    // C6 （49鍵：4オクターブ + C）
    constexpr float kWhiteW  = 14.0f; // 白鍵の幅
    constexpr float kWhiteH  = 48.0f; // 白鍵の高さ
    constexpr float kBlackW  = 9.0f;  // 黒鍵の幅
    constexpr float kBlackH  = 30.0f; // 黒鍵の高さ

    // semitone % 12 が白鍵か
    auto isWhite = [](int n) -> bool
    {
        const int s = n % 12;
        return s == 0 || s == 2 || s == 4 || s == 5 || s == 7 || s == 9 || s == 11;
    };

    // note より前（kFirstNote 以降）の白鍵数を返す。
    auto whitesBefore = [&](int note) -> int
    {
        int c = 0;
        for (int n = kFirstNote; n < note; ++n)
        {
            if (isWhite(n)) { ++c; }
        }
        return c;
    };

    int totalWhite = 0;
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (isWhite(n)) { ++totalWhite; }
    }

    // ---- Chord UI ----
    ImGui::Separator();
    const char* chordTypes[] = { "Major", "Minor", "7th", "Minor 7th", "Sus4" };
    ImGui::Checkbox("Chord", &state.chordModeEnabled);
    if (state.chordModeEnabled)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::Combo("##chordType", &state.chordType, chordTypes, IM_ARRAYSIZE(chordTypes));
    }

    // ---- コード構成音 ハイライト判定 ----
    constexpr int kChordOffsets[5][4] = {
        { 0,  4,  7, -1 }, // Major
        { 0,  3,  7, -1 }, // Minor
        { 0,  4,  7, 10 }, // 7th
        { 0,  3,  7, 10 }, // Minor7th
        { 0,  5,  7, -1 }, // Sus4
    };
    constexpr int kChordSizes[5] = { 3, 3, 4, 4, 3 };

    auto isChordMember = [&](int n) -> bool
    {
        if (!state.chordModeEnabled || n == state.tonePreviewNoteNumber) { return false; }
        const int ct = std::clamp(state.chordType, 0, 4);
        for (int i = 0; i < kChordSizes[ct]; ++i)
        {
            if (kChordOffsets[ct][i] >= 0 &&
                n == state.tonePreviewNoteNumber + kChordOffsets[ct][i])
            {
                return true;
            }
        }
        return false;
    };

    // ---- 描画 ----
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float totalW = static_cast<float>(totalWhite) * kWhiteW;

    // 鍵盤全体を覆う InvisibleButton でクリック受付
    ImGui::InvisibleButton("##vkb", ImVec2(totalW, kWhiteH));
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 mpos = ImGui::GetMousePos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 cWhite  = IM_COL32(235, 235, 235, 255);
    const ImU32 cBlack  = IM_COL32( 30,  30,  30, 255);
    const ImU32 cSel    = IM_COL32(100, 170, 255, 255); // ルート（青）
    const ImU32 cChord  = IM_COL32(100, 200, 140, 255); // コード構成音（緑）
    const ImU32 cBorder = IM_COL32( 80,  80,  80, 255);

    // 白鍵を先に描画
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (!isWhite(n)) { continue; }
        const float x0 = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
        const float x1 = x0 + kWhiteW - 1.0f;
        const float y1 = origin.y + kWhiteH;
        const ImU32 fill = (state.tonePreviewNoteNumber == n) ? cSel
                         : isChordMember(n)                   ? cChord
                         : cWhite;
        dl->AddRectFilled({ x0, origin.y }, { x1, y1 }, fill);
        dl->AddRect      ({ x0, origin.y }, { x1, y1 }, cBorder);
    }

    // 黒鍵を白鍵の上に重ねて描画
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (isWhite(n)) { continue; }
        const float cx = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
        const float x0 = cx - kBlackW * 0.5f;
        const float x1 = x0 + kBlackW;
        const float y1 = origin.y + kBlackH;
        const ImU32 fill = (state.tonePreviewNoteNumber == n) ? cSel
                         : isChordMember(n)                   ? cChord
                         : cBlack;
        dl->AddRectFilled({ x0, origin.y }, { x1, y1 }, fill);
        dl->AddRect      ({ x0, origin.y }, { x1, y1 }, cBorder);
    }

    // クリック判定: 黒鍵優先でヒットテスト
    if (clicked)
    {
        int hit = -1;

        for (int n = kFirstNote; n <= kLastNote && hit < 0; ++n)
        {
            if (isWhite(n)) { continue; }
            const float cx = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
            const float x0 = cx - kBlackW * 0.5f;
            if (mpos.x >= x0 && mpos.x < x0 + kBlackW &&
                mpos.y >= origin.y && mpos.y < origin.y + kBlackH)
            {
                hit = n;
            }
        }

        if (hit < 0)
        {
            for (int n = kFirstNote; n <= kLastNote && hit < 0; ++n)
            {
                if (!isWhite(n)) { continue; }
                const float x0 = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
                if (mpos.x >= x0 && mpos.x < x0 + kWhiteW &&
                    mpos.y >= origin.y && mpos.y < origin.y + kWhiteH)
                {
                    hit = n;
                }
            }
        }

        if (hit >= 0)
        {
            state.tonePreviewNoteNumber = hit;
            StartGUISoundTonePreview(state); // chord モード時は内部で和音 tick を生成
        }
    }
}
```

**変更サマリー（元コードとの差分）:**
- `ImGui::Separator()` 後に Chord UI 2 行（チェックボックス + コンボ）を挿入
- `cChord` 色定数を追加
- `isChordMember` ラムダを追加
- 白鍵・黒鍵の `fill` 計算を 3 択（ルート/構成音/通常）に変更
- クリックハンドラは変更なし（`StartGUISoundTonePreview` 内で chord を処理するため）

---

## 統合チェックリスト

1. - [ ] `GUIState` に `chordModeEnabled` (bool) と `chordType` (int) が追加されている
2. - [ ] `BuildOverrideNoteTicksForChord` が tick=0 に N 個のノートオンを積み、tick=ticksPerQuarter に N 個のノートオフを積む
3. - [ ] Chord OFF 時: `StartGUISoundTonePreview` が従来通り単音 tick を使用する
4. - [ ] Chord ON / Major: ルート + 長3度 + 完全5度（3音）が同時発音される
5. - [ ] Chord ON / 7th: ルート + 長3度 + 完全5度 + 短7度（4音）が同時発音される
6. - [ ] Chord ON / Sus4: ルート + 完全4度 + 完全5度（3音）が同時発音される
7. - [ ] キーボード上でルート鍵が青、構成音鍵が緑でハイライトされる
8. - [ ] コード種別コンボは Chord チェックが ON の時のみ表示される
9. - [ ] DrumKit チャンネル選択時は Chord UI ごと非表示になる
10. - [ ] ルート音が高い鍵（例: ノート 120）でも構成音が 127 を超えないよう clamp されている
11. - [ ] Auto Tone Preview（デバウンス経由）でも Chord モードが有効なら和音が鳴る
12. - [ ] コード種別を変更してから鍵を押すと新しい種別の和音が鳴る

---

## 実装上の注意

### kChordOffsets の `-1` について

`kChordOffsets` は 3 音コードにも 4 要素配列を使い、未使用スロットを `-1` で埋める。
`BuildOverrideNoteTicksForChord` には `kChordSizes[ct]` を渡すため `-1` スロットは notes[] に積まれない。
`VirtualKeyboard.inl` の `isChordMember` でも `kChordOffsets[ct][i] >= 0` のガードを必ず入れること。

### `kChordOffsets` の重複定義

同じ定数が `GUIPreviewActions.cpp` と `VirtualKeyboard.inl` の 2 箇所に定義される。
共有ヘッダー化はファイル増加になるため、現状は重複のまま管理する
（2 箇所が乖離しても UI ハイライトと発音結果が食い違うだけで機能は壊れない）。

### `order` フィールドの意味

`MIDIEventTick::order` は同一 tick 内でのイベント処理順序を決めるフィールド。
和音の場合は全 note-on が tick=0 に集まるため、`order=0,1,...` で確定的な順序を与える。
note-off は全て同一 tick だが voice 解放順序に影響するため `noteCount + i` で note-on と区別する。

### `whitesBefore` の計算コスト

`whitesBefore` は O(N) ループで毎フレーム 49 鍵 × 49 鍵 = 約 2400 回呼ばれる。
`isChordMember` は最大 4 回の比較で済むため追加負荷は無視できる。
キーボード全体の計算コストは既存コードの時点から変わらない。
