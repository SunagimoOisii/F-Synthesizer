# Next Features Roadmap

作成: 2026-03-26
状態: 実装作業用（実装完了後に破棄）
優先順位: Tier C → Tier D の順で着手する。

---

## 共通ルール（全タスク共通）

- 変更後は `./F-Synthesizer/scripts/check.ps1` を必ず実行する。
- 新規ファイルは作らない（既存ファイルの最小変更で完結させる）。
- 合成ロジック変更時は代表MIDI 1件でAB耳確認を行う。
- `docs/STATUS.md` の `Current` と `DECISIONS.md` を更新してタスクを閉じる。

---

## Tier C-1: LFO 波形追加（Square / Saw / SampleAndHold）(完了)

### 目的

`LfoWave` に `Square`・`Saw`・`SampleAndHold` を追加し、
トレモロ・ランダムステップ変調などの定番パターンを実現する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `LfoWave` enum に 3 値追加 |
| `src/SynthEngine/Modulation.cpp` | `SampleLfoWave` switch に case 追加 |
| `src/config/load/LoadSource.cpp` | LfoWave 文字列→enum 解決に 3 値追加 |
| `src/config/ConfigJSONUtils.cpp` | LfoWave→文字列シリアライズに 3 値追加 |
| `src/gui/GUIChannelEditor.cpp` | LFO wave コンボボックスに 3 項目追加 |

### 実装手順

#### Step 1: `LfoWave` enum 拡張

`include/SynthEngine/Modulation.h` の `enum class LfoWave` に追加:

```cpp
enum class LfoWave
{
    Sine,
    Triangle,
    Square,        // 追加: duty 50% の矩形波
    Saw,           // 追加: 上昇ノコギリ波 (0→1→-1 ジャンプ)
    SampleAndHold  // 追加: phase が整数を跨ぐたびに決定論的乱数を更新
};
```

#### Step 2: `SampleLfoWave` switch に case 追加

`src/SynthEngine/Modulation.cpp` の `SampleLfoWave` 関数内、既存の `Triangle` case 直後:

```cpp
case LfoWave::Square:
    return (phase < 0.5) ? 1.0 : -1.0;
case LfoWave::Saw:
    // 0→+1 上昇、折り返しで -1 に戻る
    return 2.0 * phase - 1.0;
case LfoWave::SampleAndHold:
{
    // 外部乱数状態不要: floor(phase) をシードとした xorshift で決定論的に生成。
    // phase が新しい整数に入るたびに値が変化する。
    uint32_t seed = static_cast<uint32_t>(std::floor(phase));
    seed ^= seed << 13u;
    seed ^= seed >> 17u;
    seed ^= seed << 5u;
    // [0, 0xFFFFFFFF] → [-1.0, +1.0]
    return static_cast<double>(seed) / 2147483647.5 - 1.0;
}
```

`SampleLfoWave` は `<cstdint>` の `uint32_t` を使う。`Modulation.cpp` に `#include <cstdint>` がなければ追加する。

#### Step 3: ConfigLoad — 文字列→enum 解決

`src/config/load/LoadSource.cpp` の LfoWave 解決箇所（`"sine"` / `"triangle"` を返す if-else 等）に追加:

```cpp
if (s == "square")        return LfoWave::Square;
if (s == "saw")           return LfoWave::Saw;
if (s == "sampleAndHold") return LfoWave::SampleAndHold;
```

#### Step 4: ConfigJSONUtils — enum→文字列シリアライズ

`src/config/ConfigJSONUtils.cpp` の LfoWave→文字列変換箇所に追加:

```cpp
case LfoWave::Square:        return "square";
case LfoWave::Saw:           return "saw";
case LfoWave::SampleAndHold: return "sampleAndHold";
```

#### Step 5: GUI コンボボックス

`src/gui/GUIChannelEditor.cpp` の LFO wave コンボ項目配列（`"Sine"`, `"Triangle"` が並んでいる箇所）に追加:

```cpp
"Square", "Saw", "S&H"
```

enum との対応順が一致していることを確認する。

### 受け入れ条件

- `wave=square` の Lfo1 → Amp: 音量が矩形的にオン/オフする。
- `wave=sampleAndHold` の Lfo1 → FilterCutoff: カットオフがステップ状にランダム変化する。
- 既存の `sine` / `triangle` プリセットが音変化なし。
- `check.ps1` 通過。

---

## Tier C-2: LFO key sync(完了)

### 目的

