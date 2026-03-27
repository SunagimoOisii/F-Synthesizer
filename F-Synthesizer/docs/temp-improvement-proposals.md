# 改善案（優先度 1〜4）（一時メモ）

> **破棄前提** — 採否判断後は削除する。

---

## 優先度 1: CONFIG — JSON パーサ置換 + LoadSource 分割

### 現状の問題

| ファイル | 行数 | 問題 |
|----------|------|------|
| `LoadSource.cpp` | ~1,100 | パース / バリデーション / スキーマ検証が混在 |
| `ConfigJSONUtils.cpp` | ~805 | `std::regex` ベースの自前 JSON 読み書き |

- `std::regex` による JSON 抽出は「フィールド名が重複するとどちらが拾われるか不定」「文字列値内のエスケープを完全には処理できない」など構造上の限界がある
- 新ソース追加のたびに `LoadSource.cpp` の分岐・`ConfigJSONUtils.cpp` の `WriteSourceConfig` の両方を修正しなければならない

### 改善案 A: nlohmann/json 導入 + ファイル分割

#### ステップ 1: nlohmann/json をサブモジュールまたは single-header で追加

```
F-Synthesizer/external/nlohmann/json.hpp   ← 追加
```

影響範囲は `ConfigFileInternal.h` のみ。`ReadJSONString / ReadJSONInt / ...` 系を json オブジェクト経由に置き換える。

#### ステップ 2: ConfigJSONUtils.cpp の責務を整理

現状の責務を 3 本に分ける（ファイル名は変更せず、内部 namespace で分離してもよい）:

| 責務 | 具体的な内容 |
|------|------------|
| **列挙型 ↔ 文字列変換** | `TryParse*` / `*ToString` 関数群 — 現状のまま残す |
| **JSON 読み書き共通** | `ReadTextFile`, `ExtractObjectAt` 等 — nlohmann 導入後は削減可能 |
| **JSON 書き出し** | `WriteSourceConfig`, `WriteModulationConfig` 等 — 次ステップで分割 |

#### ステップ 3: LoadSource.cpp を 4 ファイルに分割

```
src/config/load/
  LoadSource.cpp          ← エントリ: SourceConfig に dispatch するだけ (~80行)
  LoadSourceWaveform.cpp  ← WaveformConfig / AnalogConfig パース (~200行)
  LoadSourceFm.cpp        ← FmConfig パース (~150行)
  LoadSourceDrum.cpp      ← DrumConfig / DrumKitConfig パース (~200行)
  LoadSourceNoise.cpp     ← NoiseConfig / PsgConfig パース (~100行)
  LoadSourceCommon.cpp    ← ValidateLifecycleContract / ValidateSchemaRange 等の共通処理
```

`Internal.h` に共通型・前方宣言だけ残し、各ファイルが必要な分だけインクルードする。

#### 効果
- `LoadSource.cpp` を 1,100 行 → 80 行に削減
- 正規 JSON パーサにより、ネスト構造・配列・エスケープの処理が保証される
- 新ソース追加時のファイル = 1 本の `LoadSourceXxx.cpp` 追加のみ

#### リスクと対策
- nlohmann/json はヘッダオンリーで追加依存がなく、ライセンスは MIT — 受け入れやすい
- 既存の `std::regex` ベースのテスト（smoke）が通っていれば移行後も同一の JSON を入力として検証可能
- `ConfigJSONUtils.cpp` の `Write*` 関数群は nlohmann の `dump()` に移行すると出力フォーマットが変わる可能性があるため、**読み書きのラウンドトリップテスト**を事前に整備してから着手する

---

## 優先度 2: GUI — GUIState 分割 + GUIChannelEditor モジュール化

### 現状の問題

| ファイル | 行数 | 問題 |
|----------|------|------|
| `GUIState.h` | 99 | 50+ フィールドが 1 構造体に集約（God Object） |
| `GUIChannelEditor.cpp` | 988 | ソース種別ごとの UI ロジックが縦に連なる |
| `GUIActions.cpp` | 856 | Run 実行・プリセット・プレビューが混在 |

