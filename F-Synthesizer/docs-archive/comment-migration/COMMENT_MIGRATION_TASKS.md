# COMMENT_MIGRATION_TASKS

このドキュメントは `docs/archive/comment-migration/COMMENT_MIGRATION_PHASES.md` の実装チェックリストです。

このファイルの扱い:
- コメント移行中: 更新対象
- 完了後: `docs/archive/comment-migration/COMMENT_MIGRATION_TASKS.md` へ移動して凍結

## Global Rules（全Phase共通）

- [x] `docs/COMMENT_GUIDELINE.md` に準拠している
- [x] コメント追加でロジック変更を混在させていない
- [x] `TODO` / `FIXME` コメントを追加していない
- [x] 各Phase完了時に `docs/STATUS.md` / `docs/Architecture.md` / 本ファイルを更新

## Phase 1: Baseline

- [x] 対象ファイル一覧を作成（`src/` / `include/`）
- [x] 優先順位を定義（core -> midi -> app/config -> gui/io）
- [x] 必須コメント対象（複雑分岐/最適化/概要）を抽出
- [x] レビュー判定基準を明文化
- [x] 成果物を `docs/archive/comment-migration/COMMENT_MIGRATION_BASELINE.md` に固定

完了条件:
- [x] どのファイルをどの順で対応するか明確
- [x] 規約違反の判定ルールが明確

## Phase 2: Core and MIDI

- [x] `src/SynthEngine/*` の複雑分岐へコメント追加
- [x] `src/SynthEngine/*` の最適化箇所へ「目的/前提/トレードオフ」コメント追加
- [x] `src/midi/*` の変換/境界条件へコメント追加
- [x] `include/SynthEngine/*` の型概要コメント補強

完了条件:
- [x] 主要レンダ/イベント経路の意図が追える
- [x] 最適化箇所の背景が追える

## Phase 3: App and Config

- [x] `src/app/*` の実行境界コメント追加
- [x] `src/config/*` の読込/検証/保存境界コメント追加
- [x] `include/AppCore.h` の公開I/F意図コメント補強
- [x] CLI互換維持の背景コメント確認

完了条件:
- [x] Run/Config経路の設計意図が追える
- [x] 互換維持箇所の背景が追える

## Phase 4: GUI and IO

- [x] `src/gui/*` の状態遷移/操作導線コメント追加
- [x] `src/gui/*` のプレビュー/保存導線コメント追加
- [x] `src/io/*` のUTF/Path処理コメント追加
- [x] エラー診断整形の意図コメント確認

完了条件:
- [x] GUI/IOの主要導線の意図が追える
- [x] 文字コード・パス処理の背景が追える

## Phase 5: Review and Freeze

- [x] 規約適合レビュー（違反は修正必須）
- [x] `README.md` / `docs/STATUS.md` / `docs/Architecture.md` 更新
- [x] `Debug x64` ビルド確認（既存確認を継続利用）
- [x] `scripts/gui_smoke.ps1` 実行確認
- [x] アーカイブ移動方針を確認して凍結準備
- [x] `docs/archive/comment-migration/` へ移動して凍結

完了条件:
- [x] 主要対象ファイルで必須コメントが充足
- [x] ビルド・主要スモーク通過
- [x] 文書だけで再開可能

Phase 5 メモ:
- `scripts/gui_smoke.ps1` は 13/13 通過（2026-02-21）
- `msbuild` がPATHにないため、本環境ではビルド再実行は未実施（既存のDebug x64成功を継続利用）
