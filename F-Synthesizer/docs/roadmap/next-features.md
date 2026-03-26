# Next Features Roadmap

作成: 2026-03-26
状態: 実装作業用（実装完了後に破棄）
優先順位: Tier A → Tier B の順で着手する。

---

## 共通ルール（全タスク共通）

- 変更後は `./F-Synthesizer/scripts/check.ps1` を必ず実行する。
- 新規ファイルは作らない（既存ファイルの最小変更で完結させる）。
- 合成ロジック変更時は代表MIDI 1件でAB耳確認を行う。
- `docs/STATUS.md` の `Current` と `DECISIONS.md` を更新してタスクを閉じる。

---

## Tier A-1: Tone Preview スペクトラムビューア(完了)

### 目的

Sound タブの Tone Preview に FFT スペクトラム表示を追加し、
倍音構造（FM アルゴリズムの違い、フィルタの効き）を可視化する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `src/gui/main/MainWindow.inl` | 波形ビューアの直後にスペクトラム描画を追加 |

### 実装手順

1. `MainWindow.inl` の既存波形ビューアブロックを探す。
   - 目印: `ImGui::TextDisabled("Waveform (Tone Preview)");` と `ImGui::PlotLines("##wf", ...)` の2行。

2. `PlotLines` の直後に以下を追加する:

```cpp
// --- Spectrum ---
{
    constexpr int kFftSize = 1024;       // 解析点数（2の累乗）
    constexpr int kSpecBins = 128;       // 表示バー数
    static float specBuf[kSpecBins];

    // DFT（簡易版。kissFFT等の外部ライブラリ不要）
    // src は既存の波形データ参照（PlotLines と同じ変数）。
    // windowStart, total は PlotLines ブロックで計算済みのものを流用する。
    const int fftLen = std::min(kFftSize, total);
    // 実部バッファ（ハン窓 + 実DFT）
    static float re[kFftSize], im[kFftSize];
    for (int k = 0; k < fftLen; k++)
    {
        const double w = 0.5 * (1.0 - std::cos(2.0 * 3.14159265 * k / fftLen)); // Hann
        re[k] = static_cast<float>(src[windowStart + k % total]) * static_cast<float>(w);
        im[k] = 0.0f;
    }
    // 簡易 Cooley-Tukey（再帰でなく iterative）。
    // ※ kissFFT が既にリンクされていれば kiss_fft を使ってもよい。
    // 実装が重い場合は kFftSize=256 に下げる。
    // DFT 後に magnitude を kSpecBins にダウンサンプリングして specBuf へ書く。
    const int halfLen = fftLen / 2;
    for (int b = 0; b < kSpecBins; b++)
    {
        const int binStart = b * halfLen / kSpecBins;
        const int binEnd   = (b + 1) * halfLen / kSpecBins;
        float maxMag = 0.0f;
        for (int k = binStart; k < binEnd; k++)
        {
            maxMag = std::max(maxMag, std::sqrt(re[k]*re[k] + im[k]*im[k]));
        }
        specBuf[b] = maxMag;
    }
    // 正規化（最大値=1.0）
    const float specMax = *std::max_element(specBuf, specBuf + kSpecBins);
    if (specMax > 0.0f)
        for (int b = 0; b < kSpecBins; b++) specBuf[b] /= specMax;

    ImGui::TextDisabled("Spectrum (Tone Preview)");
    ImGui::PlotHistogram("##spec", specBuf, kSpecBins, 0, nullptr, 0.0f, 1.0f, ImVec2(-1.0f, 48.0f));
}
```

3. FFT 実装は cmath のみ依存で書く（外部ライブラリ追加なし）。
   - プロジェクトに kissFFT が既にある場合は置き換えてよい。
   - 簡易 DFT でも `O(N^2)` だが N=256 なら問題ない。

### 受け入れ条件

- Sound タブで Tone Preview を再生後、スペクトラムバーが更新される。
- sine 単音: 1本のバーが突出する。
- square/saw: 奇数/全倍音バーが並ぶ。
- FM ベル系: 非倍音成分のバーが見える。
- `check.ps1` 通過。

---

## Tier A-2: Noise → common shaper (filter) 接続

### 目的

