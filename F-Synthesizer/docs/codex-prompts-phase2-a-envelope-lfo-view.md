# Codex Prompts: ADSR / LFO グラフィカル表示 (Phase 2-A)

## 背景・共通コンテキスト

F-Synthesizer は C++/ImGui ベースのシンセサイザー GUI。
Layer3 チャンネルエディタの Envelope セクションと Modulation セクション（Env2 / LFO1）に
リアルタイム更新されるカーブ/波形プレビューを追加する。

### 画面上の位置

```
┌─ Layer3: Channel Editor ─────────────────────────────────────┐
│ ▼ Envelope / Gain                                             │
│   Attack ──────── 0.010                                       │
│   Decay  ──────── 0.100    ┌──────────────────┐               │
│   Sustain ─────── 1.000    │ ADSR プレビュー  │ ← 追加       │
│   Release ─────── 0.100    └──────────────────┘               │
├───────────────────────────────────────────────────────────────┤
│ ▼ Source Details > Modulation                                 │
│   LFO1 Wave  [Sine ▼]                                         │
│   LFO1 Rate  ──────── 5.00  ┌──────────────────┐              │
│   ...                       │ LFO 波形プレビュー│ ← 追加      │
│   LFO1 Fade  ──────── 0.0   └──────────────────┘              │
│   Env2 Attack ─────── 0.01                                    │
│   ...                       ┌──────────────────┐              │
│   Env2 Curve ──────── 0.00  │ Env2 カーブ表示  │ ← 追加      │
│                             └──────────────────┘              │
└───────────────────────────────────────────────────────────────┘
```

### 関連する既存コード

| シンボル | 場所 | 役割 |
|---|---|---|
| `ChannelConfig::attackSec/decaySec/sustainLevel/releaseSec` | `SynthEngine.h` | メイン ADSR（カーブなし） |
| `ModEnvelopeConfig` | `Modulation.h` | Env2: attackSec/decaySec/sustainLevel/releaseSec/curve |
| `LfoConfig` | `Modulation.h` | LFO1: wave/rateHz/depth/bipolar/keySync/delayMs/fadeMs |
| `LfoWave` enum | `Modulation.h` | Sine/Triangle/Square/Saw/SampleAndHold の 5 種 |
| `drawModulationEditor` lambda | `ChannelEditorModulation.inl` L1 | Env2 + LFO1 スライダー群。`#include` は `GUIChannelEditor.cpp` L286 |
| `sliderWaveParam` lambda | `GUIChannelEditor.cpp` L276 | float スライダーヘルパー |
| "Envelope / Gain" section | `GUIChannelEditor.cpp` L294-306 | chCfg の ADSR スライダー群 |
| `ImGui::PlotLines` | imgui.h | ライン描画。外部ライブラリ追加不要 |

### `.inl` のインクルード構造（重要）

```
GUIChannelEditor.cpp
  namespace {}  ← 匿名名前空間（L14-L226）
    BuildGuiSourceKindList()
    ApplyFmTemplateByAlgorithm()
    DrawDrumConfigEditor()
    ← ここに EnvelopeView.inl を追加 ←
  } // namespace

  namespace gui {
    DrawChannelEditor() {
      sliderWaveParam lambda (L276)
      #include "channeleditor/ChannelEditorModulation.inl"  (L286)
        → drawModulationEditor lambda ← LFO/Env2 プレビュー呼び出しを追加
      "Envelope / Gain" section (L294) ← メイン ADSR プレビュー呼び出しを追加
      #include "channeleditor/ChannelEditorCommon.inl"  (L354)
      #include "channeleditor/ChannelEditorWaveform.inl"
      ...
    }
  }
```

### 設計方針

