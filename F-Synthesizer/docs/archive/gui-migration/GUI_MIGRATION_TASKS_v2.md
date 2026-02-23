# GUI_MIGRATION_TASKS_v2

このファイルは `GUI_MIGRATION_PHASES_v2.md` を実装タスクへ落とし込むチェックリストです。

このファイルの扱い:
- v0.2 開発中: 更新対象
- v0.2 完了後: `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v2.md` へ移動して凍結

## Phase A: Config Schema

- [x] `config/channel_schema.md` を作成（キー定義と範囲）
- [x] 既存 `base/preset` との互換ルールを定義
- [x] `AppConfig` 側の必要項目を洗い出し
- [x] サンプルJSON（最小/完全）を追加

完了条件:
- [x] JSON構造が固定され、実装が開始できる

## Phase B: Config IO

- [x] チャンネル定義の読み込み処理を追加
- [x] チャンネル定義の保存処理を追加
- [x] 値範囲チェック（ch, amp, ADSR, wave type）を追加
- [ ] エラーメッセージをGUI/CLI双方で統一

完了条件:
- [x] JSON変更だけで音設定が反映される

## Phase C: Channel Editor UI

- [x] チャンネル選択UI（0-15）を追加
- [x] Source種別切替UIを追加
- [x] ADSR/amp編集UIを追加
- [x] ソース種別ごとのパラメータ編集UIを追加
- [x] 実行前バリデーションをUI入力に連動

完了条件:
- [x] GUIのみでチャンネル設定を編集し実行できる

## Phase D: Preset Workflow

- [x] `Save preset as` を追加
- [x] `Duplicate preset` を追加
- [x] `Reset channel` を追加
- [x] dirty状態表示（未保存変更）を追加
- [x] baseとの差分保存を実装

完了条件:
- [x] GUI中心でプリセット作業が完結する

## Phase E: Stabilize & Release

- [x] `scripts/gui_smoke.ps1` をv0.2仕様に拡張
- [x] 保存/読込/実行/失敗系の最小テスト追加
- [x] README更新（新UI操作手順）
- [x] Architecture更新（設定フロー/データ構造）
- [x] STATUS更新（Current Snapshot/Next Actions）
- [x] 旧設定からの移行手順を `docs/migration_v2.md` に記載

完了条件:
- [x] v0.2を日常運用できる品質でリリース可能

## 先行着手（推奨順）

1. Phase A のスキーマ確定
2. Phase B の読み込みのみ先行実装
3. Phase C の ch選択 + ADSR編集から着手
