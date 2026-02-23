# WAVEFORM_PHASES

Waveform 改善計画（ベース/リード重視）

作成日: 2026-02-23
状態: Phase E In Progress

## 前提

- 対象範囲は `waveform` 専用改善に限定する。
- 優先機能は `band-limited`、`unison`、`sub-osc`。
- 主戦場はベース/リード。
- CPU は「品質優先」で、必要なら負荷増を許容する。
- 互換性は「品質優先」で破壊的変更を許容する。
- 実装導線は `内部実装 + JSON` を先に行い、その後 `GUI対応` を行う。
- 受け入れは耳での AB 比較を必須とする。

## フェーズ共通テスト運用（必須）

- 各フェーズで、そのフェーズ専用のテストMIDIを新規作成する。
- 各テストMIDIについて `A=新方式` / `B=比較基準` の2本を必ずレンダする。
- 命名規則:
  - MIDI: `assets/midi/phase<PhaseLetter>_wave_<purpose>.mid`
  - WAV(A/B): `output/phase<PhaseLetter>_wave_<purpose>_A.wav`, `output/phase<PhaseLetter>_wave_<purpose>_B.wav`
- 比較基準:
  - Phase A は `A=bandLimited`, `B=legacy`
  - Phase B 以降は「そのフェーズで追加した機能 ON/OFF」を A/B とする
- AB結果は、短い所見（何秒付近で差が聴けるか）を `WAVEFORM_TASKS.md` に記録する。

## フェーズ分割の観点

フェーズ数は固定しない。以下の観点で分割する。

1. 音質の土台（aliasing を抑える）を最初に成立させる
2. 音の厚み（unison/sub-osc）を次に追加する
3. 方式の利用性（preset/JSON運用）を整える
4. GUI導線は後段で追加し、音質コアの検証を先行する

## WAVEFORM_PHASE_A_BANDLIMITED_FOUNDATION

Status: DONE

- `saw/square` を中心に band-limited 実装を導入する。
- 高音域での aliasing 低減を優先し、ベース/リードで破綻しにくい土台を作る。
- まずは内部レンダ経路に実装し、JSON で切替可能にする（GUIはまだ追加しない）。
- 実装済み:
  - `WaveformConfig` に `quality`（`legacy` / `bandLimited`）を追加
  - `saw/square` に polyBLEP ベースの band-limited 生成を導入
  - JSON load/save（`source.type=waveform`）に `quality` を追加
- 専用テストMIDI:
  - `phaseA_wave_alias_high.mid`
  - `phaseA_wave_bass_repeat.mid`
  - `phaseA_wave_lead_phrase.mid`

完了条件:

- 既存 waveform と band-limited waveform の切替が可能
- Phase A専用MIDIの A/B で、高音域の耳障りな折り返し成分が減ったと判断できる

## WAVEFORM_PHASE_B_UNISON_SUBOSC_CORE

Status: DONE

- waveform source に `unison`（voice数, detune, spread）を追加する。
- `sub-osc`（-1 octave）を追加し、ベース/リードの芯と厚みを強化する。
- 実装は内部 + JSON を先行し、音色設計を先に回せる状態にする。
- 実装済み:
  - `WaveformConfig` に `unisonVoices/unisonDetuneCents/unisonSpread/subOscLevel` を追加
  - waveform レンダに unison 合成（1..8 voice, cent detune, phase spread）を追加
  - waveform レンダに sub-osc（-1 octave）混合を追加
  - JSON load/save と preset差分比較に新パラメータを反映
- 専用テストMIDI（作成対象）:
  - `phaseB_wave_unison_chord.mid`（和音の厚み検証）
  - `phaseB_wave_subosc_bass.mid`（低域の芯検証）

完了条件:

- unison/sub-osc パラメータを JSON で制御できる
- Phase B専用MIDIの A/B で、単音/和音の厚み改善が耳で確認できる

## WAVEFORM_PHASE_C_JSON_PRESET_TUNING

Status: DONE

- `config` の load/save と preset 差分保存を新パラメータへ対応する。
- ベース向け/リード向けの waveform プリセットを追加する。
- 音量飽和・位相・デチューン量の実運用値を詰める。
- 実装済み:
  - ベース向けプリセットを2種類追加
    - `wave_bass_solid`
    - `wave_bass_wide`
  - リード向けプリセットを2種類追加
    - `wave_lead_bright`
    - `wave_lead_soft`
  - Phase C専用MIDIとA/B用sample configを追加し、差分レンダを確認
- 専用テストMIDI（作成対象）:
  - `phaseC_wave_preset_bass.mid`
  - `phaseC_wave_preset_lead.mid`

完了条件:

- 新パラメータが preset 保存/再読込で保持される
- ベース/リード用の再利用可能プリセットが最低各2種類ある
- Phase C専用MIDIの A/B で、プリセット間の狙い差が聴き分けられる

## WAVEFORM_PHASE_D_GUI_ENABLEMENT

Status: TODO

- waveform の新パラメータ（unison/sub-osc）を GUI から編集可能にする。
- band-limited は品質優先方針により常時有効とし、GUIトグルは設けない。
- 最小導線で音作りできる表示順・初期値にする。
- GUI導入後も JSON 運用との差異が出ないように統一する。
- 専用テストMIDI（作成対象）:
  - `phaseD_wave_gui_edit_regression.mid`

完了条件:

- GUI だけで waveform 改善パラメータを一通り設定できる
- JSON 直編集と GUI 編集で同じ結果を再現できる
- Phase D専用MIDIの A/B で、GUI編集値とJSON編集値が聴感一致する

## WAVEFORM_PHASE_E_ACCEPTANCE

Status: TODO

- 各Phaseで作成した専用テストMIDI一式で AB 比較を行い、改善可否を判定する。
- 失敗した音色ケース（薄い/痛い/埋もれる）を記録し、次修正へ繋げる。
- 補助確認として `test.mid` / `test2.mid` でも最終ABを行う。

完了条件:

- 耳での AB 比較で「適当なピコピコ音ではない」レベルに到達したと合意できる
- 次の改善点がタスク化されている
