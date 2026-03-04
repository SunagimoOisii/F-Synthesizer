# v0.4 -> v0.5 Migration Guide

このドキュメントは、GUI v0.4 から v0.5 へ移行する際の差分をまとめる。

## 1. 変更概要

- UIを `Top/Body/Bottom` の3領域へ再構成
- Top Bar に `Status` 表示と `UI Scale` 切替を追加
- チャンネル編集を「16ch全展開」から「Mix Summary + Selected Channel詳細」へ変更
- パス運用を改善（Browse/Copy/省略表示 + ホバー全文）
- v0.5リリース向けに `scripts/gui_smoke.ps1` を更新

## 2. ユーザー操作差分

- 主要アクションは上段へ集約
  - `Play` / `Play Preview (Selected ch)` / `Replay Preview` / `Stop`
- チャンネル編集は右パネルで実施
  - 一覧: `Mix Summary (M/S/L)`
  - 詳細: `Mix Details` / `Envelope / Gain` / `Source Details`
- ログは下段固定パネルで確認（高さ可変）
- 文字が小さい場合は `UI Scale` で 100/125/150% を切替

## 3. 開発者向け差分

- GUI状態管理を v0.5 構成に合わせて更新
  - 状態表示（Idle/Running/Preview/Canceled/Failed）
  - UIスケール値、ログ高さなどの状態保存
- `DrawChannelEditor` を v0.5 UX へ更新
  - 16ch要約テーブル
  - 選択チャンネル中心の折りたたみ詳細編集

## 4. 検証手順

```powershell
.\scripts\gui_smoke.ps1
```

確認項目:
- CLI成功系（default, channel samples, basic_wave, fm_default）
- `mix_all_mute` で `nonZero=0`
- CLI失敗系（invalid/missing config）
- GUI起動スモーク（既定起動）
- GUI起動スモーク（`--gui` 明示起動）

## 5. 既知の制約

- GUI自体のレイアウト/操作の完全自動E2Eテストは未導入
- プレビューはレンダ完了後の再生であり、真のリアルタイム音源編集ではない
