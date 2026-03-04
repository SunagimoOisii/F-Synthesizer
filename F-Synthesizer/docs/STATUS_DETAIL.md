# STATUS_DETAIL

Last Updated: 2026-03-04 (Phase 6 + doc-sync)
Branch: `main`
Migration Progress: `GUI v7: DONE(FROZEN) / GUI v8: DONE`

## Current Snapshot

- 実装済み
  - MIDI 解析（Note/Tempo/CC/Pitch Bend）
  - tick -> sample 変換
  - SynthEngine 分割（`Engine.cpp` / `Events.cpp` / `Renderer.cpp` / `Voices.cpp`）
  - 音源（Waveform / Noise / FM / Drum / DrumKit）
  - ADSR と WAV 書き出し
  - 入力MIDIを `assets/midi/` から読む運用に変更
- 直近の大きな変更
  - `config/base.json` + `presets/*.json` + `--preset` 導入、`Run(const AppConfig&)` に実行コアを分離
  - 既定起動を GUI 化（`--cli` でCLI強制）
  - GUI v0.2-v0.5: チャンネル編集、プリセット運用、日本語パス対策、ミックス基盤、Preview再生、SoA化、レイアウト再構築
  - Refactor Phase 1-13: 責務分割（app/config/midi/core/gui/pianoroll）、ランタイム追跡整理
  - Comment Migration Phase 1-5: 高リスク箇所への日本語コメント追加、用語ルール導入
  - ピアノロール Phase 1: 表示基盤（グリッド/鍵盤/ノート描画、ch切替、ズーム/スクロール）
  - GUI v7: Musicタブ16chミキサー、書き出し対象UI、未保存警告、エラーUX
  - GUI v8: Soundタブ縮退、保存導線整理、ドラム整理、エラー導線統一
  - Source設計改善: `config/SourceRegistry` 導入、プリセット保存統一、ConfigLoad registry移行
  - Waveform Phase A: band-limited（polyBLEP）生成導入、`quality` 設定追加
  - Modulation基盤: LFO1/Env2/Route の load/save/GUI編集、低コストスキップ
  - Parameter Smoothing: 共通モジュール追加、Waveform経路接続（Noise/FM/Drumは未接続で方針固定）
  - Wave/Drum Brushup: preset再調整、ドラムレベル抑制、帯域再配分、AB手順固定
  - GUI UX #3: DrumKit Type 連動パラメータ表示（Kick / Snare・Hat / None で表示切替）
  - GUI UX #5: WaveformConfig FilterMode デフォルトを Bypass に変更、フィルタ有効時の `(active)` インジケーター追加
  - GUI UX #6: モジュレーションルートを CollapsingHeader 化、ヘッダに "lfo1 -> pitch (+0.50)" 形式のサマリを表示
  - GUI UX #7: `Channel(ch)` と `Sound Slot(s)` の表記を整理。Musicプレビュー時に `Selected Sound Slot` が自動変更されないよう挙動を修正
  - SynthEngine #2: `noteInstanceId` を `MIDIParser -> Sequencer -> VoicesSoA` へ通線し、`MarkNoteOff` をID優先照合へ切替（不一致時は旧 `ch+note` へフォールバック）
  - SynthEngine #3: Filter cutoff の微小更新を間引き、係数再計算のホットパスCPU負荷を低減（1 cent相当未満をスキップ）
  - cleanup Phase 1-6: `scripts`/`docs`/`gui`/`midi`/`core` を監査し、削除方針を `docs/cleanup/deletion-policy.md` に確定。`scripts/piano_roll_smoke.ps1` を削除し、`AudioBuffer.cpp` の未使用/冗長処理を整理
  - doc-sync: `ROADMAP.md` を廃止して `STATUS.md` へ一本化。`PIANO_ROLL_CONTROLS.md` は `GUI_REQUIREMENTS.md` へ、`WEEKLY_MAINTENANCE.md` は `OPERATIONS.md` へ統合
- 品質確認
  - `Debug x64` ビルド成功（2026-02-21）
  - `scripts/gui_smoke.ps1` 15ステップ通過（2026-02-23）
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-03）
  - `scripts/midi_regression.ps1` 通過（2026-03-03、running status 2件 + overlap same note 1件）
  - Modulation / Smoothing / Waveform / Wave+Drum AB確認済み（2026-02-23、全て clip 0）

## Backlog

- `feat-infra`: `scripts/midi_regression.ps1` の実行ディレクトリ依存を解消し、`check.ps1` 統合オプションを追加する
- `refactor`: `module-map.md` の自動生成 TODO（背景/判断/影響範囲）を記入する
- `gui-cleanup`: DrumConfig の `0 = 未指定（内部デフォルト）` を UI で明示する
- `comments`: 既存ファイルへのコメント追加（高リスク箇所を優先、`COMMENT_GUIDELINE` 準拠）
- `gui-help`: ホバーUIヘルプ対象を拡大（誤操作が出やすい項目を優先）

## Recurring Checks

- `doc-sync`: 実装変更後に `Architecture.md` / `README.md` / `STATUS.md` の差分同期を確認する

## Notes

- 詳細設計は `Architecture.md`
- プロダクト方針は `PRODUCT_POLICY.md`
- 設計判断ログは `DECISIONS.md`
- 音色パラメータ要約は `SOUND_PARAMETERS_SUMMARY.md`（詳細は `SOUND_PARAMETERS.md`）
- 実行入口は `src/SoundGenerate.cpp`
- GUI準備要件は `GUI_REQUIREMENTS.md`
- コメント規約は `agents/standards/COMMENT_GUIDELINE.md`
- GUIスモークテストは `scripts/gui_smoke.ps1`
- 再開手順は `AGENTS.md` の Start Here / Useful Commands を参照
- 過去バージョン計画・移行記録は `docs-archive/` 配下を参照