`NoiseConfig` にフィルタパラメータを追加し、Renderer の common shaper を
noise source にも通すことで、帯域絞りノイズ（ハット/SFX）を作れるようにする。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/SynthEngine.h` | `NoiseConfig` にフィルタフィールド追加 |
| `src/SynthEngine/Renderer.cpp` | noise render 関数で `frame.shaperKind` を設定 |
| `src/SynthEngine/Voices.cpp` | noise 用 voice state に `filter` メンバを追加（他方式と同パターン） |
| `src/config/load/LoadSource.cpp` | `noise` の JSON キー `filterMode/filterCutoffHz/filterResonance` をロード |
| `src/config/ConfigJSONUtils.cpp` | noise の JSON 書き出しにフィルタフィールドを追加 |
| `src/config/SourceRegistry.cpp` | noise capability の `hasFilterIn = true` へ変更 |
| `src/gui/GUIChannelEditor.cpp` | noise 編集 UI にフィルタスライダーを追加 |

### 実装手順

#### Step 1: `NoiseConfig` にフィルタフィールドを追加

`include/SynthEngine/SynthEngine.h` の `struct NoiseConfig` を以下に変更:

```cpp
struct NoiseConfig
{
    NoiseType noise;
    // 共通フィルタ（Waveform と同じフィールド名・デフォルト値を使う）。
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
};
```

#### Step 2: noise 用 voice state にフィルタ追加

`src/SynthEngine/Voices.cpp` / `src/SynthEngine/Internal.h` 内で
WaveformConfig 向け voice state が `filter` メンバを持つ構造と同じパターンで
NoiseConfig 向け state にも `BiquadFilter filter{}` を追加する。
既存の `if constexpr (requires { st.filter; })` パターンが自動的に適用される。

#### Step 3: Renderer の noise render 関数で shaper を設定

`src/SynthEngine/Renderer.cpp` の noise レンダブロック（`NoiseConfig` の
`std::visit` 分岐内）を探し、`frame` への代入部分に以下を追加:

```cpp
frame.shaperKind = CommonShaperKind::BiquadFilter;
frame.shaperCutoffHz = src.filterCutoffHz;
frame.shaperResonance = src.filterResonance;  // 既存フィールド名を確認して合わせる
frame.shaperFilterMode = src.filterMode;
```

参考: waveform/analog の同等箇所（`frame.shaperKind = CommonShaperKind::BiquadFilter;`）
のパターンをそのまま踏襲する。

#### Step 4: Config ロード

`src/config/load/LoadSource.cpp` の noise ロード関数内に:

```cpp
// filterMode / filterCutoffHz / filterResonance の読み取り
// WaveformConfig のロードパターンと同じ関数（ReadFilterMode, ReadDouble 等）を使う
```

デフォルト値は `FilterMode::Bypass / 8000.0 / 0.707`。
フィールド未記載時はデフォルト値を使い、エラーにしない（後方互換）。

#### Step 5: SourceRegistry の capability 更新

`src/config/SourceRegistry.cpp` の noise エントリで `hasFilterIn = false` を
`hasFilterIn = true` へ変更する。

#### Step 6: GUI

`src/gui/GUIChannelEditor.cpp` の noise 編集ブロック（`NoiseConfig` 分岐内）に
waveform の filterMode/filterCutoffHz/filterResonance と同じ UI ウィジェットを追加。
`updateHoverHelp` も同パターンで追加する。

### 受け入れ条件

- `filterMode=lowpass, filterCutoffHz=2000` の noise を再生すると高域が削れた音になる。
- `filterMode=bypass` のデフォルトで旧来と同等の音になる。
- 既存の noise プリセット（`psg_noise_nesgb_sfx` など）がフィールドなしでも正常ロードできる。
- `check.ps1` 通過。

---

## Tier A-3: Tone Preview ノート選択

### 目的

Sound タブの Tone Preview で再生するノート番号をユーザーが指定できるようにする。
現状は `ResolveSoundTonePreviewNote` が自動決定するが、C2〜C6 を手動選択できると
音域別の聴感チェックが可能になる。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/gui/GUIStateStorage.h` | `tonePreviewNoteNumber` フィールド追加 |
| `include/gui/GUIState.h` | 同フィールドを `GUIState` に追加（または Storage 継承で取得） |
| `src/gui/GUIStateStorage.cpp` | JSON 永続化キー `"tonePreviewNoteNumber"` を追加 |
| `src/gui/GUIActions.cpp` | `ResolveSoundTonePreviewNote` で `state.tonePreviewNoteNumber` を優先参照 |
| `src/gui/main/MainWindow.inl` | Tone Preview 波形ビューアの直前にノート選択 SliderInt を追加 |

### 実装手順

#### Step 1: フィールド追加

`include/gui/GUIStateStorage.h` の `GUIStorageState` に:

```cpp
int tonePreviewNoteNumber = 60;  // C4 デフォルト
```

#### Step 2: 永続化

`src/gui/GUIStateStorage.cpp` の Save/Load に:

```cpp
// Save
j["tonePreviewNoteNumber"] = s.tonePreviewNoteNumber;
// Load
if (j.contains("tonePreviewNoteNumber")) s.tonePreviewNoteNumber = j["tonePreviewNoteNumber"].get<int>();
```

#### Step 3: `ResolveSoundTonePreviewNote` の修正

