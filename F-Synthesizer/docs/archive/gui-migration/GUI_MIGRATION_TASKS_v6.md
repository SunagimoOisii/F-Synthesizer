# GUI_MODE_SPLIT_TASKS

このドキュメントは `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v6.md` の実装チェックリストです。

## Global Rules（全Phase共通）

- [x] 依存方向 `gui -> app -> core` を維持している
- [x] 公開I/F（`Run` / `AppConfig`）の互換を壊していない
- [x] 既存GUI機能（Preset/Preview/Save）を破壊していない
- [x] `docs/PRODUCT_POLICY.md` の優先軸（音の気持ちよさ/即時フィードバック）に整合している
- [x] コメントは `docs/COMMENT_GUIDELINE.md` に準拠している

## Phase 1: Layout and Tab State

- [x] `Sound / Music` タブUIを追加
- [x] 初期タブを `Sound` に設定
- [x] タブ状態の保存/復元を追加

完了条件:
- [x] 起動時に `Sound` タブが表示される
- [x] タブ切替が安定して動く

## Phase 2: Action Routing

- [x] `Sound` タブに音源作成/試聴UIを配置
- [x] `Music` タブに `Export WAV` を配置
- [x] `Music` タブの再生を常に再生成に固定
- [x] タブ切替時の自動停止を実装

完了条件:
- [x] タブごとの目的に沿ったUIに分離されている
- [x] タブ切替時の停止が機能する

## Phase 3: Log and Hint

- [x] タブ別ログ表示を実装
- [x] ホバーUIヘルプ（下部1行）を実装
- [x] 既存ログの診断情報を維持

完了条件:
- [x] ログを `Sound` / `Music` で分けて確認できる
- [x] ホバー対象に応じてヘルプ文が切り替わる

## Phase 4: Compat and Polish

- [x] `scripts/gui_smoke.ps1` 互換確認
- [x] Preview/Stop/Export の回帰確認
- [x] 文言の目的語化（初心者向け）を実施

完了条件:
- [x] スモークテストが通る
- [x] 既存機能と干渉しない
