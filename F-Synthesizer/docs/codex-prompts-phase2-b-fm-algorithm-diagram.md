# Codex Prompts: FM アルゴリズムのオペレーター接続図 (Phase 2-B)

## 背景・共通コンテキスト

F-Synthesizer は C++/ImGui ベースのシンセサイザー GUI。
Layer3 FM チャンネルエディタの "FM Algorithm" コンボの直下に、
Algorithm 0〜7 のオペレーター接続構造を DrawList でリアルタイム描画するブロック図を追加する。

### 画面上の位置

```
┌─ Layer3: Channel Editor > Source Details (FM) ─────────────────┐
│  FM Algorithm  [3: M->M->M->C ▼]  [テンプレートに戻す]         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ╔══════╗  →  ╔══════╗  →  ╔══════╗  →  ╔══════╗      │    │
│  │ ║ 1/M  ║     ║ 2/M  ║     ║ 3/M  ║     ║ 4/C  ║      │    │
│  │ ╚══════╝     ╚══════╝     ╚══════╝     ╚══════╝      │    │
│  └─────────────────────────────────────────────────────────┘    │
│  Feedback  ────────── 0.22                                       │
│  ▶ Op 1 (Mod)                                                    │
│  ...                                                             │
└──────────────────────────────────────────────────────────────────┘
```

### FM 各アルゴリズムの実際の接続（`RenderFm.inl` より）

| Algorithm | 接続構造 | キャリア |
|---|---|---|
| 0 | Op0(M+fb) → Op1(C) | Op1 |
| 1 | Op0→Op1, Op2→Op3 | Op1, Op3 |
| 2 | Op0 → {Op1, Op2, Op3} | Op1, Op2, Op3 |
| 3 | Op0→Op1→Op2→Op3 (直列) | Op3 |
| 4 | Op0→Op1, Op2→Op3 (algo1と同トポロジー) | Op1, Op3 |
| 5 | Op0 → {Op1, Op2, Op3} (algo2と同トポロジー) | Op1, Op2, Op3 |
| 6 | Op0→Op1, Op2(独立C), Op3(独立C) | Op1, Op2, Op3 |
| 7 | Op0, Op1, Op2, Op3 (全キャリア) | Op0, Op1, Op2, Op3 |

> **注意:** Algo 0 で RenderFm が Op2/Op3 を未使用（出力加算なし）のため、ダイアグラムにも描画しない。

### Feedback ループ

Op0 は `src.feedback * fs.op0FeedbackSample` を自己変調入力として受け取る（`RenderFm.inl` L27-29）。
`src.feedback` の値に関わらず構造として常に存在するため、**全 algorithm で Op0 上部に feedback ループを描画する**。

### 関連する既存コード

| シンボル | 場所 | 役割 |
|---|---|---|
| `DrawFmAlgorithmDiagram` 呼び出し位置 | `ChannelEditorFm.inl` L44直後 | algo combo + テンプレートボタンの直後、Feedback スライダーの直前 |
| `#include "channeleditor/ChannelEditorFm.inl"` | `GUIChannelEditor.cpp` L358 | ChannelEditorFm.inl のインクルード元 |
| 匿名名前空間（L14-L226） | `GUIChannelEditor.cpp` | Phase 2-A の EnvelopeView.inl がここに追加済み |
| `fm->algorithm` | `FmConfig::algorithm` (int) | 0〜7 の値 |
| `ImGui::GetWindowDrawList()` | imgui.h | DrawList 取得。外部ライブラリ追加不要 |
| `ImGui::InvisibleButton` | imgui.h | canvas 領域を ImGui レイアウトに登録 |

### `.inl` インクルード構造

```
GUIChannelEditor.cpp
  namespace {}                          // 匿名名前空間
    ...
    #include "channeleditor/EnvelopeView.inl"   // Phase 2-A で追加済み
    ← ここに FmAlgorithmDiagram.inl を追加 ←
  } // namespace

  namespace gui {
    DrawChannelEditor() {
      ...
      #include "channeleditor/ChannelEditorFm.inl"   // L358
        → DrawFmAlgorithmDiagram() 呼び出しを追加
    }
  }
```

