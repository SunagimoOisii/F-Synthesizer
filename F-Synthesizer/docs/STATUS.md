# STATUS

Last Updated: 2026-03-05 (feat-infra+infra-fix done)
Branch: `main`

詳細ログと履歴は `STATUS_DETAIL.md` を参照。

## Current

- GUI v8 ベースの実装は安定運用中
- 現在の優先は `gui-help` / `feat-infra` / `gui-cleanup`
- コメント追加フェーズ（Step1-8）は完了
- `gui-help` Phase 2-9 を実施（共通導線化 + Music/Sound主要導線拡張 + 要件昇格 + 計画移管）
- `feat-infra` を実施（`midi_regression.ps1` のCWD依存解消 + `check.ps1 -RunMIDIRegression` 統合）

## Next 3

1. `gui-cleanup`: DrumConfig の `0 = 未指定（内部デフォルト）` を UI で明示する
2. `refactor`: `module-map.md` の自動生成 TODO（背景/判断/影響範囲）を記入する
3. `gui-help`: `docs/gui-help-hover-acceptance-checklist.md` に沿って最終手動ホバー確認を実施し、`gui-help` をクローズする

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 状況詳細: `STATUS_DETAIL.md`
- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