`src/gui/GUIActions.cpp` の `ResolveSoundTonePreviewNote` 冒頭に:

```cpp
// DrumKit 以外は tonePreviewNoteNumber を優先する。
const bool isDrum = state.channelConfigs &&
    std::get_if<DrumKitConfig>(&(*state.channelConfigs)[std::clamp(slot, 0, 15)].source);
if (!isDrum)
{
    return std::clamp(state.tonePreviewNoteNumber, 0, 127);
}
// DrumKit の場合は既存ロジックを維持する（以降は変更なし）
```

#### Step 4: GUI スライダー

`src/gui/main/MainWindow.inl` の Tone Preview 波形ビューアブロック
（`if (state.previewRenderedSound)` の直前）に:

```cpp
// ノート番号スライダー（DrumKit時は selectedDrumNote を流用するため表示しない）
{
    const bool isDrumSlot = state.channelConfigs &&
        std::get_if<DrumKitConfig>(&(*state.channelConfigs)[std::clamp(state.selectedSoundSlot, 0, 15)].source);
    if (!isDrumSlot)
    {
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::SliderInt("Preview Note", &state.tonePreviewNoteNumber, 24, 96))
        {
            state.tonePreviewNoteNumber = std::clamp(state.tonePreviewNoteNumber, 0, 127);
            requestAutoTonePreview();
        }
        updateHoverHelp(
            "Tone Preview の再生音程を変更します。",
            "C2(24)〜C6(84) の範囲で MIDI ノート番号を指定します。",
            "");
        ImGui::SameLine();
        // ノート名表示（例: C4, D3 など）
        const int oct = state.tonePreviewNoteNumber / 12 - 1;
        const char* noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        ImGui::TextDisabled("(%s%d)", noteNames[state.tonePreviewNoteNumber % 12], oct);
    }
}
```

### 受け入れ条件

- Sound タブでスライダーを動かすと Tone Preview が別音程で再生される。
- DrumKit スロット選択時はスライダーが表示されない。
- 設定値が `gui_state.json` に保存・復元される。
- `check.ps1` 通過。

---

## Tier B-1: FM アルゴリズム 4〜7 追加

### 目的

現状 0〜3 の FM アルゴリズムを 0〜7 に拡張し、OPN2 互換の全 8 トポロジを実現する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/SynthEngine.h` | `FmConfig::algorithm` のコメントを 0-7 に更新 |
| `src/SynthEngine/Renderer.cpp` | FM レンダの `switch(src.algorithm)` に case 4〜7 を追加 |
| `src/gui/GUIChannelEditor.cpp` | アルゴリズム選択 UI の表示範囲を 0〜7 に変更 |

### アルゴリズム定義（OPN2 互換）

```
alg 0:  [op0→op1→op2→op3]  (直列チェーン, キャリア: op3)
alg 1:  [(op0+op1)→op2→op3]  (並列開始)
alg 2:  [op0→(op1→op2)+op3 wait] -> [op0→op1, op2→op3], op0→op2 も使う形
alg 3:  [(op0→op1)→op2+op3]
alg 4:  [(op0→op1) + (op2→op3)]  (2ペア並列, キャリア: op1, op3)
alg 5:  [(op0→op1) + (op0→op2) + (op0→op3)]  (op0 → 3分岐, キャリア: op1,op2,op3)
alg 6:  [op0→op1 + op2 + op3]  (op0→op1 の後, op2/op3 は独立キャリア)
alg 7:  [op0 + op1 + op2 + op3]  (全op独立キャリア)
```

注: 既存 alg 0-3 の定義がコードと食い違う場合は既存の実装コメントを優先し、
4-7 をその続きとして定義すること（既存プリセット破綻防止）。

### 実装手順

`src/SynthEngine/Renderer.cpp` の FM switch 文末尾に追加:

```cpp
case 4:
    // (op0→op1) + (op2→op3), キャリア: op1+op3
    {
        double m0 = std::sin(phase[0] + op0FB);
        double m1 = std::sin(phase[1] + m0 * ops[0].index);
        double m2 = std::sin(phase[2]);
        double m3 = std::sin(phase[3] + m2 * ops[2].index);
        mainWave = (m1 * ops[1].level + m3 * ops[3].level) * 0.5;
    }
    break;
case 5:
    // op0 → op1, op2, op3 (3並列変調), キャリア: op1+op2+op3
    {
        double m0 = std::sin(phase[0] + op0FB);
        double m1 = std::sin(phase[1] + m0 * ops[0].index);
        double m2 = std::sin(phase[2] + m0 * ops[0].index);
        double m3 = std::sin(phase[3] + m0 * ops[0].index);
        mainWave = (m1 * ops[1].level + m2 * ops[2].level + m3 * ops[3].level) / 3.0;
    }
    break;
