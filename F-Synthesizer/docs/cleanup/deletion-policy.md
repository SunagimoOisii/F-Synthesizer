# Cleanup 削除方針（確定版）

最終更新: 2026-03-04 (Phase 6 完了)
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

## Phase 実績

- Phase 1 (`scripts`): `docs/cleanup/scripts-audit.md`
- Phase 2 (`docs`): `docs/cleanup/docs-audit.md`
- Phase 3 (`gui`): `docs/cleanup/gui-audit.md`
- Phase 4 (`midi`): `docs/cleanup/midi-audit.md`
- Phase 5 (`core`): `docs/cleanup/core-audit.md`

## 実削除・整理の結果

- ファイル削除:
  - `scripts/piano_roll_smoke.ps1`（ref=0）
- コード片削除:
  - `src/core/AudioBuffer.cpp` の未使用定数 `kPi`
  - `src/core/AudioBuffer.cpp` の冗長な 0 初期化ループ

## Phase 6 検証結果

実施日: 2026-03-04

- `Debug x64` ビルド: 成功（warning 0 / error 0）
- `scripts/gui_smoke.ps1`（quick）: 通過（6/6）
- `scripts/midi_regression.ps1`: 通過（running status 2件 + overlap same note 1件）

## 結論

- 本 cleanup では、`delete` 判定分のみ削除を実施した。
- 判定に迷う項目は方針どおり `keep` 保留とし、監査ログに記録した。
