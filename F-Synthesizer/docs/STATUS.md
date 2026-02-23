# STATUS

Last Updated: 2026-02-23
Branch: `main`
Migration Progress: `GUI v7: DONE(FROZEN) / GUI v8: DONE`

## Current Snapshot

- 実装済み
  - MIDI 解析（Note/Tempo/CC/Pitch Bend）
  - tick -> sample 変換
  - SynthEngine 分割（`Engine.cpp` / `Events.cpp` / `Renderer.cpp` / `Voices.cpp`）
  - 音源（Waveform / Noise / FM / Drum / DrumKit）
  - ADSR と WAV 書き出し
  - 入力MIDIを `assets/midi/` から読む運用に変更
- 直近の大きな変更
  - ドラム合成処理を `Renderer.cpp` で整理（最新コミット付近）
  - `config/default.json` 導入（CLI設定ファイル読み込み）
  - `Run(const AppConfig&)` に実行コアを分離
  - `config/base.json` + `config/presets/*.json` + `--preset` を導入
  - GUI導入要件を `GUI_REQUIREMENTS.md` として明文化
  - 既定起動を GUI 化（`--cli` でCLI強制）
  - GUI: 非同期実行、ログパネル、`gui_state.json` 保存/復元、Serial Save トグル
  - GUI: 実行前バリデーション（MIDI/出力パス・sampleRate・bits 等）
  - GUI v0.2: channels設定ロード/保存（`source.type` 差分マージ）
  - GUI v0.2: チャンネルエディタ（ch0-15, source切替, ADSR）
  - GUI v0.2: プリセット運用（Save Preset As / Duplicate / Reset Channel / dirty表示）
  - GUI v0.2: 日本語パス対策（UTF-8変換、フォント読込、起動時補正）
  - プリセットを拡張（`basic_wave` / `fm_default`）し、基本波形を既定運用に整理
  - GUIプリセット選択を `config/presets/*.json` の自動列挙に変更（コード修正不要で追加可能）
  - GUI v0.3 Phase A-D: ミックス基盤（mute/solo/level/pan/gain）を実装
  - GUI v0.3 Phase A-D: ミックス監視UI、単体試聴導線、状態保存/復元を実装
  - GUI v0.3 Phase A-D: GUIプリセット適用をCLIと同じ `LoadConfigFile` 経路へ統一
  - GUI v0.4 Phase A: `RunMode/RenderOptions` を導入し、Preview/Export経路を分離
  - GUI v0.4 Phase B: GUI内Preview再生（`Play Preview` / `Replay` / `Loop` / `Stop`）
  - GUI v0.4 Phase C: `Stop` の実キャンセル（`IRunObserver::ShouldCancel`）を実装
  - GUI v0.4 Phase D: SynthEngine内部を SoA 化（Run I/F は互換維持）
  - GUI v0.5 Phase A-E: 可読性/状態表示/レイアウト/チャンネルUXを再構築
  - v0.4 -> v0.5 移行ガイド `docs/archive/migration/migration_v5.md` を追加（凍結）
  - Refactor Phase 1: 構造基準を確定（責務マップ、依存方向、命名規約、移動順リスト）
  - Refactor Phase 2: `main` と設定I/O責務を `app/config` 層へ分離
  - Refactor Phase 3: `midi` パイプラインと `core` レンダ境界を導入
  - Refactor Phase 3: `MIDIParser.cpp` / `Sequencer.cpp` を `src/midi/` へ再配置
  - running status 回帰スクリプト `scripts/midi_regression.ps1` を追加
  - Refactor Phase 4: `include/io/PlatformPaths.h` / `src/io/PlatformPaths.cpp` を追加し、Path/UTF変換と診断整形を共通化
  - Refactor Phase 4: `Writer` に `WavWriteError` を導入し、保存失敗時ログを `path + cause + hint + errno/winerr` 形式へ統一
  - Refactor Phase 4: `scripts/gui_smoke.ps1` に日本語パスの設定読込・WAV保存スモークを追加
  - Refactor Phase 5: GUI 分割に着手し、Preview 再生を `src/gui/PreviewAudio.cpp` へ分離
  - Refactor Phase 5: ファイルダイアログ/パスUI補助を `src/gui/GUIPlatform.cpp` へ分離
  - Refactor Phase 5: 音源変換/比較/プリセットJSON補助を `src/gui/GUIConfigUtils.cpp` へ分離
  - Refactor Phase 5: `gui_state.json` 読込/保存を `src/gui/GUIStateStorage.cpp` へ分離
  - Refactor Phase 5: 実行前バリデーション/出力WAVパス生成を `src/gui/GUIRunHelpers.cpp` へ分離
  - Refactor Phase 5: preset差分保存/一覧収集/適用読込を `src/gui/GUIPresetIO.cpp` へ分離
  - Refactor Phase 5: `GUIState` 定義と `BuildConfig/EnsureChannel` を `src/gui/GUIStateModel.cpp` へ分離
  - Refactor Phase 5: state初期化/保存値修復（`Init`/`Repair`）を `src/gui/GUIStateModel.cpp` へ分離
  - Refactor Phase 5: チャンネル編集UI（`DrawChannelEditor`）を `src/gui/GUIChannelEditor.cpp` へ分離
  - Refactor Phase 5: 実行制御/プリセット適用/ログ解析を `src/gui/GUIActions.cpp` へ分離
  - Refactor Phase 5: `gui_state.json` モデル変換と保存経路を `src/gui/GUIStatePersistence.cpp` へ分離
  - Refactor Phase 5: `GUIMain.cpp` を描画中心へ整理（約 834 行 -> 488 行）
  - Refactor Phase 6: `ConfigFileIO.cpp` を公開I/Fラッパー化し、読込・JSON補助・保存整形を `ConfigLoad/ConfigJsonUtils/ConfigSave` へ分割
  - Refactor Phase 7: `SoundGenerate.cpp` を API ラッパー + `main` へ簡素化し、実行フロー/統計/保存制御を `src/app/Run*` へ分割
  - Refactor Phase 8: `check/gui_smoke` を新構成へ追従し、`docs/archive/migration/migration_refactor.md` を追加（凍結）
  - Comment Migration Phase 1: `docs/archive/comment-migration/COMMENT_MIGRATION_BASELINE.md` を追加し、対象一覧/優先順位/必須コメント抽出/レビュー判定基準を固定
  - Comment Migration Phase 2: `src/SynthEngine/*` / `src/midi/*` / `include/SynthEngine/*` を中心に、複雑分岐・最適化・型概要の日本語コメントを追加
  - Comment Migration Phase 3: `src/app/*` / `src/config/*` と `include/AppCore.h` を中心に、実行境界・設定読込境界・CLI互換背景の日本語コメントを追加
  - Comment Migration Phase 4: `src/gui/*` / `src/io/*` と `include/gui/*` / `include/io/*` を中心に、状態遷移・Preview導線・UTF/Path診断の日本語コメントを追加
  - Comment Migration Phase 5: 規約適合の横断レビューを実施し、`scripts/gui_smoke.ps1` 13ステップ通過を確認（2026-02-21）
  - Comment Migration Phase 5: 用語明確性ルールを `docs/COMMENT_GUIDELINE.md` に追加し、Phase3/4コメントを平易語へ整備
  - Render最適化: `SynthEngine` で可聴チャンネル判定（mute/solo/mixGain）を事前計算し、ホットループ分岐を削減
  - Render最適化: `VoicesSoA::CleanupPending` の作業バッファを `RenderState` 再利用へ変更し、周期クリーンアップ時の割り当てを削減
  - ピアノロール計画: `docs/archive/piano-roll-migration/PIANO_ROLL_PHASES.md` / `docs/archive/piano-roll-migration/PIANO_ROLL_TASKS.md` を追加（MVP: 表示 + 基本編集 + Preview反映）
  - ピアノロール計画: 要件選択（時間単位/編集対象/保存形式/Undo単位/非目標/テスト方針）を `docs/archive/piano-roll-migration/PIANO_ROLL_PHASES.md` に確定
  - ピアノロール Phase 1: `PianoRollState` と表示基盤（グリッド/鍵盤/ノート描画、ch切替、ズーム/スクロール）を実装
  - ピアノロール受け入れ: 手動テスト手順 `docs/archive/piano-roll-migration/PIANO_ROLL_ACCEPTANCE_TEST.md` を追加
  - ピアノロール受け入れ: `scripts/piano_roll_smoke.ps1` を追加（`gui_smoke` + 手動確認ハーネス）
  - プロダクト方針: `docs/PRODUCT_POLICY.md` を追加（価値/対象ユーザー/非目標を明文化）
  - GUI v7 Phase D: Musicタブに16chミキサー（Mute/Solo/Level/Pan/Gain）と音色割当（ch->source ch）を追加
  - GUI v7 Phase D: Musicタブに書き出し対象（All/Single）UIを追加し、ch10ドラム特別扱い（Auto Setup/警告表示）を追加
  - GUI v7 Phase D: `channelAssignments` と `drumChannelSpecialHandling` を `gui_state` 保存/復元へ対応
  - GUI v7 Phase E: 未保存状態の終了/プリセット切替に警告ダイアログを追加（保存して続行/保存せず続行/キャンセル）
  - GUI v7 Phase E: Save Targets 表示と、エラーUX（ログ + 画面内エラー表示 + 修正アクション + ダイアログ）を追加
  - GUI v7 Phase F: 手動受け入れ手順 `docs/GUI_V7_ACCEPTANCE_TEST.md` を追加
  - GUI v7 Phase F: `scripts/gui_smoke.ps1` を15ステップへ拡張（単一チャンネル書き出し経路を追加）
  - GUI v7 リリース判定: 自動スモークは通過、最終判定は手動受け入れ完了後に確定（保留）
  - GUI v7 Phase G: v8影響レビューを実施し、Soundタブ重複責務（MIDI/Output/Target）をv8 Phase A先行縮退へ格上げ
  - GUI v8 Phase A: Soundタブの重複責務（MIDI/Output/Target）を縮退し、Musicへ責務集約
  - GUI v8 Phase B: 保存導線（Save Project/Save All）と snapshot/link 表示を整理
  - GUI v8 Phase C: ドラムch特別扱いを必要最小限へ整理し、重複機能を撤去
  - GUI v8 Phase D: エラー導線を `Problem / Suggested Fix / Recover:* / Clear Error` へ統一
  - GUI v8 Phase E: v8手動受け入れ手順 `docs/GUI_V8_ACCEPTANCE_TEST.md` を追加し、リリース判定を記録
  - Source設計改善: `config/SourceRegistry` を導入し、source type 名/表示名/既定値の分散定義を一元化
  - Source設計改善: GUIプリセット保存の `source` JSON 出力を `config/SourceJson` 経由へ統一し、drumkit差分保存での項目欠落リスクを解消
  - Source設計改善: `ConfigLoad` の `source.type` 判定を registry ベースへ移行し、方式追加時の分岐散在を縮小
  - Waveform Phase A: `WaveformConfig` に `quality(legacy/bandLimited)` を追加し、`saw/square` で band-limited（polyBLEP）生成を導入
  - Waveform Phase A: `source.type=waveform` の JSON load/save へ `quality` を追加し、legacy と band-limited の切替を設定可能化
  - Modulation基盤: `ModulationConfig` の JSON load/save（`lfo1/env2/routes`）とバリデーションを実装
  - Modulation基盤: Soundタブ（Waveform）から modulation 編集（LFO1/Env2/Route）を実装
  - Modulation基盤: preset diff 比較へ modulation 項目を反映
  - Modulation基盤: 無効route時の早期returnと必要sourceのみ評価する低コストスキップを実装
  - Parameter Smoothing: 共通モジュール（`Smoothing.h/.cpp`）を追加し、Waveform経路（amp/pitch/filterCutoff）へ接続
  - Parameter Smoothing: Waveform `source.smoothing` の Config/GUI/preset diff 接続を実装
  - Parameter Smoothing Phase E: Noise/FM/Drum の接続可否を判断し、当面は未接続で方針固定（理由を方式別mdへ記録）
  - Parameter Smoothing Phase F: ABテスト用MIDI（`assets/midi/smoothing_phaseF_ab.mid`）を作成し、同一条件ON/OFF比較を運用手順として固定
  - Wave/Drum Brushup Phase B: waveform preset（bass2/lead2）を更新し、filter/modulation/smoothing を含む実用レンジへ再調整
  - Wave/Drum Brushup Phase C: drum基準preset（kick/snare/hat）と共存AB設定（before/after）を追加し、waveform同居時のドラム過大レベルを抑制
  - Wave/Drum Brushup Phase D: 統合AB設定（`phaseD_wave_drum_integrated_before/after`）を追加し、kick/bass と snare/lead の帯域衝突を再配分
  - Wave/Drum Brushup Phase E: AB実行手順と結果記録テンプレートを `docs/synth-migration/WAVE_DRUM_AB_RUNBOOK.md` に固定
