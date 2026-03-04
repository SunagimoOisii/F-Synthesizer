# GUI_MIGRATION_PHASES_v7

v7 は「初心者でも迷わず音を出せる」体験を最優先に、`Sound / Music` の責務を操作感として確定するフェーズ。
このファイルは 2026-02-23 時点で凍結（read-only運用）。

前提:
- 本計画は `docs/NOTES.md` の v7 基本方針と v7 操作感仕様を優先する。

## GUI_V7_PHASE_A_INTERACTION_CONTRACT_FREEZE

Status: DONE

- `Sound / Music` の責務境界を実装契約として固定
- プレビュー/停止/書き出しの挙動契約を固定
- 未保存警告（終了/切替）とエラー通知方針（推奨構成）を固定

完了条件:
- 主要操作の仕様に曖昧さがない
- `NOTES.md` と `GUI_REQUIREMENTS.md` の契約差分が解消される

## GUI_V7_PHASE_B_SOUND_MUSIC_FLOW_BASE

Status: DONE

- `Sound`: 音色作成 + 音色試聴導線を主導線化
- `Music`: MIDI編集 + 演奏調整 + 書き出し導線を主導線化
- `PR Channel` を表示/編集専用に固定（書き出し対象と非連動）

完了条件:
- 初回起動後に「とりあえず音を出す」「とりあえず書き出す」が実行できる
- `Sound` と `Music` の目的誤認が減る

## GUI_V7_PHASE_C_RUNTIME_BEHAVIOR_UNIFICATION

Status: DONE

- プレビュー実行時は毎回再生成へ統一
- タブ切替時は再生停止
- 再生中 `Export` は再生停止 -> 書き出し開始
- `Stop` は再生+レンダ両停止

完了条件:
- プレビュー/書き出しの状態遷移が一貫し、例外挙動が減る
- 主要再生系操作の回帰がない

## GUI_V7_PHASE_D_MUSIC_EDITING_CORE

Status: DONE

- Music側 16ch 固定ミキサー（Mute/Solo/Level/Pan/Gain）を整備
- 音色割当（`MIDI ch -> SoundAsset`）の基本UIを整備
- 書き出し対象を `All Channels` / `Single Channel` で明示選択可能にする
- ドラムch（ch10想定）の特別扱いを導入

完了条件:
- Music側で演奏調整と出力対象指定が完結する
- `chごとに1音色` の運用が成立する

## GUI_V7_PHASE_E_SAVE_GUARDRAIL_AND_ERROR_UX

Status: DONE

- 未保存状態での終了/切替警告を実装
- エラー時UX（ログ + 画面内表示 + 修正アクション + ダイアログ通知）を実装
- 保存対象の明示（`SoundAsset` / `MusicProject` / `Workspace`）を導入

完了条件:
- 誤終了・誤切替による作業損失を防げる
- 主要エラーでユーザーが次アクションを判断できる

## GUI_V7_PHASE_F_ACCEPTANCE_AND_RELEASE

Status: DONE

- v7受け入れテスト（初回音出し/初回書き出し/再現性）を整備
- ドキュメント（README/STATUS/GUI_REQUIREMENTS/NOTES）を更新
- v7リリース可否を判断

完了条件:
- v7の操作感仕様を満たす
- 日常運用できる品質でリリース可能（手動受け入れ完了後に確定）

## GUI_V7_PHASE_G_V8_IMPACT_REVIEW

Status: DONE

- v7実装結果を踏まえ、`docs/GUI_MIGRATION_PHASES_v8.md` の各フェーズ前提を再検証
- v7で顕在化した制約/課題が v8 の範囲・順序・完了条件へ与える影響を整理
- Soundタブ重複機能を監査し、責務逸脱（Musicと重複する演奏調整UI）の縮退方針を確定
- 影響がある場合、`docs/GUI_MIGRATION_TASKS_v8.md` の優先度とタスク内容を更新

完了条件:
- v8計画が最新実装状態と矛盾しない
- 変更理由がドキュメントに明記されている

レビュー結論（2026-02-23）:
- v7実装で `Sound` と `Music` に `MIDI Path / Output Path` の重複入力が残っており、初心者導線の迷い要因になっている
- `Sound` 側の `Target Channel` 入力は `Music` 側の `Output Target` と責務が重複するため、v8で縮退対象に確定
- v8は「見た目調整」より先に「責務境界のUI実体化（重複縮退）」を優先する
