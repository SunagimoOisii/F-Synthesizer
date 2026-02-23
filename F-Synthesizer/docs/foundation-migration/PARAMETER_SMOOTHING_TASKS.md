# PARAMETER_SMOOTHING_TASKS

このドキュメントは `docs/foundation-migration/PARAMETER_SMOOTHING_PHASES.md` の実装チェックリストです。

## 運用ルール

- 1コミット1目的を維持する（基盤/接続/GUI/検証を分離）。
- 各Phase完了時に以下を更新:
  - `docs/synth-methods/integration-playbook.md`
  - `docs/synth-methods/waveform.md`（接続済み方式の記録）
  - `docs/STATUS.md`

## Phase A: 基盤仕様固定

- [x] Smoothing方式を `one-pole` で固定
- [x] `timeMs` の意味（0=バイパス）を明文化
- [x] パラメータ別の初期推奨値を定義
- [x] 値域クランプ/異常値方針を定義

## Phase B: 共通モジュール実装

- [x] 共通ヘッダ/実装ファイルを追加（Smoothing module）
- [x] `SmoothedParam` を実装（`current/target/coeff`）
- [x] `Reset` / `SetTarget` / `Step` / `SetTimeMs` を実装
- [x] `sampleRate` 変更時の係数再計算を実装
- [x] `timeMs=0` の低コストバイパスを実装

## Phase C: Voice/Renderer統合（第1段）

- [x] `VoicesSoA` に smoothing state を追加
- [x] voice追加/再利用/削除時の初期化を実装
- [x] `filterCutoff` 接続
- [x] `amp` 接続
- [x] `pitch` 接続（feature flag: `FSYNTH_WAVE_PITCH_SMOOTHING`）
- [x] ON/OFF でノイズ差分を確認（`FSYNTH_WAVE_SMOOTHING=0/1` 比較）

## Phase D: Config I/O / GUI接続

- [x] Configモデルへ smoothing 設定を追加
- [x] `ConfigLoad` 読込 + バリデーション
- [x] `ConfigJsonUtils` 書込
- [x] GUI編集UIを追加
- [x] `GUIConfigUtils` 比較更新（preset diff）
- [x] GUI保存 -> 再読込一致を確認

## Phase E: 横展開と方式別接続

- [x] Noise への接続可否を判断・記録
- [x] FM への接続可否を判断・記録
- [x] Drum/DrumKit への接続可否を判断・記録
- [x] 将来方式（減算/加算/PSG/PCM）への接続テンプレート作成
- [x] `integration-playbook.md` へ接続状態を反映

## Phase F: AB検証と運用固定

- [x] AB用MIDIセットを確定（`assets/midi/smoothing_phaseF_ab.mid`）
- [x] ON/OFF 比較WAV生成手順を固定（`config/samples/phaseF_smoothing_off.json` / `phaseF_smoothing_on.json`）
- [x] peak/rms/clip の自動取得手順を固定（`py -3` でWAV解析）
- [x] 極端設定テスト（0ms/長時定数/高速連打）を実施
- [x] 結果を `STATUS.md` に反映

## コミット粒度（推奨）

1. `Phase A/B`: 仕様 + 共通モジュール追加
2. `Phase C`: Renderer/Voices接続（音が変わる差分）
3. `Phase D`: Config/GUI接続
4. `Phase E/F`: 横展開 + AB結果 + ドキュメント更新