- 品質確認
  - `Debug x64` ビルド成功（2026-02-21）
  - `scripts/gui_smoke.ps1` をv0.5向けに拡張（既定GUI起動 + `--gui` 明示起動）
  - `scripts/gui_smoke.ps1` 13ステップ通過（2026-02-21, config/preset/invalid/japanese path含む）
  - `scripts/gui_smoke.ps1` 15ステップ通過（2026-02-23, 単一ch書き出し + 受け入れハンドオフ追加）
  - Modulation AB確認（2026-02-23）: `assets/midi/test.mid` で ON/OFF 比較を実施し、normal/extreme の両ケースで差分を確認（clip 0）
  - Parameter Smoothing AB確認（2026-02-23）: `assets/midi/smoothing_phaseF_ab.mid` で ON/OFF 比較を実施し、`diff_rms=0.027361`、clip 0 を確認
  - Waveform Phase B AB確認（2026-02-23）: before/after 比較で bass `diff_rms=0.011260`、lead `diff_rms=0.026905`、clip 0 を確認
  - Wave/Drum Brushup Phase C AB確認（2026-02-23）: `assets/midi/wave_drum_phaseC_ab.mid` で before/after 比較を実施し、peak `0.340754 -> 0.150017`、clip 0 を確認
  - Wave/Drum Brushup Phase D AB確認（2026-02-23）: `assets/midi/wave_drum_phaseC_ab.mid` で before/after 比較を実施し、peak `0.277023 -> 0.175695`、rms `0.044319 -> 0.0313929`、clip 0 を確認

