# GUI_MIGRATION_TASKS_v7

このファイルは `docs/GUI_MIGRATION_PHASES_v7.md` の実装チェックリスト。
このファイルは 2026-02-23 時点で凍結（read-only運用）。

## Global Rules（全Phase共通）

- [x] `docs/PRODUCT_POLICY.md` の優先軸（音の気持ちよさ / 即時フィードバック / 初心者導線）に整合している
- [x] `docs/NOTES.md` の v7 基本方針・操作感仕様に整合している
- [x] `gui -> app -> core` の依存方向を維持している
- [x] 公開I/F（`Run` / `AppConfig`）互換を維持している
- [x] 非目標（DAW深機能、自動化等）をv7へ持ち込んでいない

## Phase A: Interaction Contract Freeze

- [x] `Sound` / `Music` の責務を `GUI_REQUIREMENTS.md` 上で確定
- [x] プレビュー/停止/Export の状態遷移を仕様化
- [x] 未保存警告の発火条件（終了/切替）を仕様化
- [x] エラー通知契約（ダイアログ + 画面内表示 + 修正アクション）を仕様化

完了条件:
- [x] 仕様衝突がない状態で実装へ着手できる

## Phase B: Sound/Music Flow Base

- [x] `Sound` 側の主要導線（音色作成 -> 試聴）を上段中心に整理
- [x] `Music` 側の主要導線（編集 -> 試聴 -> 書き出し）を上段中心に整理
- [x] `PR Channel` を表示/編集専用として明文化・実装

完了条件:
- [x] 初回音出しと初回書き出しが迷わず実行できる

## Phase C: Runtime Behavior Unification

- [x] プレビュー毎回再生成を統一実装
- [x] タブ切替時停止を実装/回帰確認
- [x] 再生中Exportの「停止 -> 書き出し」遷移を実装
- [x] `Stop` の再生+レンダ両停止を実装/回帰確認

完了条件:
- [x] 主要再生操作で状態遷移の破綻がない

## Phase D: Music Editing Core

- [x] Music側 16ch ミキサー（Mute/Solo/Level/Pan/Gain）を実装
- [x] `MIDI ch -> SoundAsset` 割当UIを実装（1ch=1音色）
- [x] 書き出し対象選択（All/Single）を実装
- [x] ドラムch（ch10想定）の特別扱いを実装

完了条件:
- [x] Music側だけで演奏調整と出力対象指定が完結する

## Phase E: Save Guardrail and Error UX

- [x] 未保存で終了時の警告ダイアログを実装
- [x] 未保存でプロジェクト切替時の警告ダイアログを実装
- [x] 保存対象明示（SoundAsset/MusicProject/Workspace）を実装
- [x] エラー時の画面内表示と修正アクションを実装

完了条件:
- [x] 作業損失の予防と復旧導線が機能する

## Phase F: Acceptance and Release

- [x] 受け入れテスト手順を追加（初回音出し/初回書き出し/再現性）
- [x] `scripts/gui_smoke.ps1` と回帰項目をv7仕様に拡張
- [x] README/STATUS/GUI_REQUIREMENTS/NOTES を更新
- [x] v7リリース判定を記録（手動受け入れ完了後に確定）

完了条件:
- [x] v7を日常運用できる品質でリリース可能

## Phase G: V8 Impact Review

- [x] v7実装結果と `docs/GUI_MIGRATION_PHASES_v8.md` の前提を照合
- [x] 影響差分（範囲/順序/完了条件）を整理
- [x] Soundタブ重複機能を監査し、責務逸脱（Musicと重複する演奏調整UI）の縮退方針を決定
- [x] 影響がある場合は `docs/GUI_MIGRATION_TASKS_v8.md` を更新
- [x] 変更理由を `docs/STATUS.md` または `docs/NOTES.md` に記録

完了条件:
- [x] v8計画がv7実装結果と整合している
