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

## Tier C-1: Pulse Width + ModDestination::PulseWidth(完了)

### 目的

Waveform / Analog ソースの square 波に可変パルス幅を追加し、
LFO → PulseWidth ルーティングで SID/NES 的な PWM サウンドを実現する。

### 背景

`src/synth/Oscillator.cpp` の `SampleWavePhase` は square で `p < 0.5` を固定閾値として使う。
PolyBLEP 補正はすでに実装済み。PSG は `duty` パラメータを持つが、Waveform / Analog には未実装。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/SynthEngine.h` | `WaveformConfig` / `AnalogConfig` に `pulseWidth` 追加 |
| `include/synth/Oscillator.h` | `SampleWavePhase` 宣言に `pulseWidth` 引数追加 |
| `src/synth/Oscillator.cpp` | Square case の閾値を `pulseWidth` で差し替え、PolyBLEP 補正も更新 |
| `include/SynthEngine/Modulation.h` | `ModDestination::PulseWidth` 追加、`ModulationResult` に `pulseWidthAdd` 追加 |
| `src/SynthEngine/Modulation.cpp` | switch に `PulseWidth` case 追加 |
| `src/SynthEngine/Renderer.cpp` | waveform / analog render ブロックで `effectivePW` を計算して渡す |
| `src/config/load/LoadSource.cpp` | `"pulseWidth"` キーのロード追加 |
| `src/config/ConfigJSONUtils.cpp` | `pulseWidth` の書き出し追加 |
| `src/gui/GUIChannelEditor.cpp` | パルス幅スライダーと ModDestination コンボ項目を追加 |

### 実装手順

#### Step 1: SynthEngine.h — フィールド追加

`WaveformConfig` と `AnalogConfig` に追加（既存フィールドの末尾あたり）:

```cpp
double pulseWidth = 0.5;  // 0.05..0.95。wave=Square のみ有効。
```

#### Step 2: Oscillator.h — 宣言更新

```cpp
double SampleWavePhase(
    WaveType type,
    double phase,
    double phaseInc = 0.0,
    double pulseWidth = 0.5);   // 追加（デフォルト 0.5 で既存呼び出し全て互換）
```

#### Step 3: Oscillator.cpp — Square case を更新

`SampleWavePhase` の Square case（現行 `p < 0.5`）を以下に差し替える:

```cpp
case WaveType::Square:
{
    const double pw = std::clamp(pulseWidth, 0.05, 0.95);
    double w = (p < pw) ? 1.0 : -1.0;
    w += PolyBlep(p, dt);                               // 立ち上がりエッジ（p=0）
    w -= PolyBlep(NormalizePhase(p + 1.0 - pw), dt);   // 立ち下がりエッジ（p=pw）
    return w;
}
```

注: `pulseWidth = 0.5` のとき `1.0 - pw = 0.5` → 既存と同一になる。

#### Step 4: Modulation.h — PulseWidth destination

`ModDestination` enum に追加:

```cpp
PulseWidth     // 追加: 加算適用（pulseWidthAdd）
```

`ModulationResult` に追加:

```cpp
double pulseWidthAdd = 0.0;   // 加算。0.0 = 変化なし。
```

#### Step 5: Modulation.cpp — case 追加

既存の `FilterResonance` case の後ろに:

```cpp
case ModDestination::PulseWidth:
    out.pulseWidthAdd += value * 0.45;  // depth=1.0 で最大 ±0.45 (0.05..0.95 に収まる)
    break;