### 設計方針

- **DrawList のみ:** `ImGui::DrawList` の `AddRectFilled/AddRect/AddLine/AddTriangleFilled/AddBezierCubic/AddText` を使用
- **InvisibleButton でキャンバス確保:** `GetItemRectMin()` でオリジンを取得
- **キャンバスサイズ:** 幅 = 利用可能幅（最小 200px）、高さ = 80px 固定
- **データ駆動レイアウト:** `switch(algorithm)` で各 algo の op 座標・矢印定義を設定
- **色分け:** キャリア = ゴールド、モジュレーター = ブルー、矢印 = グレー、フィードバック = オレンジ
- **ラベル:** `"1/C"`, `"2/M"` 形式で op 番号と役割を 1 行表示

---

## T1: FmAlgorithmDiagram.inl（新規ファイル）

### 新規ファイル: `src/gui/channeleditor/FmAlgorithmDiagram.inl`

```cpp
// FmAlgorithmDiagram.inl
// FM Algorithm 0〜7 のオペレーター接続図を DrawList で描画する。
// DrawFmAlgorithmDiagram(id, algorithm) を ChannelEditorFm.inl から呼び出す。
// GUIChannelEditor.cpp 匿名名前空間に #include して使用する。

#include <cmath>

static void DrawFmAlgorithmDiagram(const char* id, int algorithm)
{
    const float kCH   = 80.0f;  // canvas height
    const float kBW   = 36.0f;  // op box width
    const float kBH   = 22.0f;  // op box height
    const float kHalfW = kBW * 0.5f;
    const float kHalfH = kBH * 0.5f;

    const float cW = std::max(ImGui::GetContentRegionAvail().x, 200.0f);
    ImGui::InvisibleButton(id, ImVec2(cW, kCH));
    const ImVec2 org = ImGui::GetItemRectMin();
    ImDrawList* dl   = ImGui::GetWindowDrawList();

    // ---- Background ----
    dl->AddRectFilled(org, {org.x + cW, org.y + kCH}, IM_COL32(18, 18, 18, 215), 4.0f);
    dl->AddRect(org,       {org.x + cW, org.y + kCH}, IM_COL32(55, 55, 55, 180), 4.0f);

    // ---- Colors ----
    const ImU32 kColCarFill = IM_COL32(160, 120,  20, 230); // gold
    const ImU32 kColModFill = IM_COL32( 30,  65, 160, 230); // blue
    const ImU32 kColCarBd   = IM_COL32(255, 210,  60, 255);
    const ImU32 kColModBd   = IM_COL32( 90, 150, 255, 255);
    const ImU32 kColArrow   = IM_COL32(200, 200, 200, 220);
    const ImU32 kColFb      = IM_COL32(220, 145,  40, 210);

    // ---- Op position descriptor ----
    struct OpPos { float cx, cy; bool isCarrier; bool active; };

    // ---- Draw helpers ----
    auto drawOp = [&](const OpPos& op, int opIdx)
    {
        if (!op.active) { return; }
        const float x0 = org.x + op.cx - kHalfW;
        const float y0 = org.y + op.cy - kHalfH;
        const float x1 = x0 + kBW;
        const float y1 = y0 + kBH;
        dl->AddRectFilled({x0, y0}, {x1, y1}, op.isCarrier ? kColCarFill : kColModFill, 3.0f);
        dl->AddRect(      {x0, y0}, {x1, y1}, op.isCarrier ? kColCarBd   : kColModBd,   3.0f);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d/%s", opIdx + 1, op.isCarrier ? "C" : "M");
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const ImU32 textCol = op.isCarrier ? IM_COL32(255, 228, 100, 255)
                                           : IM_COL32(160, 200, 255, 255);
        dl->AddText({x0 + (kBW - ts.x) * 0.5f, y0 + (kBH - ts.y) * 0.5f}, textCol, buf);
    };

    auto drawArrow = [&](const OpPos& from, const OpPos& to)
    {
        const float x0 = org.x + from.cx + kHalfW;
        const float y0 = org.y + from.cy;
        const float x1 = org.x + to.cx   - kHalfW;
        const float y1 = org.y + to.cy;
        float dx = x1 - x0, dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.01f) { return; }
        const float ndx = dx / len, ndy = dy / len;
        const float al  = 7.0f, aw = 3.5f;
        const float bx  = x1 - ndx * al, by = y1 - ndy * al;
        dl->AddLine({x0, y0}, {bx, by}, kColArrow, 1.5f);
        dl->AddTriangleFilled({x1, y1},
            {bx - ndy * aw, by + ndx * aw},
            {bx + ndy * aw, by - ndx * aw},
            kColArrow);
    };

    // Feedback bezier arc above Op0 top edge
    auto drawFeedback = [&](const OpPos& op)
    {
        if (!op.active) { return; }
        const float cx = org.x + op.cx;
        const float ty = org.y + op.cy - kHalfH; // top of box
        const ImVec2 p0 = {cx - 10.0f, ty};
        const ImVec2 c1 = {cx - 26.0f, ty - 22.0f};
        const ImVec2 c2 = {cx + 26.0f, ty - 22.0f};
        const ImVec2 p3 = {cx + 10.0f, ty};
        dl->AddBezierCubic(p0, c1, c2, p3, kColFb, 1.5f);
        // Arrowhead at p3: end tangent direction = normalize(p3 - c2)
        const float tdx = p3.x - c2.x, tdy = p3.y - c2.y;
        const float tlen = std::sqrt(tdx * tdx + tdy * tdy);
        if (tlen > 0.01f)
        {
            const float ntdx = tdx / tlen, ntdy = tdy / tlen;
            const float al = 6.0f, aw = 3.0f;
            const float bx = p3.x - ntdx * al, by = p3.y - ntdy * al;
            dl->AddTriangleFilled(p3,
                {bx - ntdy * aw, by + ntdx * aw},
                {bx + ntdy * aw, by - ntdx * aw},
                kColFb);
        }
    };

    // ---- Per-algorithm layout ----
    // cx/cy: canvas-relative center coordinates of each op box
    // Vertical positions: topY≈18, midY≈40, botY≈62
    constexpr float kMidY = 40.0f;
    constexpr float kTopY = 18.0f;
    constexpr float kBotY = 62.0f;

    OpPos ops[4]        = {};
    int arrows[6][2]    = {};
    int arrowCount      = 0;

    switch (algorithm)
    {
    case 0: // Op0(M+fb) → Op1(C)  [Op2/Op3 unused]
        ops[0] = {65,  kMidY, false, true};
        ops[1] = {185, kMidY, true,  true};
        ops[2] = {0, 0, true, false};
        ops[3] = {0, 0, true, false};
        arrows[0][0] = 0; arrows[0][1] = 1; arrowCount = 1;
        break;

    case 1:
    case 4: // [Op0→Op1] + [Op2→Op3]
        ops[0] = {55,  kTopY, false, true};
        ops[1] = {160, kTopY, true,  true};
        ops[2] = {55,  kBotY, false, true};
        ops[3] = {160, kBotY, true,  true};
        arrows[0][0]=0; arrows[0][1]=1;
        arrows[1][0]=2; arrows[1][1]=3;
        arrowCount = 2;
        break;

    case 2:
    case 5: // Op0 → {Op1, Op2, Op3}
        ops[0] = {50,  kMidY, false, true};
        ops[1] = {168, kTopY, true,  true};
        ops[2] = {168, kMidY, true,  true};
        ops[3] = {168, kBotY, true,  true};
        arrows[0][0]=0; arrows[0][1]=1;
        arrows[1][0]=0; arrows[1][1]=2;
        arrows[2][0]=0; arrows[2][1]=3;
        arrowCount = 3;
        break;

    case 3: // Op0 → Op1 → Op2 → Op3
        ops[0] = {25,  kMidY, false, true};
        ops[1] = {85,  kMidY, false, true};
        ops[2] = {145, kMidY, false, true};
        ops[3] = {205, kMidY, true,  true};
        arrows[0][0]=0; arrows[0][1]=1;
        arrows[1][0]=1; arrows[1][1]=2;
        arrows[2][0]=2; arrows[2][1]=3;
        arrowCount = 3;
        break;

    case 6: // Op0→Op1, Op2(独立C), Op3(独立C)
        ops[0] = {28,  kMidY, false, true};
        ops[1] = {95,  kMidY, true,  true};
        ops[2] = {155, kMidY, true,  true};
        ops[3] = {215, kMidY, true,  true};
        arrows[0][0]=0; arrows[0][1]=1; arrowCount = 1;
        break;

    case 7: // 全キャリア
        ops[0] = {25,  kMidY, true, true};
        ops[1] = {90,  kMidY, true, true};
        ops[2] = {155, kMidY, true, true};
        ops[3] = {220, kMidY, true, true};
        arrowCount = 0;
        break;

    default:
        break;
    }

    // ---- Draw (arrows → feedback → op boxes) ----
    for (int a = 0; a < arrowCount; ++a)
    {
        drawArrow(ops[arrows[a][0]], ops[arrows[a][1]]);
    }
    drawFeedback(ops[0]);  // feedback on Op0 for all algorithms

    for (int i = 0; i < 4; ++i)
    {
        drawOp(ops[i], i);
    }

    // ---- Legend (bottom-left) ----
    constexpr float kLegY = 68.0f;
    dl->AddRectFilled({org.x + 5,  org.y + kLegY},
                      {org.x + 21, org.y + kLegY + 10}, kColCarFill, 2.0f);
    dl->AddText({org.x + 24, org.y + kLegY}, IM_COL32(255, 228, 100, 200), "Carrier");
    dl->AddRectFilled({org.x + 78, org.y + kLegY},
                      {org.x + 94, org.y + kLegY + 10}, kColModFill, 2.0f);
    dl->AddText({org.x + 97, org.y + kLegY}, IM_COL32(160, 200, 255, 200), "Modulator");

    // Feedback legend
    dl->AddLine({org.x + 170, org.y + kLegY + 5}, {org.x + 185, org.y + kLegY + 5}, kColFb, 1.5f);
    dl->AddText({org.x + 188, org.y + kLegY}, IM_COL32(220, 145, 40, 200), "FB");
}
```