`GUIState` は以下の 4 つの独立した責務を同居させている:
1. **実行パラメータ** (midiPath, wavPath, sampleRate, bits ...)
2. **非同期 Run 状態** (runFuture, logMutex, observer, stopRequested ...)
3. **プレビュー状態** (previewRenderedSound, playback, autoTonePreview ...)
4. **UI 表示状態** (UIModeTab, UIScaleIndex, selectedSoundSlot ...)

### 改善案 B: GUIState を責務別に分割

#### ステップ 1: 責務ごとに sub-struct を切り出す

```cpp
// 既存の GUIState.h をそのまま維持しつつ、内部を sub-struct に整理
struct GUIRunState              // 非同期 Run の状態
{
    std::future<int> runFuture{};
    std::mutex logMutex{};
    std::vector<std::string> soundLogs{};
    std::vector<std::string> musicLogs{};
    std::atomic<bool> stopRequested{ false };
    bool running = false;
    bool hasRun = false;
    int lastRunExitCode = 0;
    GUIRunObserver observer{};
};

struct GUIPreviewState          // プレビュー再生の状態
{
    bool soloPreviewActive = false;
    bool restorePreviewOnRunComplete = false;
    int soloPreviewChannel = 0;
    std::array<ChannelMixState, 16> soloPreviewBackup{};
    bool previewLoop = false;
    bool autoTonePreviewEnabled = false;
    bool autoTonePreviewPending = false;
    double autoTonePreviewLastEditSec = 0.0;
    bool previewAudioReady = false;
    bool runIsPreview = false;
    bool autoPlayPreviewOnRunComplete = false;
    int previewRequestedStartTick = 0;
    double previewRequestedDurationSec = 0.0;
    std::shared_ptr<SoundData> previewRenderedSound{};
    std::shared_ptr<SoundData> runOutputBuffer{};
    PreviewPlaybackState playback{};
};

struct GUIRenderParams          // 実行パラメータ
{
    char midiPath[1024]{};
    char wavPath[1024]{};
    int targetChannel = -1;
    int sampleRate = 44100;
    int initialSeconds = 6;
    int bits = 16;
    float extraReleaseSec = 0.3f;
    MasterEffectConfig masterEffects{};
};

struct GUIState                 // 統合コンテナ（フィールド数を大幅削減）
{
    GUIRenderParams renderParams{};
    GUIRunState run{};
    GUIPreviewState preview{};
    // UI 表示状態は残す
    int UIScaleIndex = 1;
    int UIModeTab = 0;
    ...
};
```

この変更は `GUIState` の**フィールドアクセスのパス**が変わる（例: `state.running` → `state.run.running`）ため、`GUIActions.cpp` / `GUIStateStorage.cpp` 等の修正が必要。1 回の機械的置換で完了する範囲。

#### ステップ 2: GUIChannelEditor.cpp を .inl 分割

現在 `src/gui/pianoroll/` 配下に `PianoRollRender.inl` 等が存在する — 同パターンで:

```
src/gui/channeleditor/
  ChannelEditorWaveform.inl   ← Waveform / Analog UI
  ChannelEditorFm.inl         ← FM UI
  ChannelEditorDrum.inl       ← Drum / DrumKit UI
  ChannelEditorNoise.inl      ← Noise / PSG UI
  ChannelEditorModulation.inl ← モジュレーション共通 UI
```

`GUIChannelEditor.cpp` はインクルードとディスパッチのみ (~100行) になる。

**注意**: `.inl` 方式は実態としてコンパイル単位を分割しないため、ビルド時間への影響は皆無。あくまで**読む単位を分ける**目的。

#### ステップ 3: GUIActions.cpp の責務分離

