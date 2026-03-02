# MD_MIGRATION_MAP

`F-Synthesizer` を AI 駆動運用テンプレートへ統合するためのマッピングと移行チェックリスト。

## 1. マッピング表

| 現在ファイル | 移行先/運用先 | 扱い |
|---|---|---|
| `docs/AGENTS.md` | ルート `AGENTS.md`（正本） | 統合・置換 |
| `docs/PRODUCT_POLICY.md` | `agents/core/AGENT.md` | 要約反映（原本は残す） |
| `docs/STATUS.md` | `docs/STATUS.md` | そのまま継続 |
| `docs/Architecture.md` | `docs/Architecture.md`（正本） | そのまま継続 |
| `docs/architecture/*.md` | `docs/Architecture.md` から参照 | 分割資料として維持 |
| `docs/NOTES.md` | `docs/DECISIONS.md` + `docs/STATUS.md` | 判断と残タスクへ分配 |
| `docs/GUI_UX_ISSUES.md` | `docs/STATUS.md`（既知の問題） | 移管後 archive 化 |
| `docs/COMMENT_GUIDELINE.md` | `agents/standards/COMMENT_GUIDELINE.md` | 標準規約として移設/同期 |
| `docs/GUI_REQUIREMENTS.md` | 非自動更新（提案制） | 現状維持 |
| `docs/SYNTH_METHODS.md` | 非自動更新（提案制） | 現状維持 |
| `docs/synth-methods/*.md` | 非自動更新（提案制） | 現状維持 |
| `docs/SOUND_PARAMETERS.md` | 非自動更新（提案制） | 現状維持 |
| `docs/PIANO_ROLL_CONTROLS.md` | 非自動更新（提案制） | 現状維持 |
| `docs/archive/*` | `docs/archive/*` | 読み取り専用で維持 |

## 2. 運用ルール（統合後）

### AI が直接更新してよいファイル

1. `docs/Architecture.md`
2. `docs/STATUS.md`
3. `docs/ROADMAP.md`
4. `docs/DECISIONS.md`

### それ以外のファイル

- 直接更新しない
- 提案を作成し、承認後に更新する

## 3. 移行チェックリスト

### Phase A: 導入準備

- [x] ルート `AGENTS.md` を正本として配置する
- [x] `CLAUDE.md` は `AGENTS.md` 参照の差分ファイルにする
- [x] `agents/core/` に `PRINCIPLES.md` / `OUTPUT_RULES.md` / `DOC_OPERATIONS.md` を配置する
- [x] `agents/standards/` にコメント規約を配置する

### Phase B: ドキュメント骨格の作成

- [x] `docs/Architecture.md` を作成/統一する
- [x] `docs/STATUS.md` を作成/統一する
- [x] `docs/ROADMAP.md` を作成/統一する
- [x] `docs/DECISIONS.md` を新規作成する
- [x] `docs/WEEKLY_MAINTENANCE.md` を配置する

### Phase C: 既存 md の移管

- [x] `docs/PRODUCT_POLICY.md` の要点を `agents/core/AGENT.md` に反映する
- [x] `docs/NOTES.md` の設計判断を `docs/DECISIONS.md` に移す
- [x] `docs/NOTES.md` の残タスクを `docs/STATUS.md` に移す
- [x] `docs/GUI_UX_ISSUES.md` の未解決項目を `docs/STATUS.md` に移す
- [x] `docs/Architecture.md` を正本として入口を一本化する

### Phase D: 運用切替

- [x] 自動更新対象を4ファイルに固定する
- [x] 非自動更新ファイルの提案テンプレを運用開始する
- [x] 週次メンテ（STATUS棚卸し -> Architecture反映 -> ROADMAP更新 -> DECISIONS追記）を開始する

### Phase E: 整理

- [x] 移行済み旧ファイルを `docs/archive/` へ移動する
- [x] リンク切れチェックを実施する
- [x] `README.md` と `AGENTS.md` の参照先を最終更新する

## 4. 完了条件

- [x] 正本ファイル（`AGENTS.md`）が明確
- [x] 自動更新4ファイルが運用されている
- [x] 非自動更新ファイルが提案制で回っている
- [ ] `docs/STATUS.md` と `docs/DECISIONS.md` が 1 週間以上更新されている
