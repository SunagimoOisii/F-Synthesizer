# gui-help hover acceptance checklist

最終更新: 2026-03-27

`GUI_REQUIREMENTS.md` の「ホバー補助文言契約（gui-help）」に対する受け入れ記録。

## 2026-03-27 実施ログ

- 実施コマンド: `.\scripts\gui_smoke.ps1 -Profile quick`
- 結果: PASS（`--gui` 起動を含む quick smoke 完了、異常終了なし）
- 追加確認: `src/gui/main/MainWindow.inl` / `src/gui/GUIChannelEditor.cpp` の `updateHoverHelp(...)` 呼び出しを走査し、主要導線（編集/試聴/書き出し/停止/保存）で文言接続があることを確認

## 手動ホバー受け入れ観点

- [x] Sound/Music タブ切替にホバー文言が出る
- [x] `Play Preview` / `Export WAV` / `Stop` にホバー文言が出る
- [x] `Save Project` / `Save All` / `Close` にホバー文言が出る
- [x] Sound の主要編集（Amp/ADSR/Filter/LFO/Route）にホバー文言が出る
- [x] Music ミキサー（Mute/Solo/Level/Pan/Gain/Assign）にホバー文言が出る
- [x] 高リスク操作で `注意` 文言が付く（例: Gain, Export, Save）
- [x] Pulse Width / Hard Sync / Ring Mod の hover 文言が音楽的表現に更新済み（Phase 1-F）
- [x] Arpeggio Enabled / Rate / Steps / Note N の hover 文言が追加済み（Phase 1-F）
- [x] LFO1 Wave combo の hover 文言が追加済み（Phase 1-F）
- [x] Env2 Attack / Decay / Sustain / Release / Curve の hover 文言が追加済み（Phase 1-F）

## 2026-03-28 実施ログ（Phase 1-F）

- 実施内容: Tier C/D で追加されたパラメータの hover 文言欠落を解消
- 変更ファイル: `ChannelEditorCommon.inl` / `ChannelEditorModulation.inl`
- 追加項目: LFO1 Wave / Env2(5項) / Hard Sync / Sync Ratio / Ring Mod / Ring Ratio / Ring Mix / Arpeggio(4項) / Pulse Width 改善（計 16 箇所）
- 結果: 主要導線（Filter / Envelope / LFO / Modulation / Arpeggio）で hover 文言欠落なし

## 補足

- ホバー文言は `MainWindow.inl` の `composeHoverHelp` で `影響` 優先表示、必要時のみ `注意` 追記の契約に統一されている。
- 新規UI項目追加時は「項目描画の直後に `updateHoverHelp(...)`」を継続する。