`LfoConfig` に `keySync` フラグを追加し、ノートオン時に LFO 位相を 0 にリセットできるようにする。
現状は free-run 固定のため、音符ごとに LFO 位相が揃わない。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `LfoConfig` に `keySync` 追加、`NoteOnModulation` 引数に `cfg` 追加 |
| `src/SynthEngine/Modulation.cpp` | `NoteOnModulation` で keySync 時に `lfo1Phase = 0.0` |
| `src/SynthEngine/Voices.cpp` | `NoteOnModulation` 呼び出し箇所に `cfg` を渡す |
| `src/config/load/LoadSource.cpp` | JSON キー `"keySync"` の bool ロード追加 |
| `src/config/ConfigJSONUtils.cpp` | JSON 書き出しに `"keySync"` 追加 |
| `src/gui/GUIChannelEditor.cpp` | LFO 編集 UI に `Key Sync` チェックボックス追加 |

### 実装手順

#### Step 1: `LfoConfig` に `keySync` 追加

`include/SynthEngine/Modulation.h` の `struct LfoConfig`:

```cpp
struct LfoConfig
{
    LfoWave wave = LfoWave::Sine;
    double rateHz = 5.0;
    double depth = 0.0;
    bool bipolar = true;
    bool keySync = false;  // 追加: true でノートオン時に位相を 0 にリセット
};
```

#### Step 2: `NoteOnModulation` 引数変更

`include/SynthEngine/Modulation.h` の宣言を変更:

```cpp
void NoteOnModulation(ModulationRuntimeState& state, const ModulationConfig& cfg);
```

`src/SynthEngine/Modulation.cpp` の定義:

```cpp
void NoteOnModulation(ModulationRuntimeState& state, const ModulationConfig& cfg)
{
    NoteOn(state.env2);
    if (cfg.lfo1.keySync)
    {
        state.lfo1Phase = 0.0;
    }
}
```

#### Step 3: `Voices.cpp` の呼び出し更新

`src/SynthEngine/Voices.cpp` で `NoteOnModulation(state)` を呼んでいる箇所を
`NoteOnModulation(state, cfg)` に変更する。
`cfg` は voice に紐付く `ModulationConfig` の参照（既存の `NoteOff` 等のパターンと同様に
呼び出しスコープ内にある `modulationCfg` または同等変数を渡す）。

#### Step 4: ConfigLoad

`src/config/load/LoadSource.cpp` の LFO ロード箇所（`rateHz`, `depth` を読んでいる近辺）に追加:

```cpp
if (auto v = ReadJSONBool(lfoObjText, "keySync"))
    cfg.lfo1.keySync = *v;
```

#### Step 5: ConfigJSONUtils

LFO 書き出しブロックに追加:

```cpp
WriteIndent(out, N); out << "\"keySync\": " << (cfg.lfo1.keySync ? "true" : "false") << ",\n";
```

インデント深さ `N` は既存の `"rateHz"` 行と揃える。

#### Step 6: GUI

`src/gui/GUIChannelEditor.cpp` の LFO 編集ブロックに:

```cpp
ImGui::Checkbox("Key Sync", &src.modulation.lfo1.keySync);
updateHoverHelp(
    "ノートオン時に LFO 位相を 0 にリセットします。",
    "オフの場合は free-run（位相を引き継ぐ）です。",
    "");
```

### 受け入れ条件

- `keySync=true` + `wave=sine` の Lfo1 → Pitch: 和音を弾いたとき各ボイスの LFO が揃って聞こえる。
- `keySync=false` で従来通り free-run。
- 既存プリセット（`keySync` フィールドなし JSON）が正常にロードされる（デフォルト `false`）。
- `check.ps1` 通過。

---

## Tier C-3: ModSource::ModWheel を Mod Matrix に追加(完了)

### 目的

CC1（モジュレーションホイール）をモジュレーションマトリクスの独立ルーティングソースとして
選択できるようにする。現状は Lfo1 depth への固定乗算のみで、FilterCutoff 等への直接ルーティングができない。

### 設計注意

現在 `EvaluateModulation` 内で `lfo1 *= (1.0 + std::clamp(input.modwheel, 0.0, 1.0))` が
無条件に実行されている（`Modulation.cpp` 約 129 行目）。
`ModSource::ModWheel` 追加後もこの既存乗算は **維持**する（後方互換）。
新規プリセットは `ModWheel` ソースを明示ルーティングして使う想定。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `ModSource` enum に `ModWheel` 追加 |
| `src/SynthEngine/Modulation.cpp` | `EvaluateModulation` に `ModWheel` ソース評価 + case 追加 |
| `src/config/load/LoadSource.cpp` | `"modWheel"` 文字列→enum 解決追加 |
| `src/config/ConfigJSONUtils.cpp` | `ModSource::ModWheel` →文字列シリアライズ追加 |
| `src/gui/GUIChannelEditor.cpp` | source コンボに `"ModWheel"` 追加 |

