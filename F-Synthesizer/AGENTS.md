# AGENTS.md — F-Synthesizer Entry Point

このプロジェクトでは本ファイルを正本とする。

## コンテキスト読み込み順

1. `agents/core/AGENT.md`
2. `agents/core/PRINCIPLES.md`
3. `agents/core/OUTPUT_RULES.md`
4. `agents/core/DOC_OPERATIONS.md`
5. `agents/core/INSTRUCTIONS.md`
6. `agents/standards/COMMENT_GUIDELINE.md`（必須版）
7. `agents/safety/RISK_POLICY.md`（存在する場合）

## 必要時のみ参照

- `agents/standards/COMMENT_GUIDELINE_FULL.md`（境界条件や運用手順の詳細が必要な場合）

## 最低限ルール

- `src/` と `include/` のコメント規約は `agents/standards/COMMENT_GUIDELINE.md` を既定として適用する。
- 境界条件や運用手順の詳細が必要な場合のみ `agents/standards/COMMENT_GUIDELINE_FULL.md` を参照する。
- 指示が競合した場合は、ユーザーの明示指示を最優先とする。
- ドキュメントは `agents/core/DOC_OPERATIONS.md` の運用を適用する。

## 運用切替（Phase D）

- 自動更新対象は `docs/Architecture.md` / `docs/STATUS.md` / `docs/DECISIONS.md` の 3 ファイルに固定。
- 進捗管理の正本は `docs/STATUS.md` のみとする。
- 上記以外の md 変更は、提案テンプレで承認を取ってから反映する。
