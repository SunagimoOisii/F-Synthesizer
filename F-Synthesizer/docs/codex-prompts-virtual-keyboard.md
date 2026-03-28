# Codex Prompts: Virtual Keyboard (Phase 1-A)

## 背景・共通コンテキスト

F-Synthesizer は C++/ImGui ベースのシンセサイザー GUI。
Sound タブ（`UIModeTab == 0`）の2カラムテーブル末尾に仮想キーボードを追加する。

### 画面上の位置

```
┌─ Sound タブ ──────────────────────────────────────────────────┐
│ [左カラム]             │ [右カラム]                             │
│  チャンネルリスト       │  Layer1: プリセット発見                 │
│  波形ビューア           │  Layer2: マクロスライダー               │
│  スペクトラムアナライザ  │  Layer3: チャンネルエディタ              │
├─────────────────────────────────────────────────────────────── ┤
│         仮想キーボード C2〜C6 (← 今回の追加)                   │
└───────────────────────────────────────────────────────────────┘
```

テーブルが `ImGui::EndTable()` で閉じた直後（`if (UIModeTab == 0)` ブロック末尾）に配置する。
これにより左右両カラムをまたぐ全幅レイアウトになる。

### 関連する既存コード

| シンボル | 場所 | 役割 |
|---|---|---|
| `state.tonePreviewNoteNumber` | `GUIState` | Preview ノート番号（0〜127）。既存の Preview Note スライダーと共有 |
| `StartGUISoundTonePreview(state)` | `GUIPreviewActions.cpp` / `GUIMain.cpp` の using | 即時 Tone Preview を起動。GUIMain.cpp の using 宣言により .inl 内で直接呼び出し可 |
| `ResolveSoundTonePreviewNote(state, slot)` | `GUIPreviewActions.cpp` | `tonePreviewNoteNumber` を参照してノートを解決 |
| DrumKit 非表示ロジック | `MainWindow.inl` L684〜710 | `std::holds_alternative<DrumKitConfig>` で判定 |
| `kAutoTonePreviewDebounceSec` | `MainWindow.inl` L48 | 0.4秒デバウンス定数 |

### 設計方針

- **即時再生:** キークリック時は `StartGUISoundTonePreview(state)` を直接呼ぶ（デバウンスなし）
  - Auto Tone Preview の ON/OFF に関わらず鍵盤クリックは常に即時再生
- **DrumKit 時は非表示:** 既存の Preview Note スライダーと同じ条件
- **選択中ノートをハイライト:** `state.tonePreviewNoteNumber` と一致する鍵盤を青色表示
- **黒鍵クリック優先:** 白鍵と黒鍵が重なる領域では黒鍵を優先してヒットテスト
- **既存 Preview Note スライダーは削除しない:** 数値表示として共存させる

---

## T1: VirtualKeyboard.inl

### 新規ファイル: `src/gui/main/VirtualKeyboard.inl`

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
            return;
    }

    constexpr int   kFirstNote = 36;    // C2
    constexpr int   kLastNote  = 84;    // C6 （49鍵：4オクターブ + C）
    constexpr float kWhiteW    = 14.0f; // 白鍵の幅
    constexpr float kWhiteH    = 48.0f; // 白鍵の高さ
    constexpr float kBlackW    =  9.0f; // 黒鍵の幅
    constexpr float kBlackH    = 30.0f; // 黒鍵の高さ

    // semitone % 12 が白鍵か
    // 白鍵: C=0, D=2, E=4, F=5, G=7, A=9, B=11
    auto isWhite = [](int n) -> bool
    {
        const int s = n % 12;
        return s==0 || s==2 || s==4 || s==5 || s==7 || s==9 || s==11;
    };

    // note より前（kFirstNote 以降）の白鍵数を返す。
    // 黒鍵の x 座標 = origin + whitesBefore(n) * kWhiteW - kBlackW/2
    // （左隣の白鍵の右端を基準とした中央揃え）
    auto whitesBefore = [&](int note) -> int
    {
        int c = 0;
        for (int n = kFirstNote; n < note; ++n)
            if (isWhite(n)) ++c;
        return c;
    };

    // 白鍵の総数から全体幅を算出
    int totalWhite = 0;
    for (int n = kFirstNote; n <= kLastNote; ++n)
        if (isWhite(n)) ++totalWhite;

    ImGui::Separator();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float  totalW = static_cast<float>(totalWhite) * kWhiteW;

    // 鍵盤全体を覆う InvisibleButton でクリック受付
    ImGui::InvisibleButton("##vkb", ImVec2(totalW, kWhiteH));
    const bool    clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2  mpos    = ImGui::GetMousePos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 cWhite  = IM_COL32(235, 235, 235, 255);
    const ImU32 cBlack  = IM_COL32( 30,  30,  30, 255);
    const ImU32 cSel    = IM_COL32(100, 170, 255, 255); // 選択中ノートのハイライト色
    const ImU32 cBorder = IM_COL32( 80,  80,  80, 255);

    // 白鍵を先に描画
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (!isWhite(n)) continue;
        const float x0 = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
        const float x1 = x0 + kWhiteW - 1.0f; // 1px 隙間でセパレータ代わり
        const float y1 = origin.y + kWhiteH;
        dl->AddRectFilled({x0, origin.y}, {x1, y1},
            (state.tonePreviewNoteNumber == n) ? cSel : cWhite);
        dl->AddRect({x0, origin.y}, {x1, y1}, cBorder);
    }

    // 黒鍵を白鍵の上に重ねて描画
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (isWhite(n)) continue;
        // 黒鍵の中心 x = 左側白鍵の右端
        const float cx = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
        const float x0 = cx - kBlackW * 0.5f;
        const float x1 = x0 + kBlackW;
        const float y1 = origin.y + kBlackH;
        dl->AddRectFilled({x0, origin.y}, {x1, y1},
            (state.tonePreviewNoteNumber == n) ? cSel : cBlack);
        dl->AddRect({x0, origin.y}, {x1, y1}, cBorder);
    }

    // クリック判定: 黒鍵優先でヒットテスト
    if (clicked)
    {
        int hit = -1;

        // 黒鍵を先に調べる（白鍵と重なる領域で黒鍵を優先するため）
        for (int n = kFirstNote; n <= kLastNote && hit < 0; ++n)
        {
            if (isWhite(n)) continue;
            const float cx = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
            const float x0 = cx - kBlackW * 0.5f;
            if (mpos.x >= x0 && mpos.x < x0 + kBlackW &&
                mpos.y >= origin.y && mpos.y < origin.y + kBlackH)
            {
                hit = n;
            }
        }

        // 黒鍵にヒットしなければ白鍵を調べる
        if (hit < 0)
        {
            for (int n = kFirstNote; n <= kLastNote && hit < 0; ++n)
            {
                if (!isWhite(n)) continue;
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
            StartGUISoundTonePreview(state); // Auto Tone Preview のデバウンスを介さず即時再生
        }
    }
}
```

---

## T2: MainWindow.inl への統合

### 変更ファイル: `src/gui/main/MainWindow.inl`

#### 変更点 1: `#include` の追加

