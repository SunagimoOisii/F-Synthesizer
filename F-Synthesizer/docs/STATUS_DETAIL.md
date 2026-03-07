# STATUS_DETAIL

Last Updated: 2026-03-08 (foundation: ParameterSchema 拡張項目の導入可否を判断)
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
  - `config/base.json` + `presets/*.json` + `--preset` 導入、`Run(const AppConfig&)` に実行コアを分離
  - 既定起動を GUI 化（`--cli` でCLI強制）
  - GUI v0.2-v0.5: チャンネル編集、プリセット運用、日本語パス対策、ミックス基盤、Preview再生、SoA化、レイアウト再構築
  - Refactor Phase 1-13: 責務分割（app/config/midi/core/gui/pianoroll）、ランタイム追跡整理
  - Comment Migration Phase 1-5: 高リスク箇所への日本語コメント追加、用語ルール導入
  - ピアノロール Phase 1: 表示基盤（グリッド/鍵盤/ノート描画、ch切替、ズーム/スクロール）
  - GUI v7: Musicタブ16chミキサー、書き出し対象UI、未保存警告、エラーUX
  - GUI v8: Soundタブ縮退、保存導線整理、ドラム整理、エラー導線統一
  - Source設計改善: `config/SourceRegistry` 導入、プリセット保存統一、ConfigLoad registry移行
  - Waveform Phase A: band-limited（polyBLEP）生成導入、`quality` 設定追加
  - Modulation基盤: LFO1/Env2/Route の load/save/GUI編集、低コストスキップ
  - Parameter Smoothing: 共通モジュール追加、Waveform経路接続（Noise/FM/Drumは未接続で方針固定）
  - Wave/Drum Brushup: preset再調整、ドラムレベル抑制、帯域再配分、AB手順固定
  - GUI UX #3: DrumKit Type 連動パラメータ表示（Kick / Snare・Hat / None で表示切替）
  - GUI UX #5: WaveformConfig FilterMode デフォルトを Bypass に変更、フィルタ有効時の `(active)` インジケーター追加
  - GUI UX #6: モジュレーションルートを CollapsingHeader 化、ヘッダに "lfo1 -> pitch (+0.50)" 形式のサマリを表示
  - GUI UX #7: `Channel(ch)` と `Sound Slot(s)` の表記を整理。Musicプレビュー時に `Selected Sound Slot` が自動変更されないよう挙動を修正
  - SynthEngine #2: `noteInstanceID` を `MIDIParser -> Sequencer -> VoicesSoA` へ通線し、`MarkNoteOff` をID優先照合へ切替（不一致時は旧 `ch+note` へフォールバック）
  - SynthEngine #3: Filter cutoff の微小更新を間引き、係数再計算のホットパスCPU負荷を低減（1 cent相当未満をスキップ）
  - cleanup Phase 1-6: `scripts`/`docs`/`gui`/`midi`/`core` を監査し、削除方針を `docs/cleanup/deletion-policy.md` に確定。`scripts/piano_roll_smoke.ps1` を削除し、`AudioBuffer.cpp` の未使用/冗長処理を整理
  - doc-sync: `ROADMAP.md` を廃止して `STATUS.md` へ一本化。`PIANO_ROLL_CONTROLS.md` は `GUI_REQUIREMENTS.md` へ、`WEEKLY_MAINTENANCE.md` は `OPERATIONS.md` へ統合
  - comments Step 1: `src/midi` / `include/midi` を `COMMENT_GUIDELINE` 準拠で更新（境界条件・失敗時挙動・フォールバック理由の注釈を追加）
  - comments Step 2: `src/SynthEngine` / `include/SynthEngine` を更新（CC/Pitch境界値処理とキャンセル境界の注釈を追加）
  - comments Step 3: `src/app` + `include/AppCore.h` を更新（Run戻り値契約、CLI起動優先順位、プロジェクトルート探索失敗時の境界を注釈）
  - comments Step 4: `src/gui` / `include/gui` を更新（非同期Run状態遷移、GUI状態保存I/O、Preview命名規則の境界を注釈）
  - comments Step 5: `src/config` / `include/config` を更新（設定解決優先順位、公開I/F契約、種別フォールバックの境界を注釈）
  - comments Step 6(io): `src/io` / `include/io` を更新（WAV書き出し失敗診断、パス解決規約、公開I/F契約を注釈）
  - comments Step 7(synth): `src/synth` / `include/synth` を更新（ADSR時間遷移、FM/Noise前提、thread_local状態の境界を注釈）
  - comments Step 8(core): `src/core` / `include/core` を更新（SoundData境界契約、RenderGateway副作用、既定バッファ初期値を注釈）
  - gui-help Phase 2: `MainWindow.inl` のホバー導線を `updateHoverHelp(what, impact, caution)` に統一し、文言順を `何をする -> 影響 -> 注意` へ統一
  - gui-help Phase 3: Music高リスク領域（Path / Output Target / Drum / Mixer主要列）へヘルプを追加
  - gui-help Phase 4: Render Settings の意図説明と、Play Preview / Tone Preview / Export WAV / Stop の差分説明を補強
  - gui-help Phase 6: Sound Preset操作/エラー復旧/未保存確認ダイアログへヘルプを追加し、Music補助ボタンの未付与分（Assign/Reset/Focus）を補完
  - gui-help Phase 7: `DrawChannelEditor` へホバー更新導線を接続し、内部の主要編集UI（Envelope/Source/Wave/Filter/Smoothing/Mod/Noise/FM/Drum）へヘルプを追加
  - gui-help Phase 8: 手動ホバー受け入れ用チェックリスト `docs/gui-help-hover-acceptance-checklist.md` を作成（実機確認項目を網羅）
  - gui-help Phase 9: ホバー文言契約を `docs/GUI_REQUIREMENTS.md` へ昇格し、計画台帳を `docs-archive/gui-help/gui-help-phase-plan.md` へ移管
  - feat-infra: `scripts/midi_regression.ps1` を絶対パスconfig化して実行ディレクトリ依存を解消し、`scripts/check.ps1 -RunMIDIRegression` を追加
  - infra-fix: `scripts/update_architecture_notes.ps1` の文字コード依存で発生するPowerShell 5.1構文崩れを回避（非ASCII文字列をASCII化）
  - acronym-case: 略称表記を段階的に大文字統一（`MidiPipeline -> MIDIPipeline`, `MIDI/WAV/CLI/JSON` 系の型名・関数名・ファイル名・参照を更新）
  - runtime cleanup: 未使用だった `Default Wave` 導線を廃止（`AppConfig.defaultWave` / `MIDIEvent.typeWave` / GUI `Default Wave` UI / Config保存キー出力）
  - gui cleanup: 未接続だった `Sound Reference (Snapshot/Link)` を廃止（MusicタブUI、`assetReferenceMode/showReferenceAdvanced` 状態、GUI state保存キー）
  - gui cleanup: Musicタブ補助文言/ヘルプを現行UIへ再整理（Reference廃止後の説明差分を反映）
  - doc-sync: 個人運用前提で `method-boundaries` / `integration-playbook` / `STATUS` の記載整合を最終確認
  - ops: 重い自動ハーネスを採用せず、`check.ps1` + 代表MIDI手動確認の軽量運用を `OPERATIONS.md` に明記
  - ops: Weekly Maintenance を実施し、`docs/architecture/*.md` の `TODO (auto-generated)` 7件を ADR 記入で解消
  - foundation: capability / lifecycle / schema 契約の運用ルールを `docs/architecture/README.md` / `HANDBOOK.md` / `module-map.md` / `config-and-io.md` に反映
  - foundation: lifecycle 契約（2.5）の最小受け皿を追加（`SourceLifecyclePolicy` 定義 + `source.lifecycle` 整合検証）
  - foundation: `SourceKind -> ParameterSchema[]` を Waveform/Noise/FM/Drum（DrumKit は空schema特例）へ拡張
  - foundation: `LoadSource.cpp` の schema 駆動検証を FM/Drum/DrumKit へ段階移行（DrumKit は noteごとに Drum schema を適用）
  - foundation: Noise enum は「parse で文字列解決 + schema で値ドメイン検証」の方針で統合
  - foundation: modulation destination の `pan` は現時点で非採用（ConfigLoad 非受理）と確定
  - foundation: 方式固有 destination 拡張規約（`<sourceKind>.<parameterId>`、例: `fm.index`）を定義
  - foundation: 方式固有 destination の Phase 1 として `fm.index` を採用（FM source限定で受理・適用）
  - foundation: 方式固有 destination の GUI編集導線を導入（FMの Modulation Destination で `fm.index` を選択可能）
  - foundation: capability ベース分岐を点検し、Soundタブ source 種別選択と voice同種判定の SourceKind 直依存を置換
  - foundation: ParameterSchema の `displayName` / `smoothable` / `automatable` は当面非導入（最小版 `id/type/range/default` を正）と判断
  - foundation: lifecycle 実装挙動を監査（retrigger/steal/one-shot終了）。`Waveform/Noise/FM` の retrigger と steal 実装に差分を確認
  - foundation: `Waveform/Noise/FM` の retrigger を `SourceLifecyclePolicy`（restart）へ一致（同一note積み増しを抑止）
  - foundation: voice上限（256）到達時の steal 優先順位を `SourceLifecyclePolicy` に沿って実装（`Oldest` 差し替え / `RejectNew` 破棄）