```

`out.pulseWidthAdd` へのクランプは Renderer 側で `effectivePW` 計算時に行う。

#### Step 6: Renderer.cpp — effectivePW 計算と渡し

waveform render ブロック（`auto& ws = std::get<WaveformVoiceState>(...)`）の
pitchMul 計算直後（`mod` が確定した後）に:

```cpp
const double effectivePW = std::clamp(src.pulseWidth + mod.pulseWidthAdd, 0.05, 0.95);
```

unison ループ内の `SampleWavePhase` 呼び出し（現在 `SampleWavePhase(src.wave, uvPhase, uvInc)`）を:

```cpp
SampleWavePhase(src.wave, uvPhase, uvInc, effectivePW)
```

sub-osc の呼び出し（`SampleWavePhase(src.wave, subPhase, phaseInc * 0.5)`）も同様に:

```cpp
SampleWavePhase(src.wave, subPhase, phaseInc * 0.5, effectivePW)
```

analog render ブロック（`auto& as = std::get<AnalogVoiceState>(...)` 、Renderer.cpp 約 257 行目）も
同じパターンで `effectivePW` を計算して渡す。

#### Step 7: ConfigLoad / ConfigJSONUtils

`pulseWidth`（0.05〜0.95）を waveform/analog の load / save に追加する。
既存の `filterCutoffHz` の追加パターンに倣う。

#### Step 8: GUI

waveform / analog 編集ブロックの wave 選択コンボ直後に:

```cpp
if (src->wave == WaveType::Square)
{
    ImGui::SliderFloat("Pulse Width", &src->pulseWidth, 0.05f, 0.95f);
}
```

ModDestination コンボ項目に `"PulseWidth"` を追加し、文字列→enum 解決も追加する。

### 受け入れ条件

- `pulseWidth=0.12` の square: NES デューティ 1/8 相当の細い音になる。
- `Lfo1(Triangle) → PulseWidth, amount=0.5`: SID 的にうねる PWM サウンドになる。
- `pulseWidth=0.5` で既存 square と同等の音（PolyBLEP 補正込み）。
- `check.ps1` 通過。

---

## Tier C-2: Ring Modulation(完了)

### 目的

Waveform / Analog ソースにリング変調を追加し、
金属的な鐘音・SID 系 SFX を作れるようにする。

### 背景

リング変調 = メイン信号 × 変調信号（サイン波）。
変調信号の周波数は `baseFreq * ringModRatio` で決まる。
mix パラメータで dry/wet をコントロールする。

Voice 側の状態変数: `WaveformVoiceState` / `AnalogVoiceState` に `ringPhase` を追加する
（`Internal.h` を変更）。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/SynthEngine.h` | `WaveformConfig` / `AnalogConfig` に ring mod フィールド追加 |
| `src/SynthEngine/Internal.h` | `WaveformVoiceState` / `AnalogVoiceState` に `ringPhase` 追加 |
| `src/SynthEngine/Renderer.cpp` | waveform / analog render ブロックでリング変調を適用 |
| `src/SynthEngine/Voices.cpp` | NoteOn 時に `ringPhase = 0.0` リセット |
| `src/config/load/LoadSource.cpp` | `"ringModEnabled"` / `"ringModRatio"` / `"ringModMix"` のロード追加 |
| `src/config/ConfigJSONUtils.cpp` | ring mod フィールドの書き出し追加 |
| `src/gui/GUIChannelEditor.cpp` | ring mod 編集 UI 追加 |

### 実装手順

#### Step 1: SynthEngine.h — フィールド追加

`WaveformConfig` と `AnalogConfig` に:

```cpp
bool ringModEnabled = false;
double ringModRatio = 2.0;   // 変調信号の周波数 = baseFreq * ratio
double ringModMix   = 1.0;   // 0.0=dry, 1.0=full ring mod
```

#### Step 2: Internal.h — ringPhase 追加

`WaveformVoiceState` に:

```cpp
double ringPhase = 0.0;
```

`AnalogVoiceState` にも同様に追加する（`driftPhase` の隣あたり）。

#### Step 3: Renderer.cpp — リング変調の適用

waveform render ブロックの `frame.sample = mainWave;` の直前に:

```cpp
if (src.ringModEnabled && src.ringModMix > 0.0)
{
    const double ringInc = phaseInc * std::clamp(src.ringModRatio, 0.125, 16.0);
    ws.ringPhase = WrapPhase(ws.ringPhase + ringInc);
    const double ringSignal = std::sin(2.0 * kPi * ws.ringPhase);
    const double mix = std::clamp(src.ringModMix, 0.0, 1.0);
    mainWave *= (1.0 - mix) + mix * ringSignal;
}
```

