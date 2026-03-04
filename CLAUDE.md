# CLAUDE.md

このリポジトリの唯一の共通エントリは `F-Synthesizer/AGENTS.md`。
Claude Code は最初に同ファイルを読み、必要最小限の追加指示のみ本ファイルで適用する。

## セッション開始時

1. `F-Synthesizer/docs/STATUS.md` を読む
2. `git status --short --branch` を確認する

## Claude 固有の最小指示

- 大きな変更は短い計画を提示してから着手する。
- `src/` / `include/` のコメント規約は `F-Synthesizer/agents/standards/COMMENT_GUIDELINE.md` を既定とする。
- 詳細規定が必要な場合のみ `F-Synthesizer/agents/standards/COMMENT_GUIDELINE_FULL.md` を参照する。
- 変更後は `./F-Synthesizer/scripts/check.ps1` を実行する。
