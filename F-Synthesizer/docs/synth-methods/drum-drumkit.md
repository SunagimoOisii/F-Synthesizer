# Drum / DrumKit

最終更新: 2026-03-19
状態: 実装済み
source type: `drum`, `drumkit`

## A. 現状の実装内容

- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
  - `src/SynthEngine/Voices.cpp`
  - `src/SynthEngine/Events.cpp`
- 実行経路:
  - `drum`: 単体ドラム合成（kick/snare/hat）
  - `drumkit`: note -> `DrumConfig` へ展開して通常ボイスとして投入
- 現状仕様:
  - One-shot 前提の自動リリース挙動
  - ノイズ + 簡易フィルタ（snare/hat）を使用

## B. 作れる音 / 向いている用途

- 音のキャラクター:
  - Kick/Snare/Hat のワンショット合成
- 得意用途:
  - リズムの迅速な試作
  - 電子的なドラム基盤
- 苦手用途:
  - 幅広いアコースティック再現

## C. 問題点

- 楽器バリエーションが小さい
- `drumType` ごとにパラメータ意味が変わり、把握しづらい
- Parameter Smoothing は非適用（one-shot アタック保護）

## D. 改善案

- 短期:
  - `drumType` 別の既定挙動を文書化
  - プリセット拡充
- 中期:
  - モデル追加（tom/clap/cymbal など）
  - パラメータ UI の文脈表示
  - one-shot アタックを壊さない限定的 smoothing（リリース帯域のみ）を再検討
- 長期:
  - ドラム専用モジュール化（共通エンベロープ/フィルタ設計）

## I. Smoothing 契約整合（2026-03-19）

- 適用状況:
  - `未適用`
- 非適用理由:
  - drum/drumkit は one-shot 前提で、立ち上がりの遅延はアタック破壊に直結する。
  - 現行実装は `attack+decay` 到達で自動 release するため、source smoothing の混在は寿命設計を崩しやすい。
- 再検討条件:
  - attack 帯域へ影響しない設計（例: release 帯域限定）を先に仕様化できること。
  - `wave_drum_phaseC_ab.mid` など代表MIDIで AB（peak/rms/clip + 聴感）を通過し、アタック遅れがないこと。

## E. 保守 / 拡張メモ

- `drumkit` は差分保存時の項目欠落を避けるため共通 JSON writer を必ず使う
- note map 拡張時は `Events` 側の展開経路とセットで確認する

## F. Wave/Drum Brushup Phase C

- 追加プリセット:
  - `config/presets/drumkit_nesgb_core.json`
- 検証MIDI:
  - `assets/midi/wave_drum_phaseC_ab.mid`
- 反映方針:
  - ドラム側の `channel amp` と `map gain` を基準レンジへ抑制
  - one-shot アタックを崩す処理は導入せず、レベル設計で改善

## G. Wave/Drum Brushup Phase D

- 統合AB設定:
  - `config/samples/phaseD_wave_drum_integrated_before.json`
  - `config/samples/phaseD_wave_drum_integrated_after.json`
- 結果（同一MIDI, waveform+drum 同居）:
  - before: peak `0.277023`, rms `0.044319`, clip `0`
  - after: peak `0.175695`, rms `0.0313929`, clip `0`
- 調整ポイント:
  - kick: `baseFreq` を下げて低域土台を担当、`toneFreq/toneLevel` でアタックを補強
  - snare/hat: `hpCut/lpCut/noiseLevel` を再配分し、lead帯域との衝突を抑制
  - channel mix の `level/pan/gain` を固定し、書き出しヘッドルームを確保

