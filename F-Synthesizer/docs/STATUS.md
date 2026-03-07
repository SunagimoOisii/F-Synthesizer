# STATUS

Last Updated: 2026-03-08 (foundation: SourceKind -> ParameterSchema[] を Waveform 以外へ拡張)
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
- `gui-cleanup`: Musicタブの補助文言/ヘルプを現行UI（Reference廃止後）へ再整理
- `doc-sync`: 個人運用前提で `method-boundaries` / `integration-playbook` / `STATUS` の記載整合を最終確認
- `ops`: 重い自動ハーネスは導入せず、`check.ps1` + 代表MIDI手動確認の運用を `OPERATIONS.md` へ明記
- `foundation`: lifecycle 契約（2.5）の最小受け皿として `SourceLifecyclePolicy` と `source.lifecycle` 整合検証を追加
- `foundation`: `SourceKind -> ParameterSchema[]` を Waveform/Noise/FM/Drum（DrumKit は空schema特例）へ拡張

## Next 3

1. `foundation`: `LoadSource.cpp` の検証を schema 駆動へ段階移行する（FM/Drum/DrumKit）
2. `doc-sync`: Musicタブ導線変更（Reference廃止）に伴う説明文/ガイド差分を点検する
3. `foundation`: lifecycle 実装挙動（retrigger/steal/one-shot終了）を契約 2.5 に照らして監査する

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 状況詳細: `STATUS_DETAIL.md`
- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
