# COMMENT_MIGRATION_BASELINE

最終更新: 2026-02-21  
対象: `src/` + `include/`（`include/third_party/*` を除く）

この文書は `COMMENT_PHASE_1_BASELINE` の成果物です。  
コメント追加作業の「対象一覧」「優先順位」「必須コメント抽出」「レビュー基準」を固定します。

## 1. 優先順位

1. `core`（`src/SynthEngine/*`, `src/core/*`, `include/SynthEngine/*`, `include/core/*`）
2. `midi`（`src/midi/*`, `include/midi/*`, `include/MIDIParser.h`, `include/Sequencer.h`）
3. `app/config`（`src/app/*`, `src/config/*`, `include/AppCore.h`, `include/app/*`, `include/config/*`）
4. `gui/io`（`src/gui/*`, `src/io/*`, `src/GUIMain.cpp`, `include/gui/*`, `include/io/*`）
5. その他基盤（`src/AudioBuffer.cpp`, `src/Envelope.cpp`, `src/Oscillator.cpp`, `src/Writer.cpp` など）

## 2. 対象ファイル一覧（Phase単位）

### Phase 2 対象（Core/MIDI）

- `src/SynthEngine/Engine.cpp`
- `src/SynthEngine/Events.cpp`
- `src/SynthEngine/Renderer.cpp`
- `src/SynthEngine/Voices.cpp`
- `src/SynthEngine/Internal.h`
- `src/core/RenderGateway.cpp`
- `src/midi/MIDIParser.cpp`
- `src/midi/MidiPipeline.cpp`
- `src/midi/Sequencer.cpp`
- `include/SynthEngine/SynthEngine.h`
- `include/core/RenderGateway.h`
- `include/midi/MidiPipeline.h`
- `include/MIDIParser.h`
- `include/Sequencer.h`

### Phase 3 対象（App/Config）

- `src/app/AppEntry.cpp`
- `src/app/Cli.cpp`
- `src/app/RunDefaults.cpp`
- `src/app/RunExecution.cpp`
- `src/app/RunSave.cpp`
- `src/app/RunStats.cpp`
- `src/app/RunInternal.h`
- `src/config/ConfigFileIO.cpp`
- `src/config/ConfigLoad.cpp`
- `src/config/ConfigJsonUtils.cpp`
- `src/config/ConfigResolver.cpp`
- `src/config/ConfigSave.cpp`
- `src/config/ConfigFileInternal.h`
- `include/AppCore.h`
- `include/app/AppEntry.h`
- `include/app/Cli.h`
- `include/config/ConfigResolver.h`

### Phase 4 対象（GUI/IO）

- `src/GUIMain.cpp`
- `src/gui/GUIActions.cpp`
- `src/gui/GUIChannelEditor.cpp`
- `src/gui/GUIConfigUtils.cpp`
- `src/gui/GUIPlatform.cpp`
- `src/gui/GUIPresetIO.cpp`
- `src/gui/GUIRunHelpers.cpp`
- `src/gui/GUIStateModel.cpp`
- `src/gui/GUIStatePersistence.cpp`
- `src/gui/GUIStateStorage.cpp`
- `src/gui/PreviewAudio.cpp`
- `src/io/PlatformPaths.cpp`
- `include/gui/GUIActions.h`
- `include/gui/GUIChannelEditor.h`
- `include/gui/GUIConfigUtils.h`
- `include/gui/GUIPlatform.h`
- `include/gui/GUIPresetIO.h`
- `include/gui/GUIRunHelpers.h`
- `include/gui/GUIState.h`
- `include/gui/GUIStateModel.h`
- `include/gui/GUIStatePersistence.h`
- `include/gui/GUIStateStorage.h`
- `include/gui/PreviewAudio.h`
- `include/io/PlatformPaths.h`

## 3. 必須コメント対象（抽出）

以下は規約上の「修正必須」対象です。

1. 複雑分岐
- 早期 return が多段になる実行制御
- エラー経路と正常経路が交差する分岐
- GUI の状態遷移（実行中/停止/プレビュー/保存）

2. 最適化コード
- head-index による O(1) 運用
- 事前計算 / 分岐削減 / 使い回しバッファ
- SoA ホットパス（Voice 配列アクセス）

3. ファイル/型概要
- 1ファイル1責務の意図
- 公開I/F型（`AppConfig`, `RenderOptions`, `IRunObserver` など）
- 境界型（GUI状態、Config中間表現、I/O診断構造）

## 4. レビュー判定基準（Phase 1確定）

### 4.1 修正必須（Fail）

- 規約上必須の場所にコメントが無い
- 最適化コードで「目的/前提/トレードオフ」のいずれかが欠ける
- `TODO` / `FIXME` を新規追加している
- コメント追加コミットにロジック変更が混在している

### 4.2 修正推奨（Warn）

- 「なぜ」が不足し、「何をしているか」だけの説明に偏る
- 用語ゆれで同一概念が追いにくい
- 1関数内のコメント密度が過剰で読解が逆に遅くなる

### 4.3 受け入れ条件（Pass）

- 対象範囲の必須箇所を第三者が追える
- 重要分岐・最適化の意図が、実装を1行ずつ追わずに把握できる
- 規約違反（Fail）が 0 件