---

## T2: GUIChannelEditor.cpp への変更

### 変更ファイル: `src/gui/GUIChannelEditor.cpp`

#### 変更点: 匿名名前空間内への FmAlgorithmDiagram.inl インクルード

Phase 2-A で追加した `EnvelopeView.inl` インクルードの直後に追加する:

変更前:
```cpp
#include "channeleditor/EnvelopeView.inl"
} // namespace
```

変更後:
```cpp
#include "channeleditor/EnvelopeView.inl"
#include "channeleditor/FmAlgorithmDiagram.inl"
} // namespace
```

> Phase 2-A 未実装の場合は `DrawDrumConfigEditor` の直後（`} // namespace` の直前）に単独で追加する。

---

## T3: ChannelEditorFm.inl への変更

### 変更ファイル: `src/gui/channeleditor/ChannelEditorFm.inl`

#### 変更点: Algorithm コンボ + テンプレートボタンの直後にダイアグラム呼び出しを追加

変更前:
```cpp
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "現在アルゴリズムの推奨テンプレートへ戻します。",
                    "FMオペレータ/フィルタ/変調の初期値を安全域へ復帰します。",
                    "現在の微調整値は上書きされます。");
            }

            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Feedback", fm->feedback, 0.0f, 1.0f, "%.2f");
```

