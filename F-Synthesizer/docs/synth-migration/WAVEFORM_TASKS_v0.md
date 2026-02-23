# WAVEFORM_TASKS

このファイルは `docs/waveform-migration/WAVEFORM_PHASES.md` の実装チェックリスト。

## Global Rules（全Phase共通）

- [x] 対象範囲を waveform 改善に限定している
- [x] 優先機能は `band-limited`, `unison`, `sub-osc` に集中している
- [x] ベース/リード用途の改善を優先している
- [x] 実装順が `内部実装 + JSON` -> `GUI対応` になっている
- [ ] 各Phaseで専用テストMIDIを作成している
- [ ] 各Phaseで A/B の2本を必ずレンダしている
- [ ] AB所見（差が出る秒数・聴感メモ）を残している

### AB運用テンプレート（各Phaseで記入）

- テストMIDI:
- A条件:
- B条件:
- 差が聴こえる位置（秒）:
- 判定:

## Phase A: Band-limited Foundation

- [x] 現行 oscillator 実装の aliasing 出やすい条件を整理
- [x] `saw` の band-limited 実装を追加
- [x] `square` の band-limited 実装を追加
- [x] waveform source に品質モード（legacy / band-limited）を追加
- [x] config load/save（`source.type=waveform`）へ新パラメータを追加
- [x] preset差分保存で新パラメータが保持されることを確認
- [x] Phase A専用テストMIDIを作成
  - [x] `assets/midi/phaseA_wave_alias_high.mid`
  - [x] `assets/midi/phaseA_wave_bass_repeat.mid`
  - [x] `assets/midi/phaseA_wave_lead_phrase.mid`
- [x] 3曲それぞれで A/B レンダを作成
  - [x] `output/phaseA_wave_alias_high_A.wav` / `output/phaseA_wave_alias_high_B.wav`
  - [x] `output/phaseA_wave_bass_repeat_A.wav` / `output/phaseA_wave_bass_repeat_B.wav`
  - [x] `output/phaseA_wave_lead_phrase_A.wav` / `output/phaseA_wave_lead_phrase_B.wav`

完了条件:

- [x] Phase A専用テストMIDIで aliasing 低減を耳で確認

## Phase B: Unison + Sub-osc Core

- [x] waveform source に unison パラメータを追加（voice数, detune, spread）
- [x] unison ボイスの位相/ピッチ更新を実装
- [x] sub-osc（-1 octave）を実装
- [x] レベル管理（クリップしにくい合成比）を調整
- [x] config load/save に unison/sub-osc パラメータを追加
- [x] preset差分保存の保持を確認
- [x] Phase B専用テストMIDIを作成
  - [x] `assets/midi/phaseB_wave_unison_chord.mid`
  - [x] `assets/midi/phaseB_wave_subosc_bass.mid`
- [x] Phase B専用テストMIDIで A/B レンダを作成
- [x] AB所見を記録（秒数付き）

AB所見:

- テストMIDI: `assets/midi/phaseB_wave_unison_chord.mid`
- A条件: `unisonVoices=8`, `unisonDetuneCents=80.0`, `unisonSpread=1.0`（sine）
- B条件: `unisonVoices=1`, `unisonDetuneCents=0.0`, `unisonSpread=0.0`（sine）
- 差が聴こえる位置（秒）: 0.0-2.0（和音の揺れ/厚み）
- 判定: Aの方が和音の厚みが増える

- テストMIDI: `assets/midi/phaseB_wave_subosc_bass.mid`
- A条件: `subOscLevel=1.0`, `unisonVoices=2`, `unisonDetuneCents=4.0`
- B条件: `subOscLevel=0.0`, `unisonVoices=1`, `unisonDetuneCents=0.0`
- 差が聴こえる位置（秒）: 0.0-4.0（低域の芯）
- 判定: Aの方が低域の芯と重みが増える

完了条件:

- [x] ベース/リードで厚み改善を耳で確認

## Phase C: JSON / Preset Tuning

- [x] ベース向け waveform プリセットを追加（最低2種）
- [x] リード向け waveform プリセットを追加（最低2種）
- [x] Phase C専用テストMIDIを作成
  - [x] `assets/midi/phaseC_wave_preset_bass.mid`
  - [x] `assets/midi/phaseC_wave_preset_lead.mid`
