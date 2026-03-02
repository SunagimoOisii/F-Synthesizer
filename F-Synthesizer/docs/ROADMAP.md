# ROADMAP

最終更新: 2026-03-03

## 直近の優先事項

1. `MidiParser` running status 修正の回帰テスト追加
2. `SynthEngine` 同一ノート重なり時の `NoteOff` 対象ずれ修正
3. `Writer` 16bit 以外指定時のヘッダ/実データ不整合修正

## 完了した項目

- GUI v8 の主要移行完了（`Sound` 縮退、保存導線整理、エラー導線統一）
- GUI UX #7 完了（`Channel(ch)` / `Sound Slot(s)` 表記整理、Musicプレビュー時のSlot自動変更廃止）
- `scripts/gui_smoke.ps1` を `quick/full` プロファイル化（開発時の実行時間短縮）

## 将来の検討事項

- 合成方式拡張の運用改善（`docs/synth-methods/` の構成統一）
- GUI UX 改善項目の段階対応（`docs/STATUS.md` の Additional Backlog を参照）