### 実装手順

#### Step 1: enum 拡張

`include/SynthEngine/Modulation.h` の `enum class ModSource`:

```cpp
enum class ModSource
{
    None,
    Lfo1,
    Env2,
    Velocity,
    ChannelPressure,
    PolyPressure,
    ModWheel        // 追加: CC1 (0..1 正規化済み)
};
```

#### Step 2: `EvaluateModulation` に ModWheel ソース追加

`src/SynthEngine/Modulation.cpp` の `EvaluateModulation` 内、
`usePolyPressure` の隣に:

```cpp
bool useModWheel = false;
```

ソース検出ループ内に:

```cpp
useModWheel = useModWheel || route.source == ModSource::ModWheel;
```

値計算行に:

```cpp
const double modWheel = useModWheel ? std::clamp(input.modwheel, 0.0, 1.0) : 0.0;
```

`srcValue` switch に:

```cpp
case ModSource::ModWheel: srcValue = modWheel; break;
```

#### Step 3: ConfigLoad

```cpp
if (s == "modWheel") return ModSource::ModWheel;
```

#### Step 4: ConfigJSONUtils

```cpp
case ModSource::ModWheel: return "modWheel";
```

#### Step 5: GUI

source コンボ項目配列の末尾に `"ModWheel"` を追加する。
enum との対応順が一致していることを確認する。

### 受け入れ条件

- `ModWheel → FilterCutoff, amount=0.5` のルートで CC1 を動かすとカットオフが変化する。
- 既存の `Lfo1 → FilterCutoff` ルート + modwheel の組み合わせで二重適用されるが、
  これは仕様（後方互換維持）。
- 既存プリセットが正常ロードされる。
- `check.ps1` 通過。

---

## Tier D-1: LFO delay + fade-in(完了)

### 目的

`LfoConfig` に `delayMs` / `fadeMs` を追加し、ノートオン後に LFO が遅れて
立ち上がる自然なビブラートを実現する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `LfoConfig` に `delayMs`/`fadeMs` 追加、`ModulationRuntimeState` に `lfo1ElapsedSec` 追加 |
| `src/SynthEngine/Modulation.cpp` | `NoteOnModulation` でリセット、`StepLfoSample` で delay/fade を適用 |
| `src/config/load/LoadSource.cpp` | JSON キー `"delayMs"` / `"fadeMs"` のロード追加 |
| `src/config/ConfigJSONUtils.cpp` | 書き出しに追加 |
| `src/gui/GUIChannelEditor.cpp` | LFO 編集 UI にスライダー追加 |

### 実装手順

#### Step 1: フィールド追加

`include/SynthEngine/Modulation.h`:

```cpp
struct LfoConfig
{
    LfoWave wave = LfoWave::Sine;
    double rateHz = 5.0;
    double depth = 0.0;
    bool bipolar = true;
    bool keySync = false;
    double delayMs = 0.0;   // 追加: LFO が始まるまでの無音時間 (ms)
    double fadeMs  = 0.0;   // 追加: LFO が最大 depth に達するまでのフェード時間 (ms)
};

struct ModulationRuntimeState
{
    double lfo1Phase = 0.0;
    double lfo1ElapsedSec = 0.0;  // 追加: ノートオンからの経過秒数
    ADSRState env2{};
    double env2Value = 0.0;
};
```

#### Step 2: `NoteOnModulation` でリセット

`src/SynthEngine/Modulation.cpp` の `NoteOnModulation` に追加:

```cpp
state.lfo1ElapsedSec = 0.0;
```

#### Step 3: `StepLfoSample` に delay/fade 適用

現在の `StepLfoSample` シグネチャを変更:

```cpp
double StepLfoSample(ModulationRuntimeState& state, const LfoConfig& lfo, double deltaTimeSec)
```

（`phase` 参照を `state.lfo1Phase` に置き換え、elapsed を更新）

関数内の計算ロジック:

