# STATUS

Last Updated: 2026-03-26 (週次メンテ: masterEffects構成と表現拡張の文書同期)
Branch: `main`

進捗管理の正本は本ファイルのみ。

## Current

- 優先: `doc-sync` / `gui-help`（実機確認） / 軽量運用の維持
- 通常検証フローは `check.ps1` 1本化済み（必要時のみ `full + MIDI regression`）
- `foundation` 契約（capability / lifecycle / schema）は `foundation-contract.md` を正本として凍結運用
- Renderer 内部を `source render -> common shaper -> modulation apply -> mix` へ分離し、`SourceRenderFrame` で段間データを受け渡す構造へ整理
- smoothing 方針を `waveform=適用` / `fm,noise,drum=非適用` に統一し、非対応方式の `source.smoothing` は load 時エラー化
- `source.type=analog` を追加（waveform同等の減算基盤 + drive + drift、GUI/Load/Save/Preset/Renderer/Voice初期化まで接続）
- SubtractiveConfig を廃止し、filterKeytrack を WaveformConfig へ移行（method-boundaries 準拠）
- architecture と SOUND_PARAMETERS の重複整理を進行中（正本一本化 + 履歴は `docs-archive/`）
- waveform / fm の modulation route 編集GUIを `0..7` まで拡張（サマリ表示含む）
- Tier1 expression foundation（MIDI CC拡張 / sustain hold / velocity mod source）を実装完了し、tier1 roadmap を破棄
- Tier2実装: stereoレンダリング（L/R分離, pan equal-power）, PitchBend RPN(0)対応を追加
- master effect layer を `SampleRateReducer -> BitCrusher -> Chorus -> Flanger -> Delay -> Reverb` へ拡張し、Run/Config境界の `masterEffects` で一括制御（delay tempoSync は tempo map 追従）
- 表現拡張として Aftertouch（Channel Pressure / Poly Pressure）を追加
- Tier2 roadmap（`docs/roadmap/tier2-sound-quality-and-playability.md`）は完了に伴い破棄し、契約情報を正本ドキュメントへ移管
- Sound タブ左カラム末尾に波形ビューア追加: `previewRenderedSound` の PCM を 256 点にダウンサンプリングして `ImGui::PlotLines()` 表示、再生中は `frameCursor` でカーソル縦線をオーバーレイ（`MainWindow.inl` のみ変更、新規ファイルなし）
- Tier A-2 実装: `NoiseConfig` に共通 filter（`filterMode/filterCutoffHz/filterResonance`）を追加し、Noise を `common shaper (BiquadFilter)` 経路へ接続（Load/Save/Registry/GUI/Voice 初期化まで反映）
- Tier A-3 実装: Tone Preview ノート番号を `tonePreviewNoteNumber` として保存可能にし、Sound タブで `Preview Note` を手動選択（DrumKit 時は非表示、`ResolveSoundTonePreviewNote` は DrumKit 以外で手動値を優先）
- 完了済みの詳細履歴は `docs/DECISIONS.md` と Git 履歴を参照

## Next 3

1. `tier2-regression`: pan端点, effect端点（SampleRateReducer/BitCrusher/Flanger含む）, RPN±12 の回帰チェックを軽量スモークへ組み込む
2. `sub-verify`: `phaseE_sub_keytrack_A/B.json` と `wave_sub_bass_warm/lead_resonant.json` を実機レンダして耳確認する
3. `gui-help`: 手動ホバー受け入れ確認（実機GUI操作）を実施し、必要なら `GUI_REQUIREMENTS.md` へ追記する

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