## Priority Issues

1. `MidiParser`: running status 修正に対する回帰テスト追加
2. `SynthEngine`: 同一ノート重なり時の NoteOff 対象ずれ修正
3. `Writer`: 16bit 以外指定時のヘッダ/実データ不整合修正

## Next Actions

1. `MidiParser`: running status 修正に対する回帰テスト追加
2. `SynthEngine`: 同一ノート重なり時の NoteOff 対象ずれ修正
3. `Writer`: 16bit 以外指定時のヘッダ/実データ不整合修正

## Resume Commands

```powershell
git status --short --branch
git log --oneline --decorate -n 12
.\scripts\check.ps1 -SkipBuild -SkipRun
```

## Notes

- 詳細設計は `Architecture.md`
- プロダクト方針は `PRODUCT_POLICY.md`
- 拡張案とメモは `NOTES.md`
- 実行入口は `src/SoundGenerate.cpp`
- GUI準備要件は `GUI_REQUIREMENTS.md`
- GUIスモークテストは `scripts/gui_smoke.ps1`
- GUI v0.3 計画は `GUI_MIGRATION_PHASES_v3.md` / `GUI_MIGRATION_TASKS_v3.md`
- GUI v0.4 計画は `GUI_MIGRATION_PHASES_v4.md` / `GUI_MIGRATION_TASKS_v4.md`
- GUI v0.5 計画は `GUI_MIGRATION_PHASES_v5.md` / `GUI_MIGRATION_TASKS_v5.md`
- リファクタ計画は `REFACTOR_PHASES.md` / `REFACTOR_TASKS.md`
- コメント移行計画は `docs/archive/comment-migration/COMMENT_MIGRATION_PHASES.md` / `docs/archive/comment-migration/COMMENT_MIGRATION_TASKS.md`（凍結）
- コメント移行Phase1成果物は `docs/archive/comment-migration/COMMENT_MIGRATION_BASELINE.md`（凍結）
- コメント移行の運用基準は `docs/COMMENT_GUIDELINE.md`
