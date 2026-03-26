# STATUS

Last Updated: 2026-03-26 (Tier1 expression foundation 実装完了・roadmap整理)
Branch: `main`

進捗管理の正本は本ファイルのみ。

## Current

- 優先: `doc-sync` / `gui-help`（実機確認） / 軽量運用の維持
- 通常検証フローは `check.ps1` 1本化済み（必要時のみ `full + MIDI regression`）
- `foundation` 契約（capability / lifecycle / schema）は `foundation-contract.md` を正本として凍結運用
- Renderer 内部を `source render -> common shaper -> modulation apply -> mix` へ分離し、`SourceRenderFrame` で段間データを受け渡す構造へ整理
- smoothing 方針を `waveform=適用` / `fm,noise,drum=非適用` に統一し、非対応方式の `source.smoothing` は load 時エラー化
- SubtractiveConfig を廃止し、filterKeytrack を WaveformConfig へ移行（method-boundaries 準拠）
- architecture と SOUND_PARAMETERS の重複整理を進行中（正本一本化 + 履歴は `docs-archive/`）
- waveform / fm の modulation route 編集GUIを `0..7` まで拡張（サマリ表示含む）
- Tier1 expression foundation（MIDI CC拡張 / sustain hold / velocity mod source）を実装完了し、tier1 roadmap を破棄
- 完了済みの詳細履歴は `docs/DECISIONS.md` と Git 履歴を参照

## Next 3

1. `tier2-verify`: Tier2候補（stereo/effects/pitchbend-rpn）の実装順を確定し、最小スコープを固定する
2. `sub-verify`: `phaseE_sub_keytrack_A/B.json` と `wave_sub_bass_warm/lead_resonant.json` を実機レンダして耳確認する
3. `gui-help`: 手動ホバー受け入れ確認（実機GUI操作）を実施し、必要なら `GUI_REQUIREMENTS.md` へ追記する

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