- **リアルタイム更新:** プレビューは毎フレーム計算・描画。キャッシュ不要（kN=128 の演算は軽量）
- **`ImGui::PlotLines` で統一:** スペクトラムビューアと同方式。外部ライブラリ追加なし
- **新規ファイル 1 本のみ:** `EnvelopeView.inl` を匿名名前空間に追加。既存 .inl への変更は最小
- **S&H 波形は固定パターン表示:** 実際のランダム値でなく代表的なステップ形状をハードコード
- **スタック上の配列を使用:** `static` 変数より明快。`float buf[128]` = 512B でスタック圧迫なし
- **カーブ適用:** Env2 プレビューの attack/decay/release に `curve` を反映（`pow(alpha, exp(curve))`）

---

## T1: EnvelopeView.inl（新規ファイル）

### 新規ファイル: `src/gui/channeleditor/EnvelopeView.inl`

```cpp
// EnvelopeView.inl
// ADSR カーブと LFO 波形のインラインプレビューを描画するヘルパー関数群。
// GUIChannelEditor.cpp の匿名名前空間に #include して使用する。
//
// DrawADSRPreview  : メイン Envelope / Env2 どちらにも使用（curve=0.0f で線形）
// DrawLfo1WavePreview: LFO1 の全 LfoWave 列挙値に対応

#include <cmath>

static void DrawADSRPreview(
    const char* id,
    float attackSec,
    float decaySec,
    float sustainLevel,
    float releaseSec,
    float curve = 0.0f)
{
    constexpr int kN = 128;

    // curve=0: exponent=1 (linear), curve=1: exponent≈2.72 (gradual/convex)
    // 「低いと急激、高いとなだらか」に合わせ attack に凸カーブを使う
    const float exp_ = std::exp(curve); // 1.0 .. e (約 2.72)

    // Sustain ホールド区間: 全体に対して一定比率で確保
    const float holdSec = (attackSec + decaySec + releaseSec) * 0.3f + 0.01f;
    const float totalSec = attackSec + decaySec + holdSec + releaseSec + 0.001f;
    const float dt = totalSec / static_cast<float>(kN - 1);

    float buf[kN];
    for (int i = 0; i < kN; ++i)
    {
        const float t = i * dt;
        float level;
        if (t < attackSec)
        {
            const float alpha = attackSec > 0.0f ? t / attackSec : 1.0f;
            level = std::pow(alpha, 1.0f / exp_); // convex: curve 高いほどなだらか
        }
        else if (t < attackSec + decaySec)
        {
            const float alpha = decaySec > 0.0f ? (t - attackSec) / decaySec : 1.0f;
            level = 1.0f - (1.0f - sustainLevel) * std::pow(alpha, exp_);
        }
        else if (t < attackSec + decaySec + holdSec)
        {
            level = sustainLevel;
        }
        else
        {
            const float elapsed = t - (attackSec + decaySec + holdSec);
            const float alpha = releaseSec > 0.0f
                ? std::min(elapsed / releaseSec, 1.0f)
                : 1.0f;
            level = sustainLevel * (1.0f - std::pow(alpha, exp_));
        }
        buf[i] = level;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.35f, 0.80f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
    ImGui::PlotLines(id, buf, kN, 0, nullptr, 0.0f, 1.0f, ImVec2(-1.0f, 44.0f));
    ImGui::PopStyleColor(2);
}

static void DrawLfo1WavePreview(const char* id, LfoWave wave)
{
    constexpr int kN = 128;
    constexpr int kCycles = 3;

    // S&H 表示用固定ステップパターン（代表的なランダムホールド形状）
    constexpr float kSahSteps[8] = { 0.70f, -0.30f, 1.00f, -0.80f,
                                      0.20f, -0.65f,  0.90f, -0.10f };

    float buf[kN];
    for (int i = 0; i < kN; ++i)
    {
        const float t = static_cast<float>(i) / kN * kCycles;
        const float phase = t - std::floor(t); // 0..1 within one cycle
        float v;
        switch (wave)
        {
        case LfoWave::Sine:
            v = std::sin(phase * 6.28318f);
            break;
        case LfoWave::Triangle:
            v = (phase < 0.25f) ? (4.0f * phase)
              : (phase < 0.75f) ? (2.0f - 4.0f * phase)
              :                   (4.0f * phase - 4.0f);
            break;
        case LfoWave::Square:
            v = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case LfoWave::Saw:
            v = 2.0f * phase - 1.0f;
            break;
        case LfoWave::SampleAndHold:
        {
            const int step = static_cast<int>(phase * 8.0f) % 8;
            v = kSahSteps[step];
        }
            break;
        default:
            v = 0.0f;
            break;
        }
        buf[i] = v;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.40f, 0.70f, 1.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
    ImGui::PlotLines(id, buf, kN, 0, nullptr, -1.2f, 1.2f, ImVec2(-1.0f, 40.0f));
    ImGui::PopStyleColor(2);
}
```

