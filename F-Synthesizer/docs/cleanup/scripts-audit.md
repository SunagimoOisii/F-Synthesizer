# scripts 監査ログ（Phase 1）

最終更新: 2026-03-04
準拠ポリシー: `docs/cleanup/deletion-policy.md`

## 実施内容

- `scripts/*.ps1` を対象に、リポジトリ内参照を `rg` で調査した。
- 判定時は `docs-archive/` を参照対象から除外した。
- 削除前の必須検証として `Debug x64` ビルド成功を確認した（2026-03-04）。

## 判定結果

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| scripts/check.ps1 | file | ref=8（README / docs / scripts内） | keep | 運用入口として現役 |
| scripts/gui_smoke.ps1 | file | ref=9（README / docs / scripts内） | keep | 回帰スモークの主経路 |
| scripts/install_git_hooks.ps1 | file | ref=1（docs/OPERATIONS.md） | keep | フック導入手順で使用 |
| scripts/midi_regression.ps1 | file | ref=2（docs/STATUS_DETAIL.md） | keep | 直近の回帰確認対象 |
| scripts/piano_roll_smoke.ps1 | file | ref=0 | delete | 現行参照なし。加えて `docs\\archive\\...` 参照でパス不整合 |
| scripts/update_architecture_notes.ps1 | file | ref=1（scripts/check.ps1 から呼び出し） | keep | check連携で使用 |
| scripts/update_synth_docs.ps1 | file | ref=3（scripts/check.ps1 / docs） | keep | ドキュメント自動更新で使用 |

