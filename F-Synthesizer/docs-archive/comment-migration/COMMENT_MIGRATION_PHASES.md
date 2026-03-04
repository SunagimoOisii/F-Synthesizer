# COMMENT_MIGRATION_PHASES

このドキュメントは、既存コードへのコメント追加を段階的に進めるためのフェーズ定義です。

このファイルの扱い:
- コメント移行中: 更新対象
- 完了後: `docs/archive/comment-migration/COMMENT_MIGRATION_PHASES.md` へ移動して凍結

## 運用前提（固定）

- コメント規約は `docs/COMMENT_GUIDELINE.md` に従う
- 適用範囲は `src/` + `include/`
- `TODO` / `FIXME` は追加しない
- コメント追加で挙動を変えない（ロジック変更は別コミット）

## COMMENT_PHASE_1_BASELINE

Status: DONE (2026-02-21)

- 対象ファイル一覧と優先順位を作成
- 「必須コメント対象」（複雑分岐、最適化、ファイル/型概要）を抽出
- レビュー基準（修正必須ルール）を明確化
- 成果物: `docs/archive/comment-migration/COMMENT_MIGRATION_BASELINE.md`

完了条件:
- 対象ファイルと優先順が明文化されている
- チェック観点が `docs/archive/comment-migration/COMMENT_MIGRATION_TASKS.md` に反映されている

## COMMENT_PHASE_2_CORE_AND_MIDI

Status: DONE (2026-02-21)

- `src/SynthEngine/*` と `src/midi/*` の必須コメントを追加
- 最適化コードのコメント（目的/前提/トレードオフ）を補強
- `include/SynthEngine/*` の型概要コメントを補強
- 成果物: core/midi 対象ファイルへのコメント反映（ロジック変更なし）

完了条件:
- ホットパスと複雑分岐に説明がある
- 主要レンダ/イベント経路の意図が追える

## COMMENT_PHASE_3_APP_AND_CONFIG

Status: DONE (2026-02-21)

- `src/app/*` と `src/config/*` に方針コメントを追加
- 実行フロー境界・設定読込境界の意図を明文化
- `include/AppCore.h` の公開I/F意図を補強
- 成果物: app/config 対象ファイルへのコメント反映（ロジック変更なし）

完了条件:
- Run経路とConfig経路の設計意図が追える
- 互換維持箇所（CLI/設定キー）の背景が追える

## COMMENT_PHASE_4_GUI_AND_IO

Status: DONE (2026-02-21)

- `src/gui/*` と `src/io/*` にコメントを追加
- GUI状態遷移、プレビュー再生、保存導線の前提を明記
- UTF/Path/診断整形の意図を補強
- 成果物: gui/io 対象ファイルへのコメント反映（ロジック変更なし）

完了条件:
- GUI操作導線と保存導線の意図が追える
- 文字コード/パス処理の設計背景が追える

## COMMENT_PHASE_5_REVIEW_AND_FREEZE

Status: DONE (2026-02-21)

- 追加コメントの規約適合を横断レビュー
- `README.md` / `docs/STATUS.md` / `docs/Architecture.md` を更新
- 完了後、履歴を `docs/archive/comment-migration/` へ移動する（実施済み）
- 凍結方針: Phase 5 完了後に `docs/archive/comment-migration/` を作成し、`COMMENT_MIGRATION_PHASES.md` / `COMMENT_MIGRATION_TASKS.md` / `COMMENT_MIGRATION_BASELINE.md` を移動して凍結する

完了条件:
- 主要対象ファイルの必須コメントが充足している
- ビルド・主要スモークが通り、再開可能な文書状態である

備考:
- `scripts/gui_smoke.ps1` は 13/13 通過（2026-02-21）
- `Debug x64` は既存確認済みだが、本環境では `msbuild` が PATH に無く再実行検証は未実施