---

## T2: GUIChannelEditor.cpp への変更

### 変更ファイル: `src/gui/GUIChannelEditor.cpp`

#### 変更点 1: `<cmath>` include の追加（未収録の場合のみ）

ファイル先頭の `#include` 群に `<cmath>` が含まれていなければ追加:

```cpp
#include <cmath>
```

#### 変更点 2: 匿名名前空間内への EnvelopeView.inl インクルード

`DrawDrumConfigEditor` 関数の直後（`} // namespace` の直前）に追加:

```cpp
#include "channeleditor/EnvelopeView.inl"
} // namespace
```

**挿入位置の特定方法:**

`GUIChannelEditor.cpp` 内で以下のパターンを探す:

```cpp
    d.noiseType = noiseType;
    }
    return changed;
}
} // namespace
```

この `return changed; }` の直後（`} // namespace` の直前）に追加する:

```cpp
    d.noiseType = noiseType;
    }
    return changed;
}

#include "channeleditor/EnvelopeView.inl"
} // namespace
```

#### 変更点 3: "Envelope / Gain" セクションへの ADSR プレビュー追加

`if (ImGui::CollapsingHeader("Envelope / Gain", ...))` ブロック内の
`Release` スライダーの直後に追加する:

変更前:
```cpp
        changed |= ImGui::InputDouble("Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Release を調整します。", "ノートオフ後の余韻時間が変わります。", nullptr);
    }
```

変更後:
```cpp
        changed |= ImGui::InputDouble("Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Release を調整します。", "ノートオフ後の余韻時間が変わります。", nullptr);
        ImGui::TextDisabled("Envelope");
        DrawADSRPreview("##adsr_main",
            static_cast<float>(chCfg.attackSec),
            static_cast<float>(chCfg.decaySec),
            static_cast<float>(chCfg.sustainLevel),
            static_cast<float>(chCfg.releaseSec));
    }
```

---

## T3: ChannelEditorModulation.inl への変更

### 変更ファイル: `src/gui/channeleditor/ChannelEditorModulation.inl`

#### 変更点 1: LFO1 Fade スライダーの後に LFO 波形プレビューを追加

変更前:
```cpp
        localChanged |= sliderWaveParam("LFO1 Fade (ms)", modulation.lfo1.fadeMs, 0.0f, 2000.0f, "%.1f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Fade を調整します。", "LFOが最大深さに達するまでの立ち上がり時間が変わります。", nullptr);

        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Attack", modulation.env2.attackSec, 0.0f, 10.0f, "%.3f");
```

変更後:
```cpp
        localChanged |= sliderWaveParam("LFO1 Fade (ms)", modulation.lfo1.fadeMs, 0.0f, 2000.0f, "%.1f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Fade を調整します。", "LFOが最大深さに達するまでの立ち上がり時間が変わります。", nullptr);
        ImGui::TextDisabled("LFO1 Wave");
        DrawLfo1WavePreview(
            (std::string("##lfo1_preview_") + idPrefix).c_str(),
            modulation.lfo1.wave);

        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Attack", modulation.env2.attackSec, 0.0f, 10.0f, "%.3f");
```

#### 変更点 2: Env2 Curve スライダーの後に ADSR プレビューを追加

変更前:
```cpp
        localChanged |= sliderWaveParam("Env2 Curve", modulation.env2.curve, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Curve を調整します。", "変化の加速感が変わります。低いと急激に、高いとなだらかに変化します。", nullptr);

        const char* modSources[] = { "none", "lfo1", ...
```

