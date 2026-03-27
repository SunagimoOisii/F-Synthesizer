# STATUS

Last Updated: 2026-03-27 (改善案A完了: nlohmann/json導入 + LoadSource分割)
Branch: `main`

進捗管理の正本は本ファイルのみ。

## Current

- 優先: `Tier D`（modulation表現拡張） / `sub-verify`（実機レンダ耳確認） / 軽量運用の維持
- 通常検証フローは `check.ps1` 1本化済み（既定は build-only、必要時のみ `-RunRuntimeSmoke`）
- `foundation` 契約（capability / lifecycle / schema）は `foundation-contract.md` を正本として凍結運用
- Renderer 内部を `source render -> common shaper -> modulation apply -> mix` へ分離し、`SourceRenderFrame` で段間データを受け渡す構造へ整理
- smoothing 方針を `waveform=適用` / `fm,noise,drum=非適用` に統一し、非対応方式の `source.smoothing` は load 時エラー化
- `source.type=analog` を追加（waveform同等の減算基盤 + drive + drift、GUI/Load/Save/Preset/Renderer/Voice初期化まで接続）
- SubtractiveConfig を廃止し、filterKeytrack を WaveformConfig へ移行（method-boundaries 準拠）
- `LoadSource.cpp` / `ConfigJSONUtils.cpp` の Waveform/Analog 重複処理をテンプレートヘルパーへ集約（共通パース・共通スキーマ値取得・共通JSON書き出し）
- 改善案A完了: `include/third_party/nlohmann/json.hpp` を導入し、`ConfigJSONUtils` の `ReadJSON*`/`ExtractObjectForKey`/`ParseTopLevelObjectEntries` を正規JSONパーサ実装へ移行、`LoadSource` を `LoadSourceCommon/Waveform/Fm/Drum/Noise` へ分割して `LoadSource.cpp` をディスパッチ専用化
- 優先度Aリファクタ: Waveform/Analog の重複を GUI編集 (`GUIChannelEditor.cpp`) / Renderer (`Renderer.cpp`) / Voice初期化 (`Voices.cpp`) で共通ヘルパー化（analog 固有 drift は分離維持）
- 優先度Bリファクタ: `LoadModulation.cpp` の smoothing parse/validate、`LoadSource.cpp` の schema 検証ループ、`GUIConfigUtils.cpp` の Waveform/Analog 等価比較を共通ヘルパー化
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
- Tier A-1 実装: Sound タブの Tone Preview にスペクトラムビューアを追加（Hann 窓付き簡易 DFT を 128 bin ヒストグラム表示、外部 FFT ライブラリ追加なし）
- Tier B-1 実装: FM アルゴリズムを `0..7` へ拡張（Renderer に case 4〜7 を追加、GUI の選択肢/テンプレート範囲を 0..7 へ更新、Load/Schema も 0..7 を受理）
- Tier B-2 実装: `ModDestination::FilterResonance` を追加し、`resonanceMul` を modulation 経路で評価して common shaper の `SetFilterResonance` に乗算適用（GUI destination / JSON parse+save 文字列も対応）
- Tier C-1 実装: `LfoWave` に `Square/Saw/SampleAndHold` を追加し、SynthEngine波形サンプル/Config parse+save/GUI LFO1 wave コンボを拡張
- Tier C-2 実装: `LfoConfig.keySync` と `NoteOnModulation(state, cfg)` を追加し、`keySync=true` 時はノートオンで `lfo1Phase=0` にリセット（Load/Save/GUI/比較関数まで反映）
- Tier C-3 実装: `ModSource::ModWheel` を追加し、`EvaluateModulation` で `input.modwheel(0..1)` を独立ソースとして評価可能化（既存の `lfo1 *= (1+modwheel)` は後方互換として維持）
- Tier D-1 実装: `LfoConfig` に `delayMs/fadeMs` と runtime の `lfo1ElapsedSec` を追加し、`StepLfoSample` で delay/fade-in を適用（Load/Save/GUI/比較関数まで反映）
- Tier D-2 実装: `ModEnvelopeConfig.curve` を追加し、`StepEnv2Sample` で `pow` ベースのカーブ適用を追加（Load/Save/GUI/比較関数まで反映）
- Tier C-1 実装（next-features）: Waveform/Analog の square に `pulseWidth`（0.05..0.95）を追加し、`ModDestination::PulseWidth` で LFO/Env2 から PWM を加算制御可能化（Oscillator/Renderer/Load/Save/Schema/GUI 反映）
- Tier C-2 実装（next-features）: Waveform/Analog に ring modulation（`ringModEnabled/ringModRatio/ringModMix`）を追加し、voice state の `ringPhase` を使って `mainWave` へ乗算適用（Renderer/Voices/Load/Save/Schema/GUI 反映）
- Tier D-1 実装（next-features）: Waveform/Analog に arpeggio（`enabled/rateHz/steps/semitones[8]`）を追加し、voice state の `arpStep/arpElapsedSec` で step進行しながら `pitchMul` へ半音オフセットを乗算適用（Renderer/Voices/Load/Save/GUI/比較関数 反映）
- Tier D-2 実装（next-features）: Waveform/Analog に hard sync（`hardSyncEnabled/hardSyncRatio`）を追加し、voice state の `syncPhase` ラップ時にスレーブ位相 `voices.phase` をリセットして倍音を生成（Renderer/Voices/Load/Save/Schema/GUI/比較関数 反映）
- `gui-help` 手動受け入れの記録を `docs/gui-help-hover-acceptance-checklist.md` に追加し、`GUI_REQUIREMENTS.md` と同期
- 完了済みの詳細履歴は `docs/DECISIONS.md` と Git 履歴を参照

## Next 3

1. `sub-verify`: `phaseE_sub_keytrack_A/B.json` と `wave_sub_bass_warm`（`lead_resonant` はファイル実在確認）を実機レンダして耳確認する
2. `tier2-regression`: `-RunRuntimeSmoke` 実行時の閾値/実行時間を見直し、軽量運用と両立させる
3. `docs-sync`: `Architecture.md` と `SOUND_PARAMETERS.md` の重複整理を継続する

## Blockers

- 現時点で重大ブロッカーなし

## Quick Links

- 設計: `Architecture.md`
- 実行手順: `OPERATIONS.md`
- 設計判断: `DECISIONS.md`