| 現状 | 分割後 |
|------|-------|
| Run 実行・キャンセル | `GUIRunActions.cpp`（既存 GUIActions 内の Run 関連） |
| プリセット保存・読み込み | `GUIPresetIO.cpp`（既存ファイルを拡充） |
| プレビュー起動・停止 | `GUIPreviewActions.cpp` |

#### 効果
- `GUIState` の認知負荷が大幅に低下（各 sub-struct は 10〜15 フィールド）
- 新機能追加時に「どの sub-struct に追加すべきか」が明確になる
- `GUIChannelEditor.cpp` が 988 行 → ~100 行に削減（中身は .inl に移動）

#### リスクと対策
- `GUIState` フィールドのアクセスパス変更は広範囲 → sed / 一括置換で対応可能
- sub-struct 化で `sizeof(GUIState)` は変わらないためメモリ影響なし

---

## 優先度 3: SynthEngine — Renderer.cpp 分割

### 現状の問題

`Renderer.cpp`（736行）はソース種別ごとのレンダリングが縦方向に並んでいる:

```
RenderVoiceSource<WaveformConfig, ...>  ~150行
RenderVoiceSource<AnalogConfig, ...>    ~100行
RenderVoiceSource<FmConfig, ...>        ~150行
RenderVoiceSource<DrumConfig, ...>      ~80行
RenderVoiceSource<DrumKitConfig, ...>   ~80行
...
+ フィルタ適用 / モジュレーション適用 / ミックス
```

関数間は `namespace {}` 内に定義された小ヘルパー（`SampleOp`, `EnsureDrumFilters`, `ArpSemitoneRatio` 等）が入り組んでいる。

### 改善案 C: Renderer を .inl 分割

pianoroll と同じパターンを踏襲する:

```
src/SynthEngine/renderer/
  RenderCommon.inl      ← VoiceRenderInput, TimeScaleFromOffset 等の共通ヘルパー
  RenderWaveform.inl    ← WaveformConfig / AnalogConfig レンダリング
  RenderFm.inl          ← FmConfig レンダリング（SampleOp 含む）
  RenderDrum.inl        ← DrumConfig / DrumKitConfig レンダリング
  RenderNoise.inl       ← NoiseConfig / PsgConfig レンダリング
  RenderMix.inl         ← ミックス / エフェクト適用
```

`Renderer.cpp` 本体はインクルードとエントリ関数のみ (~60行)。

#### ヘルパーの帰属整理

現在 `Renderer.cpp` のローカル namespace にある共通ヘルパーを適切な .inl に移動する:

| ヘルパー | 移動先 |
|----------|--------|
| `ArpSemitoneRatio` | `RenderWaveform.inl`（Waveform/Analog 専用） |
| `SampleOp` | `RenderFm.inl` |
| `EnsureDrumFilters` | `RenderDrum.inl` |
| `SourceFilterResonance` | `RenderCommon.inl` |
| `WrapPhase` | `RenderCommon.inl` |

#### Modulation 適用の分離

現在 `Renderer.cpp` 内でモジュレーション適用（`EvaluateModulation` 呼び出し後の `pitchMul` 合成等）がレンダリングループに埋め込まれている。これを `RenderMix.inl` に切り出し、`SourceRenderFrame` を受け取って後段で適用するフロー（STATUS.md に記載の `source render -> common shaper -> modulation apply -> mix`）に合わせる。

#### 効果
- `Renderer.cpp` が 736 行 → ~60 行に削減
- ソース追加時の修正ファイル = `RenderXxx.inl` 1 本のみ
- FM / Drum など互いに干渉しない部分のコードレビューが容易になる

#### リスクと対策
- `.inl` はコンパイル単位を分割しないため、**ビルド時間・リンク・最適化に影響なし**
- `Internal.h` の `Voice` / `PerSourceVoiceState` を .inl が参照できれば移行は機械的
- 移行前後で `-RunRuntimeSmoke` による音声出力比較が必須

---

