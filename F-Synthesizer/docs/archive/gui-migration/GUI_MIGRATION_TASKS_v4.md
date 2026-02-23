# GUI_MIGRATION_TASKS_v4

このファイルは `GUI_MIGRATION_PHASES_v4.md` を実装タスクへ落とし込むチェックリストです。

このファイルの扱い:
- v0.4 開発中: 更新対象
- v0.4 完了後: `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v4.md` へ移動して凍結

## Phase A: Preview/Export Split

- [x] `RunMode`（`Preview/Export`）を追加
- [x] `RenderOptions` 構造を追加
- [x] `Run(config, options)` を導入
- [x] Preview既定値（短尺/保存なし）を定義
- [x] 既存CLIの後方互換を確認

完了条件:
- [x] PreviewとExportが別経路で動作する

## Phase B: In-App Playback

- [x] 再生ライブラリを導入（`miniaudio`）
- [x] GUIに `Play Preview` ボタンを追加
- [x] GUIに `Stop` ボタンを追加（再生停止）
- [x] GUIに `Loop Preview` トグルを追加
- [x] Preview結果のメモリ再生を実装（WAV保存不要）

完了条件:
- [x] 生成ファイルを開かずにPreview確認できる

## Phase C: Runtime Control

- [x] Render loopにキャンセルポイントを追加
- [x] `Stop` を実キャンセルへ接続
- [x] キャンセル時のログ/終了コードを定義
- [x] 連続Preview時の状態復帰を安定化
- [x] 実行中UIロック範囲を見直し

完了条件:
- [x] Preview中断が即時に効く

## Phase D: Direct SoA Migration

- [x] Voice状態を SoA 配置へ移行（周波数/位相/包絡など）
- [x] Event処理のホットパスを SoA 前提に最適化
- [x] ミックス処理（Mute/Solo/Pan/Gain）との整合を確認
- [x] 既存 `Run` I/F を維持したまま内部実装を SoA 化

完了条件:
- [x] SoA移行後に主要プリセットが実用再生できる
- [x] Preview体感速度が改善する

## Phase E: Stabilize & Release

- [x] `scripts/gui_smoke.ps1` をv0.4仕様へ拡張
- [x] Preview再生/停止/ループの回帰テストを追加
- [x] README更新（GUI内Preview運用手順）
- [x] Architecture更新（RunOptions/SoA直接移行）
- [x] STATUS更新（Current Snapshot/Next Actions）
- [x] v0.3 -> v0.4 移行手順を `docs/migration_v4.md` に記載

完了条件:
- [x] v0.4を日常運用できる品質でリリース可能

## 先行着手（推奨順）

1. Phase A の `RunMode/RenderOptions` 導入
2. Phase C の実キャンセル先行実装
3. Phase B のGUI内再生を接続
4. Phase D の SoA 直接移行
