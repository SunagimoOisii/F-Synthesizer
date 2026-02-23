# AGENTS.md

このファイルは、このリポジトリで Codex を使って作業する際の共通ルールです。

## Goal

- 変更の再現性を上げる
- 久しぶりに開いても状況把握を速くする
- コード変更とドキュメント更新のズレを減らす

## Start Here

1. `STATUS.md` を読む
2. `git status --short --branch` を確認
3. 必要なら `.\scripts\check.ps1 -SkipBuild -SkipRun` を実行

## Required Workflow

1. 変更前に対象ファイルと目的を明確化する
2. 変更は最小単位で行う（1タスク=1ゴール）
3. 変更後に `.\scripts\check.ps1` を実行する
4. ルールに従って markdown を更新する

## Documentation Rules

- `src/` または `include/` を変更したら `STATUS.md` を更新する
- `src/SynthEngine/` または `include/SynthEngine/` を変更したら `Architecture.md` を更新する
- 設計検討メモは `NOTES.md` に記録する

## Definition Of Done

- 実装が完了している
- 必要な検証を実行している
- `scripts/check.ps1` が通る（必要に応じて `-AllowDocMismatch` を明示利用）
- 関連する md が更新されている

## Useful Commands

```powershell
git status --short --branch
git log --oneline --decorate -n 12
.\scripts\check.ps1
.\scripts\check.ps1 -SkipBuild -SkipRun
```

