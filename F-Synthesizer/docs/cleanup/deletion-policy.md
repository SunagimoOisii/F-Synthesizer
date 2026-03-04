# Cleanup 削除方針（Phase 0）

最終更新: 2026-03-04
管理者: AIエージェント（Codex）

## 対象範囲

- 本方針は、このリポジトリの cleanup 作業に適用する。
- `docs-archive/` は対象外とする。

## 判定ルール

- `delete`: 参照数が `0` の場合は削除する。
- `keep`: 判定に迷いがある場合は削除せず保留する。
- `deprecate`: 本プロジェクトでは使用しない。

## 削除前の検証

- 必須チェック: `Debug x64` ビルド成功。
- 削除承認に追加の必須チェックは設けない。

## エスカレーション

- 判定が不明確な項目は、AIエージェントが `keep` として削除を見送る。

## 監査記録フォーマット

各フェーズの監査ドキュメント（`docs/cleanup/*-audit.md`）で、以下の表を使用する。

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| example/path.cpp | function/file | `rg` の結果要約 | delete/keep | 短い理由 |
