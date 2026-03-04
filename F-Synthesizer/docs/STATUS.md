# STATUS

Last Updated: 2026-03-05 (sound-reference removed)
Branch: `main`

詳細ログと履歴は `STATUS_DETAIL.md` を参照。

## Current

- GUI v8 ベースの実装は安定運用中
- 現在の優先は `gui-help` / `feat-infra` / `doc-sync`
- コメント追加フェーズ（Step1-8）は完了
- `gui-help` Phase 2-9 を実施（共通導線化 + Music/Sound主要導線拡張 + 要件昇格 + 計画移管）
- `feat-infra` を実施（`midi_regression.ps1` のCWD依存解消 + `check.ps1 -RunMIDIRegression` 統合）
- 略称大文字統一を継続実施（`MIDI` / `WAV` / `CLI` / `JSON`）
- 略称大文字統一を継続実施（`UI` / `ID`、GUI state保存キーは後方互換のため旧キー併用）
- `runtime`: 未使用だった `Default Wave` 導線（`defaultWave` / `MIDIEvent.typeWave`）を廃止
- `gui-cleanup`: 未接続だった `Sound Reference (Snapshot/Link)` UI/状態保存を廃止

## Next 3

1. `doc-sync`: UI操作と実データ反映の実装監査メモを継続し、`STATUS_DETAIL` へ追記する
2. `test`: `check.ps1` のビルド実行環境（msbuild PATH）を整備し、CI/ローカルで同一手順にする
3. `gui-cleanup`: Musicタブの補助文言/ヘルプを現行UI（Reference廃止後）に合わせて再整理する

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 状況詳細: `STATUS_DETAIL.md`
- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
