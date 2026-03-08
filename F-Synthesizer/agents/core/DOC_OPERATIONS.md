# DOC_OPERATIONS.md — ドキュメント運用

## 自動更新対象

AI が直接更新してよいのは次の 3 ファイルのみ。

1. `docs/Architecture.md`
2. `docs/STATUS.md`
3. `docs/DECISIONS.md`

## STATUS 運用

- `docs/STATUS.md` を進捗管理の正本として運用する（`Current` / `Next 3` / `Blockers`）。

## 情報保持ポリシー

- 短期情報（次回週次まで）: `docs/STATUS.md`
- 長期判断: `docs/DECISIONS.md`
- 長期契約: `docs/synth-methods/foundation-contract.md`
- ADR詳細: `docs/architecture/*.md` の `Special Notes`

## 非自動更新ファイル

- 上記以外は直接変更しない。
- 変更が必要な場合は提案し、承認後に更新する。

## 提案テンプレ（非自動更新ファイル）

以下の形式で提案し、承認後に更新する。

```text
---
📋 提案制ファイルの更新提案

- 対象: <path>
- 要約: <何を変えるか>
- 理由: <なぜ必要か>
- 影響: <影響範囲>

承認 / 却下 / 修正 をお願いします。
```

## 更新後の報告

- ドキュメントを更新した場合、変更ファイルと要約を作業報告に記載する。
