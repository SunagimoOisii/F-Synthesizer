# gui-help Hover Acceptance Checklist

Last Updated: 2026-03-05
Scope: Phase 8 manual verification

## Purpose

- Sound / Music の主要導線で `Help:` のホバー文言が欠落しないことを確認する
- 文言が `影響 -> 注意` を中心に表示されることを確認する

## Environment

- `build/x64/Debug/F-Synthesizer.exe` を起動
- `assets/midi/solstice_intro.mid` を使用可能

## Check Rules

- PASS: ホバーで意図どおり文言が表示される
- FAIL: 文言未表示、誤説明、順序崩れ（影響/注意）
- NOTE: 長文で読みにくい場合は短文化候補を記録

## Sound Tab

1. Top Controls
- [ ] `Play Preview (PR Channel using Selected Slot)`
- [ ] `Play Tone (C4)`
- [ ] `Loop Preview`
- [ ] `Stop`
- [ ] `Close`

2. Preset/Slot Controls
- [ ] `Preset` コンボ
- [ ] `Apply Preset Paths`
- [ ] `Reset Defaults`
- [ ] `Preset Name`
- [ ] `Save Preset As`
- [ ] `Duplicate Preset`
- [ ] `Reset Sound Slot`
- [ ] `Selected Sound Slot (0-15)`
- [ ] `Edit Assigned Slot of PR ch`
- [ ] `Default Wave`

3. Channel Editor (DrawChannelEditor)
- [ ] `Envelope / Gain` の各項目（Amp/Attack/Decay/Sustain/Release）
- [ ] `Source Type`
- [ ] Waveform主要項目（Wave/Unison/Filter/Smoothing/Modulation）
- [ ] Noise主要項目
- [ ] FM主要項目
- [ ] DrumKit主要項目

## Music Tab

1. Path / Reference / Target
- [ ] `MIDI Path` / `Browse MIDI...` / `Copy MIDI`
- [ ] `Output Path` / `Browse Output...` / `Copy Output`
- [ ] `Snapshot (Recommended)` / `Link (Advanced)`
- [ ] `All Channels` / `Single Channel` / `Target Ch`

2. Render / Mixer / Drum
- [ ] `Sample Rate` / `Initial Seconds` / `Bits`
- [ ] `Extra Release (sec)` / `Serial Save (timestamp suffix)`
- [ ] `Set PR Assign = Same slot index`
- [ ] `Set Output Target = PR ch`
- [ ] `Reset All Assign = Same slot index`
- [ ] `Enable ch10 Drum Guard`
- [ ] `Auto Setup ch10 Drum`
- [ ] `Focus PR ch10`
- [ ] Mixer table: `slot` / `M` / `S` / `Level` / `Pan` / `Gain`

## Error / Unsaved Dialogs

1. Error Inline / Modal
- [ ] `Recover: Browse MIDI`
- [ ] `Recover: Browse Output`
- [ ] `Recover: Go Sound Tab`
- [ ] `Recover: Go Music Tab`
- [ ] `Clear Error`
- [ ] `OK`
- [ ] `Dismiss`

2. Unsaved Changes
- [ ] `保存して続行`
- [ ] `保存せず続行`
- [ ] `キャンセル`

## Result Summary

- Date:
- Tester:
- Result: `PASS / CONDITIONAL PASS / FAIL`
- Notes:
  - 
  - 
