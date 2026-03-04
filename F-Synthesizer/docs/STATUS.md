# STATUS

Last Updated: 2026-03-05 (gui-help Phase 2-5 completed)
Branch: `main`

詳細ログと履歴は `STATUS_DETAIL.md` を参照。

## Current

- GUI v8 ベースの実装は安定運用中
- 現在の優先は `gui-help` / `feat-infra` / `gui-cleanup`
- コメント追加フェーズ（Step1-8）は完了
- `gui-help` Phase 2-7 を実施（共通導線化 + Music/Sound主要導線拡張 + 検証）

## Next 3

1. `feat-infra`: `scripts/midi_regression.ps1` の実行ディレクトリ依存を解消し、`check.ps1` 統合オプションを追加する
2. `gui-cleanup`: DrumConfig の `0 = 未指定（内部デフォルト）` を UI で明示する
3. `gui-help`: `docs/gui-help-hover-acceptance-checklist.md` に沿って手動ホバー確認を実施し、`GUI_REQUIREMENTS.md` 昇格/`docs-archive` 移管を完了する

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 状況詳細: `STATUS_DETAIL.md`
- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