- [x] Phase C専用テストMIDIで A/B レンダを作成
- [x] 失敗パターン（薄い/痛い/埋もれる）をメモ化
- [x] AB所見を記録（秒数付き）

AB所見:

- テストMIDI: `assets/midi/phaseC_wave_preset_bass.mid`
- A条件: `wave_bass_solid` 相当（saw + sub強め + unison少なめ）
- B条件: `wave_bass_wide` 相当（square + unison多め + sub控えめ）
- 差が聴こえる位置（秒）: 0.0-3.5
- 判定: Aは芯が太く、Bは広がりが強い

- テストMIDI: `assets/midi/phaseC_wave_preset_lead.mid`
- A条件: `wave_lead_bright` 相当（saw + detune強め）
- B条件: `wave_lead_soft` 相当（triangle + detune控えめ）
- 差が聴こえる位置（秒）: 0.0-4.0
- 判定: Aはエッジが立ち、Bは丸い輪郭

失敗パターンメモ:

- `subOscLevel` を上げすぎると、低音域で主音程より下が目立ちすぎて輪郭がぼやける
- `unisonDetuneCents` を10以上にすると、ベースで音程感が不安定になりやすい
- `square` + `unisonVoices` 多めでは高域が痛くなりやすく、`amp` を抑える前提が必要

完了条件:

- [x] 音作り開始時に使えるプリセット群が揃っている

## Phase D: GUI Enablement

- [x] band-limited を常時有効方針に統一（GUIトグルは追加しない）
- [x] GUI の Source Details に unison 設定を追加
- [x] GUI の Source Details に sub-osc 設定を追加
- [x] GUI保存 -> JSON読込 -> 再表示で値一致を確認
- [x] 操作導線（並び順、初期値、入力幅）を調整
- [x] Phase D専用テストMIDIを作成（`assets/midi/phaseD_wave_gui_edit_regression.mid`）
- [x] GUI編集値とJSON編集値で A/B レンダを作成
- [x] AB所見を記録（秒数付き）

AB所見:

- テストMIDI: `assets/midi/phaseD_wave_gui_edit_regression.mid`
- A条件: GUI編集値相当（`phaseD_wave_gui_edit_regression_A.json`）
- B条件: JSON編集値（`phaseD_wave_gui_edit_regression_B.json`）
- 差が聴こえる位置（秒）: 差分なし
- 判定: A/B一致（`fc /b` 同一、SHA256同一）

完了条件:

- [x] GUI単体で waveform 改善機能を完結操作できる

## Phase E: Acceptance

- [x] 補助確認として `assets/midi/test.mid` の AB 比較を実施(ドラムはなし)
- [x] 補助確認として `assets/midi/test2.mid` の AB 比較を実施(ドラムはなし)
- [x] 改善判断ログ（主観メモ）を残す
- [x] `docs/synth-methods/waveform.md` を更新
- [x] 未達項目を次タスクへ切り出す

AB所見:

- テストMIDI: `assets/midi/test.mid`（`targetChannel=0`）
- A条件: `phaseE_wave_test_mid_A.json`（unison/sub-osc ON）
- B条件: `phaseE_wave_test_mid_B.json`（unison/sub-osc OFF）
- 差が聴こえる位置（秒）: 6.0-20.0 付近（主旋律進行で確認しやすい）
- 判定: Aは厚みと密度が増し、Bは輪郭が単純

- テストMIDI: `assets/midi/test2.mid`（`targetChannel=0`）
- A条件: `phaseE_wave_test2_mid_A.json`（unison/sub-osc ON）
- B条件: `phaseE_wave_test2_mid_B.json`（unison/sub-osc OFF）
- 差が聴こえる位置（秒）: 3.0-12.0 付近
- 判定: Aは奥行きが増え、Bは前に出るが薄め

改善判断ログ（主観メモ）:

- `test.mid` はA条件の方が「ピコピコ感」が減り、音像の厚みが出る
- `test2.mid` はA条件で密度が上がる一方、速いフレーズではやや飽和感が出る

次タスク（未達項目の切り出し）:

- `unison/sub-osc` 推奨レンジを用途別（bass/lead）にプリセット注釈へ明記
- 高密度フレーズ向けに `amp` 自動抑制ルール（またはリミッタ相当）を検討

完了条件:

- [x] 「適当なピコピコ音ではない」水準に到達したと合意