ファイル先頭の `#include` 群（他の `.inl` の include がある付近）に追加:

```cpp
#include "gui/main/VirtualKeyboard.inl"
```

#### 変更点 2: 描画呼び出しの追加

Sound タブ内の `ImGui::EndTable()` の直後、`if (UIModeTab == 0)` ブロックの閉じ括弧 `}` の直前に追加:

```cpp
// テーブル終了後、全幅で仮想キーボードを描画
DrawVirtualKeyboard(state);
```

**挿入位置の特定方法:**

`MainWindow.inl` 内で以下のパターンを探す:

```cpp
        ImGui::EndTable();
    }
}
else
{
    ImGui::TextUnformatted("Music");
```

この `ImGui::EndTable();` と `}` の間が挿入箇所:

```cpp
        ImGui::EndTable();
    }
    DrawVirtualKeyboard(state);  // ← ここに追加
}
else
{
    ImGui::TextUnformatted("Music");
```

---

## T3: 統合確認チェックリスト

実装完了後に以下を手動で確認する:

1. **ビルド:** `./scripts/check.ps1` がエラーなく通る
2. **表示:** Sound タブを開くと2カラムテーブルの下に鍵盤が表示される
3. **白鍵クリック:** 白鍵をクリックすると `tonePreviewNoteNumber` が更新され、即座に Tone Preview が再生される
4. **黒鍵クリック:** 黒鍵をクリックすると正しく半音上のノートが再生される（白鍵に引き込まれない）
5. **黒鍵/白鍵境界:** 黒鍵の下端より下（白鍵のみの領域）は白鍵が優先される
6. **ハイライト:** クリックしたノートが青色でハイライトされる
7. **Preview Note スライダーとの同期:** 鍵盤クリック後、既存の Preview Note スライダーも同じノート番号を表示している
8. **DrumKit 非表示:** DrumKit を持つスロットを選択すると鍵盤が非表示になる
9. **Auto Tone Preview との併存:** Auto Tone Preview ON/OFF に関わらず、鍵盤クリックで即時再生される
10. **Music タブ:** Music タブに切り替えると鍵盤が表示されない（`UIModeTab == 0` 内にあるため）

---

## 実装の注意点

### `whitesBefore` の計算コスト

`whitesBefore` はノート数分のループを持つため、外側のループと合わせて O(n²) になる。
ノート数は最大 49（C2〜C6）なので実用上の問題はないが、もし将来的に音域を広げる場合は
`static float xCache[128]` などに事前計算した値を保持すること。

### 鍵盤の幅とウィンドウ幅

`kWhiteW = 14.0f` × 29 白鍵 = 406px。
デフォルトウィンドウ幅 1280px の Sound タブ（左右マージン除く）で余裕を持って収まる。
UIスケール変更（`state.UIScaleIndex`）を考慮する場合は `kWhiteW` に `ImGui::GetFontSize() / 13.0f` 等を乗じるとよいが、現時点では固定値で十分。

### `StartGUISoundTonePreview` の参照

`GUIMain.cpp` に `using gui::StartGUISoundTonePreview;` が記述されているため、
`MainWindow.inl` 内（`DrawMainWindowFrame` 関数スコープ）から直接呼び出せる。
`VirtualKeyboard.inl` はこの関数内に `#include` されるため同様に参照可能。
