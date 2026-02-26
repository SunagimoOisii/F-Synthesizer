# CLAUDE.md

このファイルは Claude Code がセッション開始時に自動で読み込む共通ルールです。
（旧 Codex 向け: `F-Synthesizer/docs/AGENTS.md`）

## プロジェクト構造

- コード本体: `F-Synthesizer/src/`, `F-Synthesizer/include/`
- ドキュメント: `F-Synthesizer/docs/`
- ビルド・チェックスクリプト: `F-Synthesizer/scripts/`

## セッション開始時の確認

1. `F-Synthesizer/docs/STATUS.md` を読む
2. `git status --short --branch` を確認する

## 作業ルール

- 変更前に対象ファイルと目的を明確にする
- 変更は最小単位で行う（1タスク = 1ゴール）
- 変更後は `.\F-Synthesizer\scripts\check.ps1` を実行する
- ファイル操作には Bash より専用ツールを優先する（Read / Edit / Write / Grep / Glob）

## コメント規約

`src/` または `include/` を変更する場合は `F-Synthesizer/docs/COMMENT_GUIDELINE.md` に従う。

要点:
- 全関数への一律コメントは禁止。高リスク箇所（誤用リスク API、並行性前提、境界値処理、最適化）に限定する
- 1ファイルあたりのコメント追加は原則 20 行以内
- `TODO` / `FIXME` をコードに残さない。設計検討は `docs/NOTES.md`、残タスクは `docs/STATUS.md` へ
- 抽象語だけで終わるコメントを残さない（5.1 節の用語チェックを実施する）

## ドキュメント更新ルール

| 変更箇所 | 更新が必要なファイル |
|---|---|
| `src/` または `include/` を変更 | `docs/STATUS.md` |
| `src/SynthEngine/` または `include/SynthEngine/` を変更 | `docs/Architecture.md` |
| 設計検討・方針メモ | `docs/NOTES.md` |
| 合成方式の追加・変更 | `docs/SYNTH_METHODS.md` + 対応する個別 md |

## 完了条件

- 実装が完了している
- `.\F-Synthesizer\scripts\check.ps1` が通る
- 関連する md が更新されている

## よく使うコマンド

```powershell
git status --short --branch
git log --oneline --decorate -n 12
.\F-Synthesizer\scripts\check.ps1
.\F-Synthesizer\scripts\check.ps1 -SkipBuild -SkipRun
```
