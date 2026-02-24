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

## Auto Comment Operation

コメントを自動追加する場合は、次のガードを必須とする。

1. 対象限定
- 対象は `docs/COMMENT_GUIDELINE.md` の高リスク箇所（誤用リスクAPI、並行性前提、境界値/フォールバック、最適化）に限定する。
- 全関数への一律付与は禁止。

2. 差分制限
- 1ファイルあたりのコメント追加は原則20行以内。
- 超える場合は分割するか、手動レビューを必須にする。

3. 生成後セルフチェック
- 抽象語だけで終わるコメント（例: 同一文脈、適切に処理）を残さない。
- 既存コメントの重複や言い換えだけの追記を避ける。
- 呼び出し経路コメントは断定しすぎない（実装変更で壊れやすい説明を避ける）。

4. 失敗時の扱い
- 品質が担保できない場合は、コメント追加をスキップして理由を記録する。
- 規約を満たさないコメントを無理に埋めない。

5. PR/報告チェック項目
- なぜそのコメントが必要か（対象リスク）を明記する。
- 過剰コメントを削除/抑制したかを明記する。
- 誤用しやすいAPI対の使い分けを明記したかを明記する。

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
