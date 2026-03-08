# docs 監査ログ（Phase 2）

最終更新: 2026-03-04
準拠ポリシー: `docs/cleanup/deletion-policy.md`

## 対象と観点

- 対象: `docs/`（`docs-archive/` は原則除外）
- 観点:
  - 重複ページ
  - 運用停止済み手順
  - 現実装との不整合

## 調査サマリ

- `docs/**/*.md` を対象に、ファイル名参照を `rg` で集計した。
- `ref=0` は次の3件:
  - `docs/PIANO_ROLL_CONTROLS.md`
  - `docs/WEEKLY_MAINTENANCE.md`
  - `docs/cleanup/scripts-audit.md`
- 不整合として検出した3件のうち、明確なものは本フェーズで修正済み:
  - `OPERATIONS.md` の GUI 手動受け入れ参照先
  - `GUI_REQUIREMENTS.md` の v7/v8 手動受け入れ参照先
  - `Architecture.md` の GUI 実装配置説明

## 判定結果

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| docs/PIANO_ROLL_CONTROLS.md | file | ref=0 | keep | 手動参照の運用ドキュメント。削除確証がないため保留 |
| docs/WEEKLY_MAINTENANCE.md | file | ref=0 | keep | 定期運用手順。実行運用が継続中のため保留 |
| docs/cleanup/scripts-audit.md | file | ref=0 | keep | Phase 1 の監査記録。監査履歴として保持 |
| docs/OPERATIONS.md | file | ref=7 | keep | GUI手動受け入れ参照を `docs-archive/gui-migration/...` へ修正 |
| docs/GUI_REQUIREMENTS.md | file | ref=2 | keep | v7/v8受け入れ参照先を実在パスへ修正 |
| docs/Architecture.md | file | ref=16 | keep | GUI配置説明を `src/gui/` 実態に合わせて修正 |
| docs/ROADMAP.md | file | ref=5 (監査時点) | delete | 運用見直しにより `STATUS.md` へ統合して廃止 |

## 残課題

- `docs/ROADMAP.md` は `STATUS.md` へ統合して廃止済み（運用一本化）。
- `ref=0` の運用ドキュメントは入口リンクが弱いため、必要に応じて `README.md` か `OPERATIONS.md` から導線追加を検討する。