変更後:
```cpp
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "現在アルゴリズムの推奨テンプレートへ戻します。",
                    "FMオペレータ/フィルタ/変調の初期値を安全域へ復帰します。",
                    "現在の微調整値は上書きされます。");
            }

            DrawFmAlgorithmDiagram("##fmAlgoDiagram", fm->algorithm);
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "FM オペレーター接続図を表示します。",
                    "青=モジュレーター (M)、金=キャリア (C)。矢印は変調の流れを示します。",
                    nullptr);
            }

            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Feedback", fm->feedback, 0.0f, 1.0f, "%.2f");
```

---

## T4: 統合確認チェックリスト

実装完了後に以下を手動で確認する:

1. **ビルド:** `./scripts/check.ps1` がエラーなく通る
2. **表示:** FM ソースのチャンネルを選択すると "FM Algorithm" コンボの直下に 80px 高のキャンバスが表示される
3. **Algo 0 確認:** 2 つのボックス（`1/M` → `2/C`）と feedback ループが表示される（Op3/Op4 は非表示）
4. **Algo 1 確認:** 2 段 2 列の配置（`1/M`→`2/C` と `3/M`→`4/C`）が上下に並ぶ
5. **Algo 2 確認:** 左に `1/M`、右に `2/C`, `3/C`, `4/C` が縦並びで、3 本の矢印が扇状に広がる
6. **Algo 3 確認:** `1/M`→`2/M`→`3/M`→`4/C` の直列チェーンが横一列に並ぶ
7. **Algo 4 確認:** Algo 1 と同一トポロジーの 2 段 2 列レイアウト
8. **Algo 5 確認:** Algo 2 と同一トポロジーの扇状レイアウト
9. **Algo 6 確認:** `1/M`→`2/C` + `3/C`（矢印なし） + `4/C`（矢印なし）の横一列
10. **Algo 7 確認:** `1/C` `2/C` `3/C` `4/C` が横一列、矢印なし
11. **即時切替:** "FM Algorithm" コンボを変更するとダイアグラムが即座に切り替わる
12. **Feedback ループ:** 全 algo で Op0 上部にオレンジのベジェ弧が表示される
13. **ホバー文言:** ダイアグラムにカーソルを合わせるとヘルプ文言が表示される
14. **Waveform/Analog/Noise タブ無影響:** FM 以外のソースタイプでは表示されない（`ChannelEditorFm.inl` 内の `else if` 内にあるため）