```cpp
double StepLfoSample(ModulationRuntimeState& state, const LfoConfig& lfo, double deltaTimeSec)
{
    state.lfo1ElapsedSec += deltaTimeSec;

    const double delayEnd = lfo.delayMs * 0.001;
    if (state.lfo1ElapsedSec < delayEnd)
    {
        // delay 中は位相を進めない（位相固定）
        return 0.0;
    }

    const double rate = std::clamp(lfo.rateHz, 0.0, 100.0);
    state.lfo1Phase = WrapPhase(state.lfo1Phase + (rate * deltaTimeSec));

    double raw = SampleLfoWave(lfo.wave, state.lfo1Phase);
    if (!lfo.bipolar)
    {
        raw = (raw * 0.5) + 0.5;
    }
    raw *= std::clamp(lfo.depth, 0.0, 1.0);

    // fade-in 適用
    const double fadeTotal = lfo.fadeMs * 0.001;
    if (fadeTotal > 0.0)
    {
        const double elapsed = state.lfo1ElapsedSec - delayEnd;
        const double fadeFactor = std::clamp(elapsed / fadeTotal, 0.0, 1.0);
        raw *= fadeFactor;
    }

    return raw;
}
```

`EvaluateModulation` 内の `StepLfoSample(state.lfo1Phase, ...)` 呼び出しを
`StepLfoSample(state, ...)` に変更する。

#### Step 4: 宣言更新

`include/SynthEngine/Modulation.h` の `StepLfoSample` 宣言を新シグネチャに合わせる。

#### Step 5: ConfigLoad / ConfigJSONUtils / GUI

既存の `rateHz` / `depth` のロード・保存・GUI スライダーと同パターンで
`delayMs`（0〜2000 ms）と `fadeMs`（0〜2000 ms）を追加する。

### 受け入れ条件

- `delayMs=500, fadeMs=300` の Lfo1 → Pitch: ノートオン 0.5 秒後にビブラートが始まり 0.3 秒で最大になる。
- `delayMs=0, fadeMs=0` で従来と同等の挙動（delay 中も位相を進めない点は許容）。
- `check.ps1` 通過。

---

## Tier D-2: Envelope curve（linear / exponential 切り替え）(完了)

### 目的

`ModEnvelopeConfig`（Env2）の各セグメントに curve パラメータを追加し、
指数カーブによるアナログ的なリリース感を実現する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `ModEnvelopeConfig` に `curve` 追加 |
| `include/SynthEngine/synth/Envelope.h` | `StepADSR` に curve 引数追加（または新オーバーロード） |
| `src/SynthEngine/synth/Envelope.cpp` | curve 適用（pow ベース） |
| `src/SynthEngine/Modulation.cpp` | `StepEnv2Sample` で `cfg.env2.curve` を渡す |
| `src/config/load/LoadSource.cpp` | `"curve"` ロード追加 |
| `src/config/ConfigJSONUtils.cpp` | 書き出しに追加 |
| `src/gui/GUIChannelEditor.cpp` | Env2 編集 UI に curve スライダー追加 |

### 実装手順

#### Step 1: `ModEnvelopeConfig` に curve 追加

```cpp
struct ModEnvelopeConfig
{
    double attackSec  = 0.01;
    double decaySec   = 0.1;
    double sustainLevel = 1.0;
    double releaseSec = 0.1;
    double curve = 0.0;  // 追加: 0.0=linear, 1.0=強い指数カーブ (range: 0.0〜1.0)
};
```

#### Step 2: カーブ適用

ADSR の線形 0→1 進行 `t`（0〜1）に対して以下を適用する:

```cpp
// curve=0 で linear、curve>0 で指数的に遅くなる (pow(t, 1/(1+curve*4)))
// Envelope.cpp の StepADSR 内、または Modulation.cpp の StepEnv2Sample 内で適用する。
double ApplyCurve(double t, double curve)
{
    if (curve <= 0.0) return t;
    const double exp = 1.0 / (1.0 + curve * 4.0);  // curve=1.0 → t^0.2 (遅い立ち上がり)
    return std::pow(std::clamp(t, 0.0, 1.0), exp);
}
```

ADSR の各フェーズ（attack/decay/release）の進行値 `t` にこの関数を通して出力する。

既存の `StepADSR` のシグネチャを変えず、`StepEnv2Sample` 内で
`StepADSR` の戻り値に後処理として適用する形が最小変更:

```cpp
double StepEnv2Sample(ModulationRuntimeState& state, const ModEnvelopeConfig& env2, double deltaTimeSec)
{
    // 既存 StepADSR 呼び出し（変更なし）
    const double v = StepADSR(...);
    // curve 適用（attack/decay/release フェーズ外の sustain は一定なので影響小）
    const double curved = ApplyCurve(v, std::clamp(env2.curve, 0.0, 1.0));
    state.env2Value = curved;
    return curved;
}
```

#### Step 3: ConfigLoad / ConfigJSONUtils / GUI