analog render ブロックも同じパターンで `as.ringPhase` を使って適用する。

`WrapPhase` と `kPi` は Renderer.cpp 内で既に定義済み。

#### Step 4: Voices.cpp — ringPhase リセット

waveform NoteOn 初期化ブロック（`voices.sourceState[i] = WaveformVoiceState{}` の直後）に:

```cpp
ws.ringPhase = 0.0;
```

analog の `as.ringPhase = 0.0;` も同様。

#### Step 5: ConfigLoad / ConfigJSONUtils / GUI

既存の `subOscLevel` スライダーと同パターンで:

```cpp
ImGui::Checkbox("Ring Mod", &src->ringModEnabled);
if (src->ringModEnabled)
{
    ImGui::SliderFloat("Ring Ratio", &src->ringModRatio, 0.125f, 16.0f);
    ImGui::SliderFloat("Ring Mix",   &src->ringModMix,   0.0f,   1.0f);
}
```

### 受け入れ条件

- `ringModEnabled=true, ringModRatio=2.0, ringModMix=1.0` + sine: 金属的な倍音が乗る。
- `ringModMix=0.0` で `enabled=true` でも音が変わらない（乗算係数が 1.0 になる）。
- `ringModEnabled=false` のデフォルトで既存と同等の音。
- `check.ps1` 通過。

---

## Tier D-1: Arpeggio(完了)

### 目的

Waveform / Analog ソースにアルペジオ機能を追加し、
1 チャンネルのモノフォニック演奏でコードトーンを高速サイクルする
チップチューン定番サウンドを実現する。

### 設計概要

