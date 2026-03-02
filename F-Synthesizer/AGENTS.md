# AGENTS.md — F-Synthesizer Entry Point

このプロジェクトでは本ファイルを正本とする。

## コンテキスト読み込み順

1. `agents/core/AGENT.md`
2. `agents/core/PRINCIPLES.md`
3. `agents/core/OUTPUT_RULES.md`
4. `agents/core/DOC_OPERATIONS.md`
5. `agents/core/INSTRUCTIONS.md`
6. `agents/standards/` 配下（存在する場合）
7. `agents/safety/RISK_POLICY.md`（存在する場合）

## 最低限ルール

- `src/` と `include/` のコメント規約は `agents/standards/COMMENT_GUIDELINE.md` を適用する。
- 指示が競合した場合は、ユーザーの明示指示を最優先とする。
- ドキュメントは `agents/core/DOC_OPERATIONS.md` の運用を適用する。

## 運用切替（Phase D）

- 自動更新対象は `docs/Architecture.md` / `docs/STATUS.md` / `docs/ROADMAP.md` / `docs/DECISIONS.md` の 4 ファイルに固定。
- 上記以外の md 変更は、提案テンプレで承認を取ってから反映する。