`curve`（0.0〜1.0）を `attackSec` 等と同パターンで追加する。
GUI は `SliderFloat("Curve", &env2.curve, 0.0f, 1.0f)` の 1 行。

### 受け入れ条件

- `curve=1.0` の Env2 → Amp: リリースが指数的に減衰し、`curve=0.0` より自然に聞こえる。
- `curve=0.0` で従来の linear と同等。
- `check.ps1` 通過。

---

## Tier D-3: 第2 LFO（Lfo2）

### 目的

`ModulationConfig` に `lfo2` を追加し、`ModSource::Lfo2` としてマトリクスで選択できるようにする。
これにより「Lfo1 → Pitch（ビブラート）+ Lfo2 → FilterCutoff（フィルタLFO）」のような
2系統変調が可能になる。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/Modulation.h` | `ModSource::Lfo2` 追加、`ModulationConfig` に `lfo2` 追加、`ModulationRuntimeState` に `lfo2Phase`/`lfo2ElapsedSec` 追加 |
| `src/SynthEngine/Modulation.cpp` | `ResetModulationState`・`NoteOnModulation`・`EvaluateModulation` に Lfo2 対応追加 |
| `src/config/load/LoadSource.cpp` | `"lfo2"` オブジェクトのロード追加 |
| `src/config/ConfigJSONUtils.cpp` | `lfo2` の書き出し追加 |
| `src/gui/GUIChannelEditor.cpp` | LFO2 設定 UI 追加、source コンボに `"Lfo2"` 追加 |

### 実装手順

#### Step 1: 構造体拡張

`include/SynthEngine/Modulation.h`:

```cpp
enum class ModSource
{
    None,
    Lfo1,
    Lfo2,           // 追加
    Env2,
    Velocity,
    ChannelPressure,
    PolyPressure,
    ModWheel
};

struct ModulationConfig
{
    LfoConfig lfo1{};
    LfoConfig lfo2{};   // 追加（デフォルト値は lfo1 と同じ）
    ModEnvelopeConfig env2{};
    ModMatrix matrix = ...;
};

struct ModulationRuntimeState
{
    double lfo1Phase = 0.0;
    double lfo1ElapsedSec = 0.0;
    double lfo2Phase = 0.0;       // 追加
    double lfo2ElapsedSec = 0.0;  // 追加（D-1 を先に実装した場合）
    ADSRState env2{};
    double env2Value = 0.0;
};
```

#### Step 2: `EvaluateModulation` に Lfo2 対応

`src/SynthEngine/Modulation.cpp` の `EvaluateModulation` 内で
`useLfo1` パターンと完全対称に `useLfo2` を追加する:

```cpp
bool useLfo2 = false;
// ソース検出ループ内:
useLfo2 = useLfo2 || route.source == ModSource::Lfo2;
// 値計算:
double lfo2 = useLfo2 ? StepLfoSample(state.lfo2Phase, cfg.lfo2, deltaTimeSec) : 0.0;
// D-1 適用後なら StepLfoSample(state, lfo2Phase/lfo2ElapsedSec, cfg.lfo2, ...) に合わせる
// srcValue switch:
case ModSource::Lfo2: srcValue = lfo2; break;
```

`ResetModulationState` と `NoteOnModulation` にも `lfo2Phase=0.0` / `lfo2ElapsedSec=0.0` を追加する。

#### Step 3: ConfigLoad / ConfigJSONUtils

`lfo2` は `lfo1` と完全に同じキー構造で `"lfo2"` オブジェクトとして読み書きする。
既存の `lfo1` ロード関数を `LoadLfoConfig(text, "lfo1")` のような形で共通化しているなら、
`"lfo2"` を引数に変えて呼ぶだけ。

#### Step 4: GUI

LFO1 の編集 UI ブロックをそのままコピーして `lfo2` 向けに変数名を変える。
`ImGui::CollapsingHeader("LFO 2", ...)` で折りたたみ可にすると UI が締まる。

source コンボ項目配列に `"Lfo2"` を `"Lfo1"` の直後に追加する。

### 受け入れ条件

- `Lfo1 → Pitch + Lfo2 → FilterCutoff` を同時に使えること。
- `lfo2` フィールドのない旧プリセットが正常ロードされる（デフォルト適用）。
- `check.ps1` 通過。

---

## タスク完了時の処置

各タスク完了後:
1. `docs/STATUS.md` の `Current` を更新する。
2. 判断事項があれば `docs/DECISIONS.md` に追記する。
3. 全タスク完了後、本ファイル（`next-features.md`）を削除する。
