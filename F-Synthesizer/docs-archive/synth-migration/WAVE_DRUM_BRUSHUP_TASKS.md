# WAVE_DRUM_BRUSHUP_TASKS

このドキュメントは `docs-archive/synth-migration/WAVE_DRUM_BRUSHUP_PHASES.md` の実装チェックリストです。

## 運用ルール

- 1コミット1目的を維持する。
- AB比較は同一MIDI・同一長さ・同一sampleRateで実施する。
- 各Phase完了時に以下を更新:
  - `docs/synth-methods/waveform.md`
  - `docs/synth-methods/drum-drumkit.md`
  - `docs/synth-methods/integration-playbook.md`
  - `docs/STATUS.md`

## Phase A: 品質基準固定

- [x] waveform/drum の評価観点を定義
- [x] peak/rms/clip/diff の基準を定義
- [x] AB判定ルール（聴感 + 数値）を定義
- [x] テストMIDIの役割分離（音色用/ミックス用）を定義

## Phase B: Waveformブラッシュアップ

- [x] bassプリセットを2種追加
- [x] leadプリセットを2種追加
- [x] filter/modulation/smoothing の推奨レンジを文書化
- [x] 過大detune/subでの破綻を抑制
- [x] AB比較で改善確認（clip 0）

## Phase C: Drumブラッシュアップ

- [x] kick基準プリセットを追加
- [x] snare基準プリセットを追加
- [x] hat基準プリセットを追加
- [x] ドラム音量暴れ対策（基準レベル）を反映
- [x] waveform同居時のAB比較で改善確認

## Phase D: 統合チューニング

- [x] waveform+drum の統合検証MIDIを固定
- [x] 低域衝突（kick/bass）を調整
- [x] 中高域衝突（snare/lead）を調整
- [x] ミックス基準（level/pan/gain）を固定
- [x] 書き出しヘッドルーム基準を固定

## Phase E: 運用固定

- [x] AB比較の実行コマンドを固定
- [x] 結果記録テンプレートを作成
- [x] `waveform.md` 更新
- [x] `drum-drumkit.md` 更新
- [x] `STATUS.md` 更新

## コミット粒度（推奨）

1. `Phase A`: 基準定義とテスト資産準備
2. `Phase B`: waveformのみの改善
3. `Phase C`: drumのみの改善
4. `Phase D`: 統合チューニング
5. `Phase E`: 運用固定とドキュメント更新

