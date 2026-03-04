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

監査実施時は個別ドキュメントに以下の表を使用し、完了後は
`docs/cleanup/archive/<batch>/` に移動して保管する。

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| example/path.cpp | function/file | `rg` の結果要約 | delete/keep | 短い理由 |

## Phase 実績

- Phase 1 (`scripts`): `docs/cleanup/archive/2026-03-phase1-6/scripts-audit.md`
- Phase 2 (`docs`): `docs/cleanup/archive/2026-03-phase1-6/docs-audit.md`
- Phase 3 (`gui`): `docs/cleanup/archive/2026-03-phase1-6/gui-audit.md`
- Phase 4 (`midi`): `docs/cleanup/archive/2026-03-phase1-6/midi-audit.md`
- Phase 5 (`core`): `docs/cleanup/archive/2026-03-phase1-6/core-audit.md`

## Phase 要約

- Phase 1 (`scripts`): `ref=0` の `scripts/piano_roll_smoke.ps1` を削除。
- Phase 2 (`docs`): 受け入れ手順参照と Architecture 記述の不整合を修正。
- Phase 3 (`gui`): 未使用ファイル/到達不能分岐なし（delete なし）。
- Phase 4 (`midi`): 未参照経路なし、`MarkNoteOff` フォールバックは維持妥当。
- Phase 5 (`core`): 未使用 `kPi` と冗長初期化ループを削除。

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
