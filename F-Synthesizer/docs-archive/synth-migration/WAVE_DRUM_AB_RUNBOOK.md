# WAVE_DRUM_AB_RUNBOOK

最終更新: 2026-02-23  
対象: Wave/Drum Brushup の AB 比較運用（Phase E）

## 1. 目的

- 同一条件で再現可能な AB 比較を固定する。
- 1人開発でも結果が追跡できる記録フォーマットを固定する。

## 2. 前提

- 実行ディレクトリ: `F-Synthesizer/`
- 実行バイナリ: `build/x64/Debug/F-Synthesizer.exe`
- 比較条件:
  - 同一MIDI
  - 同一sampleRate
  - 同一bits
  - 同一initialSeconds / extraReleaseSec

## 3. 実行コマンド（固定）

```powershell
# Phase D 統合 AB（waveform + drum）
.\build\x64\Debug\F-Synthesizer.exe --cli --config config/samples/phaseD_wave_drum_integrated_before.json
.\build\x64\Debug\F-Synthesizer.exe --cli --config config/samples/phaseD_wave_drum_integrated_after.json

# DrumKit 基準プリセット確認
.\build\x64\Debug\F-Synthesizer.exe --cli --preset drumkit_basic
```

## 4. 記録テンプレート（固定）

| Date | Phase | Pair | MIDI | Peak Before | Peak After | RMS Before | RMS After | Clip Before | Clip After | Diff RMS | 聴感メモ |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| 2026-02-23 | D | integrated before/after | `assets/midi/wave_drum_phaseC_ab.mid` | 0.277023 | 0.175695 | 0.044319 | 0.0313929 | 0 | 0 | - | low-end衝突低減、中高域の抜け改善 |

## 5. 判定ルール

- clip は常に `0`（必須）
- 統合ABは peak が `0.25` 付近以下を目安にヘッドルームを確保
- 採用判定は数値 + 聴感メモの両方で行う