- 品質確認
  - `Debug x64` ビルド成功（2026-02-21）
  - `scripts/gui_smoke.ps1` 15ステップ通過（2026-02-23）
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-03）
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-05、gui-help Phase 2-5 反映後）
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-05、gui-help Phase 6 反映後）
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-05、gui-help Phase 7 反映後）
  - `scripts/midi_regression.ps1` 通過（2026-03-05、docs/ 配下から実行してCWD非依存を確認）
  - `scripts/check.ps1 -SkipBuild -SkipRun -RunMIDIRegression` 通過（2026-03-05）
  - `scripts/midi_regression.ps1` 通過（2026-03-03、running status 2件 + overlap same note 1件）
  - Modulation / Smoothing / Waveform / Wave+Drum AB確認済み（2026-02-23、全て clip 0）
  - 手動ホバー確認は GUI 対話操作が必要なため、この実行環境では未実施（要ローカル実機確認）
  - 実装監査メモ（2026-03-05）: GUIと実データ反映の対応関係を確認
    - `Drum Gain`: `Renderer.cpp` の Drum 経路で `drumGain` として乗算され、プレビュー/書き出し双方へ反映されることを確認
    - `Sound Reference (Snapshot/Link)`: 実行分岐未接続のため、2026-03-05 に機能自体を廃止
  - 実装監査メモ（2026-03-05 / doc-sync追記）
    - `MIDI Path` / `Output Path`: `BuildConfigFromGUI` -> `Run` へ反映（`RunExecution.cpp` でMIDI読込 / 出力先利用）
    - `Output Target (All/Single, Target Ch)`: `state.targetChannel` -> `BuildMIDIPipeline(... targetChannel ...)` に反映
    - `Render Settings`:
      - `Sample Rate` / `Bits`: `SoundData(soundLength, bits, sampleRate)` と Previewデバイス初期化に反映
      - `Initial Seconds` / `Extra Release`: `RunExecution.cpp` の `soundLength` 計算へ反映
      - `Serial Save`: Export時のみ `BuildSerialWAVPath` へ反映（Previewは非対象）
    - `Music Mixer / Assignment`:
      - `assignCh*`: `BuildConfigFromGUI` で `channelConfigs` を再マップして反映
      - `Mute/Solo/Level/Pan/Gain`: `channelMixStates` として `RenderWithEngine` に反映
    - `Play Preview` / `Tone Preview`:
      - `Play Preview`: PianoRoll編集ノートを `overrideNoteTicks` で反映
      - `Tone Preview`: `selectedSoundSlot` + `selectedDrumNote` から単音ノート列を生成して反映
    - 表示専用（音データ非反映）:
      - `UI Scale`, `logPanelHeight`, `runLogTab` はUI表示状態のみを変更

## Backlog

- `doc-sync`: Musicタブ導線変更（Reference廃止）に伴う説明文/ガイド差分を点検する

## Recurring Checks

- `doc-sync`: 実装変更後に `Architecture.md` / `README.md` / `STATUS.md` の差分同期を確認する

## Notes

- 詳細設計は `Architecture.md`
- プロダクト方針は `PRODUCT_POLICY.md`
- 設計判断ログは `DECISIONS.md`
- 音色パラメータ要約は `SOUND_PARAMETERS_SUMMARY.md`（詳細は `SOUND_PARAMETERS.md`）
- 実行入口は `src/SoundGenerate.cpp`
- GUI準備要件は `GUI_REQUIREMENTS.md`
- コメント規約は `agents/standards/COMMENT_GUIDELINE.md`
- GUIスモークテストは `scripts/gui_smoke.ps1`
- 再開手順は `AGENTS.md` の Start Here / Useful Commands を参照
- 過去バージョン計画・移行記録は `docs-archive/` 配下を参照