変更後:
```cpp
        localChanged |= sliderWaveParam("Env2 Curve", modulation.env2.curve, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Curve を調整します。", "変化の加速感が変わります。低いと急激に、高いとなだらかに変化します。", nullptr);
        ImGui::TextDisabled("Env2 Envelope");
        DrawADSRPreview(
            (std::string("##env2_preview_") + idPrefix).c_str(),
            static_cast<float>(modulation.env2.attackSec),
            static_cast<float>(modulation.env2.decaySec),
            static_cast<float>(modulation.env2.sustainLevel),
            static_cast<float>(modulation.env2.releaseSec),
            static_cast<float>(modulation.env2.curve));

        const char* modSources[] = { "none", "lfo1", ...
```

---

## T4: 統合確認チェックリスト

実装完了後に以下を手動で確認する:

1. **ビルド:** `./scripts/check.ps1` がエラーなく通る
2. **Envelope / Gain 表示:** "Envelope / Gain" セクションを開くと Release スライダーの下に緑の ADSR カーブが表示される
3. **ADSR リアルタイム更新:** Attack/Decay/Sustain/Release スライダーを動かすとカーブが即座に変化する
4. **Sustain 形状:** Sustain が高いと上部に平坦区間が見え、Sustain=0 だと Release が x 軸に沿って消える
5. **LFO 波形プレビュー表示:** Layer3 Modulation セクションで "LFO1 Wave" ラベルの下に青い波形が表示される
6. **LFO 全 5 波形:** Sine/Triangle/Square/Saw/SampleAndHold すべてで波形が切り替わる
   - Sine: なめらかな曲線（3 周期）
   - Triangle: ノコギリ状の三角波
   - Square: 段差のある矩形波
   - Saw: 右上がりのノコギリ波
   - S&H: 不規則なステップ状
7. **Env2 プレビュー表示:** "Env2 Curve" スライダーの下に緑の Env2 カーブが表示される
8. **Env2 Curve 反映:** Curve=0 で急峻（attack が凸形）、Curve=1 でなだらかになる
9. **idPrefix 分離:** Waveform / Analog / FM それぞれで個別の `idPrefix` が渡されるため、
   複数の音源タイプを切り替えても `ImGui::PlotLines` の ID 衝突が起きない
10. **Music タブ無影響:** Music タブに切り替えると Channel Editor が非表示になりプレビューは描画されない

---

## 実装の注意点

### ID 衝突の回避

`drawModulationEditor` は `idPrefix` 引数を受け取る（例: `"waveform_modulation"`, `"fm_modulation"`）。
プレビューの ID 文字列にこれを含めることで、同一フレームに複数の音源が同時表示された場合の
`ImGui::PlotLines` ID 衝突を防ぐ。

### 匿名名前空間と DrawChannelEditor 内 lambda の可視性

`DrawADSRPreview` / `DrawLfo1WavePreview` は匿名名前空間に定義される static 関数。
`DrawChannelEditor` 関数スコープ内の `drawModulationEditor` lambda からも
（lambda は関数ローカルスコープで定義されるが）外部の名前空間スコープの関数は参照可能。

### `ChannelConfig` の ADSR は `double` 型

`chCfg.attackSec` 等は `double`。`DrawADSRPreview` は `float` 引数のため、
呼び出し側で `static_cast<float>` が必要（T2 変更点 3 に明記済み）。

### 既存 Envelope / Gain セクションは `ImGui::InputDouble` を使用

メイン ADSR は `SliderFloat` ではなく `InputDouble` で編集されている。
ADSR プレビューはその下に追加するだけでよく、既存のウィジェットは変更不要。

### `<cmath>` の多重インクルード

GUIChannelEditor.cpp に `<cmath>` が既に含まれている場合は追加不要。
`std::sin`, `std::floor`, `std::pow`, `std::exp`, `std::min` を使用する。
