# GUI_MIGRATION_TASKS_v8

このファイルは `docs/GUI_MIGRATION_PHASES_v8.md` の実装チェックリスト。

## Global Rules（全Phase共通）

- [x] v7で確定した操作骨格（初心者導線/即時フィードバック）を壊していない
- [x] `snapshot` 既定による再現性を維持している
- [x] 非目標（DAW深機能、自動化）をv8へ持ち込んでいない
- [x] 文言は目的語中心で、専門語の露出を最小化している

## Phase A: Assignment and Mix Polish

- [x] Soundタブから `MIDI Path / Output Path / Target Channel` を縮退し、Musicへ責務集約
- [x] Soundタブは `音色作成 / Play Tone / チャンネル試聴` の最小導線へ再配置
- [x] 音色割当一覧UIの視認性を改善
- [x] 割当編集の操作回数を削減
- [x] ミックスUI（Mute/Solo/Level/Pan/Gain）の情報密度を最適化
- [x] `PR Channel` / 割当 / 出力対象の関係を明示する表示を追加

完了条件:
- [x] Sound/Musicの責務重複がUI上で解消されている

## Phase B: Reference and Save Polish

- [x] `Save Project` 主導線を磨き込み
- [x] `Save All` の対象と影響範囲を明示
- [x] `snapshot` 推奨表示を統一
- [x] `link` を詳細設定として追加（通常導線から分離）

完了条件:
- [x] 保存対象（SoundAsset/MusicProject/Workspace）の境界を誤認しない

## Phase C: Drum and Channel Specialization

- [x] ドラムch向けの操作文言/表示を調整
- [x] ch10想定の編集ショートカット導線を追加

完了条件:
- [x] ドラム編集の着手コストが下がる

## Phase D: Error and Recovery Polish

- [x] 代表エラーの修正アクションボタンを定義
- [x] ダイアログ文言を初心者向けに統一
- [x] 画面内エラー表示の配置と優先度を統一

完了条件:
- [x] エラー時の復旧導線が一貫している

## Phase E: Acceptance Metrics and Release

- [x] v8受け入れテストを追加
- [x] 初回導線/反復試行/再現性の確認項目を追加
- [x] `scripts/gui_smoke.ps1` と関連手順を更新
- [x] README/STATUS/GUI_REQUIREMENTS/NOTES を更新
- [x] v8リリース判定を記録

完了条件:
- [x] v7比で体験品質が改善した状態でリリース可能