case 6:
    // op0→op1, op2/op3 独立, キャリア: op1+op2+op3
    {
        double m0 = std::sin(phase[0] + op0FB);
        double m1 = std::sin(phase[1] + m0 * ops[0].index);
        double m2 = std::sin(phase[2]);
        double m3 = std::sin(phase[3]);
        mainWave = (m1 * ops[1].level + m2 * ops[2].level + m3 * ops[3].level) / 3.0;
    }
    break;
case 7:
    // 全op独立キャリア
    {
        double m0 = std::sin(phase[0] + op0FB);
        double m1 = std::sin(phase[1]);
        double m2 = std::sin(phase[2]);
        double m3 = std::sin(phase[3]);
        mainWave = (m0 * ops[0].level + m1 * ops[1].level +
                    m2 * ops[2].level + m3 * ops[3].level) * 0.25;
    }
    break;
```

注: 変数名（`phase`, `op0FB`, `ops`, `mainWave`）は既存 case 0-3 と合わせる。
実際の変数名が異なる場合は既存コードの変数名を確認してから使うこと。

GUI で `algorithm` のスライダー/コンボ上限を 3 → 7 に変更する
（`GUIChannelEditor.cpp` の FM 編集ブロック内）。

### 受け入れ条件

- alg 0-3 の既存プリセット（`fm_mdpc88_*`）がAB前後で同等の音になる。
- alg 4: 2ペア並列の倍音が出る。
- alg 7: 4つの独立 sine が混合される（クリーン）。
- `check.ps1` 通過。

---

## Tier B-2: filterResonance を modulation destination に追加

### 目的

`ModDestination::FilterResonance` を追加し、Env2 → filterResonance のルーティングで
共鳴が時間変化するフィルタースイープを実現する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `ModDestination` enum に `FilterResonance` を追加、`ModResult` に `resonanceMul` 追加 |
| `src/SynthEngine/Modulation.cpp` | switch の `FilterCutoff` case と同パターンで `FilterResonance` case を追加、clamp 追加 |
| `src/SynthEngine/Renderer.cpp` | `SetFilterResonance` 呼び出し箇所で `resonanceMul` を乗算 |
| `src/config/load/LoadSource.cpp` | destination 文字列 `"filterResonance"` の解決を追加 |
| `src/gui/GUIChannelEditor.cpp` | modulation route の destination コンボに `FilterResonance` を追加 |

### 実装手順

#### Step 1: enum + ModResult 拡張

`include/SynthEngine/Modulation.h`:

```cpp
enum class ModDestination
{
    None,
    Pitch,
    Amp,
    FilterCutoff,
    FilterResonance,   // 追加
    FmIndex
};

struct ModResult
{
    double pitchMul = 1.0;
    double ampMul = 1.0;
    double filterCutoffMul = 1.0;
    double resonanceMul = 1.0;   // 追加（乗算適用, 1.0=変化なし）
    double fmIndexMul = 1.0;
};
```

#### Step 2: Modulation.cpp に case 追加

既存の `FilterCutoff` case の直後:

```cpp
case ModDestination::FilterResonance:
    out.resonanceMul *= (1.0 + value);
    break;
```

clamp 行の追加:

```cpp
out.resonanceMul = std::clamp(out.resonanceMul, 0.0, 10.0);  // 最大10倍
```

#### Step 3: Renderer.cpp で resonanceMul を適用

`SetFilterResonance(st.filter, baseResonance * ResonanceScaleFromCc(in.resonance));`
の行を:

```cpp
SetFilterResonance(st.filter,
    baseResonance * ResonanceScaleFromCc(in.resonance) * mod.resonanceMul);
```

に変更する（`mod` は既にスコープ内にある `ModResult`）。

#### Step 4: ConfigLoad の destination 解決

`src/config/load/LoadSource.cpp` の destination 文字列→enum 変換箇所に:

```cpp
if (s == "filterResonance") return ModDestination::FilterResonance;
```

#### Step 5: GUI

`src/gui/GUIChannelEditor.cpp` の destination コンボ項目配列に `"FilterResonance"` を追加。
`ModDestination::FilterResonance` への変換を同ファイル内の解決関数に追加する。

### 受け入れ条件

- `Env2 -> filterResonance, amount=0.5` の waveform でアタック中に共鳴が上昇する。
- `amount=0.0` で既存と同等の音になる（`resonanceMul=1.0` 維持）。
- FM / Noise / DrumKit ではフィルタがない場合でも `resonanceMul` 適用が無害（フィルタなし方式は shaper を通らない）。
- `check.ps1` 通過。

---

## タスク完了時の処置

各タスク完了後:
1. `docs/STATUS.md` の `Current` を更新する。
2. 判断事項があれば `docs/DECISIONS.md` に追記する。
3. 全タスク完了後、本ファイル（`next-features.md`）を削除する。