## 優先度 4: MIDI — MIDIParser.cpp 責務分離

### 現状の問題

`MIDIParser.cpp`（455行）が 2 つの独立した責務を持っている:

| 責務 | 内容 |
|------|------|
| **SMF バイナリ解析** | `ReadBE32/16`, `ReadVarLen`, ヘッダ/トラック読み込み |
| **イベント構築** | ノート重複追跡, サステインペダル, `MIDIBuildOutput` 組み立て |

これらは完全に直列な処理だが、「バイナリ構造の知識」と「ゲームロジック的なイベント変換」の 2 つの異なるドメイン知識を要求するため、読む目的によって必要な前提知識が異なる。

### 改善案 D: 2 ファイルに分割

```
src/midi/
  MIDIReader.cpp      ← SMF バイナリ解析のみ（ReadBE32/16, ReadVarLen, ParseSMF）
  MIDIParser.cpp      ← イベント構築のみ（ノート追跡, サステイン, MIDIBuildOutput 組み立て）
```

#### MIDIReader.cpp の公開 API

```cpp
// 新設: SMF を読み込んで生の tick イベント列と tempo map を返す
struct MIDIRawOutput
{
    std::vector<MIDIEventTick> rawEvents;  // ソートされた全 tick イベント
    std::vector<TempoEvent> tempoEvents;
    MIDIParseStatus status;
};

MIDIRawOutput ParseSMFFile(const std::string& path);
```

#### MIDIParser.cpp の変更

現在の `BuildMIDIData(path, ...)` を `BuildMIDIData(MIDIRawOutput&&, ...)` に変更し、バイナリ I/O から切り離す。

#### 効果
- SMF 解析部は単体テスト可能になる（バイナリファイルを用意して `MIDIRawOutput` の内容を検証）
- イベント構築部は `MIDIRawOutput` をモックして独立テスト可能
- 「MIDI ファイルが壊れているのか / ノート変換が壊れているのか」の切り分けが容易になる

#### リスクと対策
- 変更量は小さい（455行 → ~220行 + ~230行）
- `MIDIPipeline.cpp` が呼び出す `BuildMIDIData` のシグネチャ変更が必要
- バイナリパーサはステートフル（`std::ifstream` を引き回す）なので、インタフェースを `ParseSMFFile` という関数呼び出し型にまとめることで外部への露出を最小化できる

---

## 全体俯瞰

| 優先度 | 対象 | 変更の種類 | 修正箇所の広がり | 推定リスク |
|--------|------|----------|-----------------|-----------|
| 1 | CONFIG / JSON パーサ置換 | 外部依存追加 + 全 config ファイル | config 全体 | 中（ラウンドトリップ検証が必要） |
| 1 | CONFIG / LoadSource 分割 | ファイル分割のみ | config/load/ のみ | 低 |
| 2 | GUI / GUIState sub-struct 化 | フィールドアクセスパス変更 | GUI 全体 | 中（機械的置換で対応可） |
| 2 | GUI / GUIChannelEditor .inl 分割 | ファイル分割のみ | gui/channeleditor/ のみ | 低 |
| 3 | SynthEngine / Renderer .inl 分割 | ファイル分割のみ | SynthEngine/ のみ | 低（smoke テスト必須） |
| 4 | MIDI / MIDIParser 分割 | シグネチャ変更 + ファイル分割 | midi/ + pipeline 呼び出し箇所 | 低 |

### 着手順の推奨

1. **LoadSource 分割**（優先度 1 のうち、外部依存なしで即着手可能）
2. **Renderer .inl 分割**（smoke テストで前後比較が容易）
3. **GUIChannelEditor .inl 分割**（ビルド影響なし、視覚的効果大）
4. **MIDIParser 分割**（変更量が少なく完結しやすい）
5. **GUIState sub-struct 化**（置換範囲が広いので単独 PR で行う）
6. **nlohmann/json 導入**（ラウンドトリップ検証の準備ができてから）