---

## 実装の注意点

### Op2/Op3 が Algo 0 で非表示になる理由

`RenderFm.inl` の case 0 は Op0 と Op1 の出力のみを `frame.sample` に加算する。
Op2/Op3 は `FmConfig` 上は存在するが信号経路に含まれない。
ダイアグラムで `ops[2].active = false; ops[3].active = false;` と設定することで描画をスキップし、
実際の音響動作と一致した表示にする。

### Algo 1 と Algo 4 のトポロジーが同一である理由

`RenderFm.inl` を確認すると case 1 と case 4 は同じ接続コード構造を持つ（どちらも `Op0→Op1`, `Op2→Op3` の 2 ペア）。
相違点はデフォルトテンプレート値のみ。よって `switch (algorithm)` で `case 1: case 4:` を fallthrough させてよい。
同様に case 2 と case 5 も同トポロジー。

### `AddBezierCubic` のエンドタンジェント計算

Cubic Bezier のエンドポイント P3 における接線方向は `P3 - C2`（最後の制御点との差）に比例する。
`drawFeedback` ではこの値を正規化してアローヘッドの向きを算出しているため、
制御点 `c2 = {cx + 26, ty - 22}` と終点 `p3 = {cx + 10, ty}` の差 = `(-16, 22)` が使われる。

### `InvisibleButton` のクリック判定

`InvisibleButton` は ImGui のヒットテスト/フォーカス対象になる。
ダイアグラムはインタラクティブ操作なし（クリックに反応しない）ため `IsItemClicked` 等は使わないが、
ホバー文言取得のために `if (ImGui::IsItemHovered())` チェックは `updateHoverHelp` 経由で機能する。

### キャンバス幅が 200px 未満の場合

`std::max(ImGui::GetContentRegionAvail().x, 200.0f)` でクランプしているため、
右カラムが極端に狭い場合は op ボックスが canvas 外にはみ出す可能性がある。
現行デフォルトウィンドウ（1280px）では右カラム ≈ 380px となり問題ない。
Algo 3 と Algo 6/7 の最右 op は cx ≈ 220px のため 260px 以上あれば全て収まる。
