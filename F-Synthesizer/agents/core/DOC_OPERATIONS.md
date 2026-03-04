# DOC_OPERATIONS.md — ドキュメント運用

## 自動更新対象

AI が直接更新してよいのは次の 4 ファイルのみ。

1. `docs/Architecture.md`
2. `docs/STATUS.md`
3. `docs/ROADMAP.md`
4. `docs/DECISIONS.md`

## STATUS 運用（分割）

- `docs/STATUS.md` はダッシュボードとして運用する（`Current` / `Next 3` / `Blockers`）。
- `docs/STATUS_DETAIL.md` は履歴・詳細ログ専用として運用する。
- AI は `docs/STATUS.md` 更新時に、必要な差分があれば `docs/STATUS_DETAIL.md` も直接更新してよい。

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
