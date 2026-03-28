# STATUS

Last Updated: 2026-03-28 (GUI レイアウト修正: FX チェーン座標バグ修正 + Sound タブ 3 層縦スタック化)
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
- 改善案B完了: `GUIState` を `renderParams/run/preview` の sub-struct で整理し、`GUIChannelEditor.cpp` を `src/gui/channeleditor/*.inl` へ分割、`GUIActions.cpp` を `GUIRunActions.cpp` / `GUIPreviewActions.cpp` / `GUIPresetIO.cpp` に責務分離
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
- Fix B-1 実装: Layer2 マクロ操作時の `autoTonePreviewPending` 直書きを廃止し、`autoTonePreviewEnabled` 時のみ `autoTonePreviewLastEditSec` を更新してデバウンスが正しく機能するよう修正
- Fix A-1 実装: Sound左カラムの重複 `Preset` コンボと `Apply Preset Paths` ボタンを削除し、プリセット選択導線を右カラム Layer1 Discovery に一本化
- Fix A-2 実装: 左カラム `Source Type` ラベル/ヘルプ文言を Layer1 連動前提へ更新し、Layer1ヘッダに現在の source 種別とプリセット件数を表示
- Fix B-2 実装: Layer3 の `Filter Resonance (Q)` ホバーヘルプに Layer2「荒さ」の書き込み範囲（0.5〜6.0）注記を追加し、UI上の範囲差を明示
- `gui-help` 手動受け入れの記録を `docs/gui-help-hover-acceptance-checklist.md` に追加し、`GUI_REQUIREMENTS.md` と同期
- Phase 1-C 実装: Tone Preview / Play Preview 再生中の VU メーターを Sound タブ左カラム（Spectrum 下）に追加（ファストアタック/スローデケイ、1.5 秒ピークホールド、CLIP 表示。GUIState 追加なし、VUMeter.inl + MainWindow.inl のみ変更）
- Phase 1-D 実装: Sound タブ Layer2/Layer3 の Undo/Redo（Ctrl+Z/Y）を追加。Layer2 は per-slider IsItemActivated フック、Layer3 は IsAnyItemActive() ブラケット方式で 1 操作 = 1 エントリ（GUISoundHistory.h/cpp 新規、GUIState/Layer2Macros/GUIChannelEditor/MainWindow 変更、スタック上限 50）
- Phase 1-F 実装: Tier C/D 追加パラメータの hover 文言欠落を解消（Pulse Width 改善、Hard Sync/Ring Mod/Arpeggio/LFO1 Wave/Env2 系 計 16 箇所を ChannelEditorCommon.inl + ChannelEditorModulation.inl に追加）
- Phase 2-A 実装: Layer3 Channel Editor に ADSR/LFO グラフィカル表示を追加（`Envelope / Gain` に ADSR プレビュー、`Modulation` に LFO1 波形プレビュー + Env2 カーブプレビュー。`LfoWave` 全列挙値対応）
- Phase 2-B 実装: FM Source Details に Algorithm 0〜7 のオペレーター接続図を追加（DrawList描画、Op0 feedback ループ表示、algo 変更時に即時切替）
- Phase 2-C 実装: Virtual Keyboard に Chord モード（Major/Minor/7th/Minor7th/Sus4）を追加し、Tone Preview で同時和音を発音。構成音ハイライト表示と GUI state 永続化（`chordModeEnabled/chordType`）を追加
- Phase 2-E 実装: `UIModeTab=2` の Export タブを追加し、書き出し専用ビュー（`ExportView.inl`）を実装。Output Path / Output Target / Export/Preview/Stop / VU メーター / 最近の書き出し表示を集約し、ログを `exportLogs` へ分離。`TryFinalizeCompletedRun` で成功時の `recentWavPaths`（最大5件）を更新
- Phase 2-F 実装: Music タブの ch10 表示時に `Piano Roll / Step Seq` 切替を追加し、7行x16ステップのドラムシーケンサーを実装。`ApplyStepSeqNotes` で ch9 ノートのみ差し替えて PianoRoll プロジェクトへ同期し、Play Preview / Export WAV 反映を維持。step grid/velocity/view state のワークスペース永続化（`stepSeqViewActive/stepSeqBits*/stepSeqVel*`）を追加
- Phase 2-G 実装: Music タブ Master Effects に `SampleRateReducer -> BitCrusher -> Chorus -> Flanger -> Delay -> Reverb` の6ブロックチェーン図を追加。DrawList 描画で ON/OFF 状態を可視化し、各ブロッククリックで有効/無効を直接トグル（SampleRateReducer は ratio、BitCrusher は bits でバイパス制御）
- GUI バグ修正: FX チェーン可視化で `GetCursorPosX/Y()`（ローカル座標）を `GetCursorScreenPos()`（スクリーン座標）に修正。ブロックが画面外に描画されていた問題とピアノロールレイアウト崩壊を解消
- Sound タブ 3 層縦スタック化: `BeginTable("layout_split", 2列)` を廃止し全幅 1 列化。Source Type / Preset 管理を Layer1 CollapsingHeader 内に統合、Waveform / Spectrum / VU Meter / Preview Note を Layer2 直下に移動し「発見→調整→詳細編集」の縦スタック構造を実現
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
