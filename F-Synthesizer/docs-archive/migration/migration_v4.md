# v0.3 -> v0.4 Migration Guide

このドキュメントは、GUI v0.3 から v0.4 へ移行する際の差分をまとめる。

## 1. 変更概要

- Preview は WAV 書き出し前提から、GUI内メモリ再生へ変更
- `Stop` は「ログのみ」から、レンダ中断要求まで実装
- SynthEngine 内部を AoS から SoA へ移行（公開 `Run` I/F は維持）

## 2. ユーザー操作差分

- `Preview Play (Selected ch)` は `Play Preview (Selected ch)` に変更
- `Replay Preview` を追加（直前バッファの再再生）
- `Loop Preview` を追加（プレビューのループ再生）
- `Stop` は再生停止 + レンダキャンセル要求を行う

## 3. 開発者向け差分

- `IRunObserver` に `ShouldCancel()` を追加
- `RenderMIDIEvents` にキャンセルコールバック経路を追加
- SynthEngine 内部状態を `VoicesSoA` へ移行
  - `source/note/velocity/phase/env/...` を配列分離
  - `AddVoice/MarkNoteOff/CleanupPending` を SoA API 化

## 4. 検証手順

```powershell
.\scripts\gui_smoke.ps1
```

確認項目:
- CLI成功系（default, channel samples, basic_wave, fm_default）
- `mix_all_mute` で `nonZero=0`
- CLI失敗系（invalid/missing config）
- GUI起動スモーク

## 5. 既知の制約

- キャンセルはチェックポイント単位で反映されるため、完全な即時停止ではない
- GUI操作の完全自動E2Eテストは未導入（スモークは起動確認中心）