- `ArpeggioConfig` を `WaveformConfig` / `AnalogConfig` に埋め込む（`ModulationConfig` と同様）。
- Voice 状態として `arpStep` と `arpElapsedSec` を追加する（`WaveformVoiceState` / `AnalogVoiceState`）。
- Renderer の source render ブロック内で、`pitchMul` に半音オフセット倍率を乗算する。

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/SynthEngine.h` | `ArpeggioConfig` struct 追加、`WaveformConfig` / `AnalogConfig` に埋め込み |
| `src/SynthEngine/Internal.h` | `WaveformVoiceState` / `AnalogVoiceState` に `arpStep` / `arpElapsedSec` 追加 |
| `src/SynthEngine/Renderer.cpp` | waveform / analog render ブロックで arp ステップ進行と pitchMul 適用 |
| `src/SynthEngine/Voices.cpp` | NoteOn 時に `arpStep=0, arpElapsedSec=0.0` リセット |
| `src/config/load/LoadSource.cpp` | `"arpeggio"` オブジェクトのロード追加 |
| `src/config/ConfigJSONUtils.cpp` | `arpeggio` の書き出し追加 |
| `src/gui/GUIChannelEditor.cpp` | arpeggio 編集 UI 追加 |

### 実装手順

#### Step 1: SynthEngine.h — ArpeggioConfig 追加

```cpp
struct ArpeggioConfig
{
    bool enabled = false;
    double rateHz = 10.0;                                    // ステップ切り替え速度
    int steps = 3;                                           // 有効ステップ数（1..8）
    std::array<int, 8> semitones{0, 4, 7, 12, 0, 0, 0, 0}; // ルートからの半音オフセット
};
```

`WaveformConfig` と `AnalogConfig` に追加:

```cpp
ArpeggioConfig arpeggio{};
```

#### Step 2: Internal.h — voice state に arp フィールド追加

`WaveformVoiceState` と `AnalogVoiceState` に:

```cpp
int    arpStep       = 0;
double arpElapsedSec = 0.0;
```

#### Step 3: Voices.cpp — NoteOn 時にリセット

waveform NoteOn 初期化（`voices.sourceState[i] = WaveformVoiceState{}` の直後）に:

```cpp
ws.arpStep = 0;
ws.arpElapsedSec = 0.0;
```

analog も同様に `as.arpStep / as.arpElapsedSec` をリセットする。

#### Step 4: Renderer.cpp — arp ステップ進行と pitchMul 乗算

waveform render ブロックで `pitchMul` が確定した直後（`const double phaseInc = ...` の手前）に:

```cpp
if (src.arpeggio.enabled && src.arpeggio.steps > 0)
{
    ws.arpElapsedSec += in.dt;
    const int steps = std::clamp(src.arpeggio.steps, 1, 8);
    const double stepDur = 1.0 / std::max(0.5, src.arpeggio.rateHz);
    while (ws.arpElapsedSec >= stepDur)
    {
        ws.arpElapsedSec -= stepDur;
        ws.arpStep = (ws.arpStep + 1) % steps;
    }
    const int semitoneOffset = src.arpeggio.semitones[ws.arpStep];
    pitchMul *= std::pow(2.0, semitoneOffset / 12.0);
}
```

`in.dt` は `EvaluateModulation` へ渡している `deltaTimeSec` 相当の変数（既存 render ブロックで確認する）。

analog render ブロックも同じパターンで `as.arpStep / as.arpElapsedSec` を使って実装する。

#### Step 5: ConfigLoad

`waveform` / `analog` ロード関数に `"arpeggio"` オブジェクトを追加:

```cpp
if (auto arpText = FindJSONObject(sourceText, "arpeggio"))
{
    if (auto v = ReadJSONBool(*arpText, "enabled"))     cfg.arpeggio.enabled = *v;
    if (auto v = ReadJSONDouble(*arpText, "rateHz"))    cfg.arpeggio.rateHz  = std::clamp(*v, 0.5, 100.0);
    if (auto v = ReadJSONInt(*arpText, "steps"))        cfg.arpeggio.steps   = std::clamp(*v, 1, 8);
    // semitones: 最大8要素の JSON array を読む（既存の配列読み取り関数があれば流用）
}
```

#### Step 6: ConfigJSONUtils — 書き出し

```cpp
WriteIndent(out, N); out << "\"arpeggio\": {\n";
WriteIndent(out, N+1); out << "\"enabled\": " << (cfg.arpeggio.enabled ? "true" : "false") << ",\n";
WriteIndent(out, N+1); out << "\"rateHz\": " << cfg.arpeggio.rateHz << ",\n";
WriteIndent(out, N+1); out << "\"steps\": " << cfg.arpeggio.steps << ",\n";
WriteIndent(out, N+1); out << "\"semitones\": [";
for (int k = 0; k < 8; k++) { out << cfg.arpeggio.semitones[k]; if (k < 7) out << ", "; }
out << "]\n";
WriteIndent(out, N); out << "},\n";
```

#### Step 7: GUI

waveform / analog 編集ブロックの末尾（modulation の直前あたり）に:

```cpp
if (ImGui::CollapsingHeader("Arpeggio"))
{
    ImGui::Checkbox("Enabled##arp", &src->arpeggio.enabled);
    ImGui::SliderFloat("Rate Hz##arp", &src->arpeggio.rateHz, 0.5f, 40.0f);
    ImGui::SliderInt("Steps##arp",    &src->arpeggio.steps,   1, 8);
    for (int k = 0; k < src->arpeggio.steps; k++)
    {
        char label[16];
        std::snprintf(label, sizeof(label), "Note %d##arp", k);
        ImGui::SliderInt(label, &src->arpeggio.semitones[k], -24, 24);
    }
}
```

### 受け入れ条件

- `enabled=true, rateHz=10, steps=3, semitones=[0,4,7]`: 単音を弾くと C→E→G→C→... が高速サイクルする。
- `steps=1, semitones=[0]`: 単音のまま（arp 無効相当）。
- NoteOn のたびにステップ 0 から開始する。
- `check.ps1` 通過。

---

## Tier D-2: Hard Sync

### 目的

Waveform / Analog ソースにオシレータハードシンクを追加し、
「マスター発振器が 1 周期完了するたびにスレーブ発振器をリセット」することで
エイリアシング的に豊かな倍音を生成する。

### 設計概要

- マスター発振器: `syncPhase`（`baseFreq * hardSyncRatio` で進む、`WaveformVoiceState` に追加）
- スレーブ発振器: 既存の `voices.phase[i]`
- 検出: `syncPhase` のラップアラウンドを検出 → `voices.phase[i] = 0.0` にリセット

### 変更ファイル

| ファイル | 変更種別 |
|---|---|
| `include/SynthEngine/SynthEngine.h` | `WaveformConfig` / `AnalogConfig` に `hardSyncEnabled` / `hardSyncRatio` 追加 |
| `src/SynthEngine/Internal.h` | `WaveformVoiceState` / `AnalogVoiceState` に `syncPhase` 追加 |
| `src/SynthEngine/Renderer.cpp` | waveform / analog render ブロックで sync 検出・リセット追加 |
| `src/SynthEngine/Voices.cpp` | NoteOn 時に `syncPhase = 0.0` リセット |
| `src/config/load/LoadSource.cpp` | `"hardSyncEnabled"` / `"hardSyncRatio"` ロード追加 |
| `src/config/ConfigJSONUtils.cpp` | 書き出し追加 |
| `src/gui/GUIChannelEditor.cpp` | 編集 UI 追加 |

### 実装手順

#### Step 1: SynthEngine.h — フィールド追加

`WaveformConfig` / `AnalogConfig` に:

```cpp
bool hardSyncEnabled = false;
double hardSyncRatio = 2.0;   // マスター周波数 = baseFreq * ratio (0.5..8.0)
```

#### Step 2: Internal.h — syncPhase 追加

`WaveformVoiceState` / `AnalogVoiceState` に:

```cpp
double syncPhase = 0.0;
```

#### Step 3: Voices.cpp — NoteOn リセット

```cpp
ws.syncPhase = 0.0;
// analog: as.syncPhase = 0.0;
```

#### Step 4: Renderer.cpp — sync 検出・リセット

waveform render ブロック内で `voices.phase[i]` が更新される行を探す
（通常は `voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc)` の形）。
その行を以下に差し替える:

```cpp
if (src.hardSyncEnabled)
{
    const double syncInc = phaseInc * std::clamp(src.hardSyncRatio, 0.5, 8.0);
    const double prevSync = ws.syncPhase;
    ws.syncPhase = WrapPhase(ws.syncPhase + syncInc);
    if (ws.syncPhase < prevSync)
    {
        // マスターが 1 周期完了 → スレーブをリセット
        voices.phase[i] = 0.0;
    }
    else
    {
        voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
    }
}
else
{
    voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
}
```

`WrapPhase` は Renderer.cpp 内で定義済み（約 61 行目）。

analog render ブロックも同じパターンで `as.syncPhase` を使って実装する。

#### Step 5: ConfigLoad / ConfigJSONUtils / GUI

既存の `subOscLevel` の追加パターンに倣って追加する。

GUI:

```cpp
ImGui::Checkbox("Hard Sync", &src->hardSyncEnabled);
if (src->hardSyncEnabled)
{
    ImGui::SliderFloat("Sync Ratio", &src->hardSyncRatio, 0.5f, 8.0f);
    updateHoverHelp(
        "マスター発振器の周波数比。高いほど倍音が密になります。",
        "Sync Ratio=2.0: マスターが 2 倍速で回転し、スレーブを 2 回リセットします。",
        "");
}
```

### 受け入れ条件

- `hardSyncEnabled=true, hardSyncRatio=2.0, wave=saw`: 通常 saw より倍音が密で、
  フィルタスイープと組み合わせると金属的な音色になる。
- `hardSyncRatio=1.0`: リセットが 1 周期ごとで通常 saw と同等（事実上 sync なし）。
- `hardSyncEnabled=false` で既存と完全に同等の音。
- `check.ps1` 通過。

---

## タスク完了時の処置

各タスク完了後:
1. `docs/STATUS.md` の `Current` を更新する。
2. 判断事項があれば `docs/DECISIONS.md` に追記する。
3. 全タスク完了後、本ファイル（`next-features.md`）を削除する。
