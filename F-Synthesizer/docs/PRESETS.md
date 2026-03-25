# PRESETS

最終更新: 2026-03-25

`config/presets/` 配下のプリセット一覧と用途メモ。

## 使い方

CLI:

```powershell
# 例: FMベル
F-Synthesizer.exe --cli --preset fm_bell_glass

# 例: Wave pad
F-Synthesizer.exe --cli --preset wave_pad_air
```

GUI:

- Main 画面の `Preset` コンボから選択して適用する。

## プリセット一覧

| Preset | 主な source.type | 対象ch | 用途メモ |
|---|---|---|---|
| `drumkit_basic` | `drumkit` | `9` | 基本ドラムキット（kick/snare/hat） |
| `fm_bass_pluck` | `fm` | `0` | FM系の短いベースプラック |
| `fm_bass_deep` | `fm` | `0` | チェーンFMベース（4オペ algo3） |
| `fm_bell_glass` | `fm` | `0` | FMベル系（ガラス質） |
| `fm_brass_mega` | `fm` | `0` | Mega Drive系ブラス（4オペ algo1） |
| `fm_organ_retro` | `fm` | `0` | 1変調3キャリアオルガン（algo2） |
| `fm_string_retro` | `fm` | `0` | 80年代FM弦（4オペ algo1） |
| `noise_hat_air` | `noise` | `0` | ノイズ主体のハット/空気感レイヤー |
| `psg_bass_triangle` | `psg` | `0` | PSG三角波ベース |
| `psg_lead_pulse` | `psg` | `0` | PSGパルス系リード |
| `psg_lead_square` | `psg` | `0` | PSG矩形波リード |
| `wave_bass_solid` | `waveform` | `0` | 低域重心のWaveベース |
| `wave_bass_wide` | `waveform` + `drumkit` | `1,2,3,9,10` | ワイド系Waveセット |
| `wave_lead_bright` | `waveform` + `drumkit` | `1,2,3,9,10` | 明るいWaveリードセット |
| `wave_lead_soft` | `drumkit` | `9` | 既存プリセット（実データ準拠） |
| `wave_pad_air` | `waveform` | `0` | ゆっくり立ち上がるPad |
| `wave_sub_bass_warm` | `waveform` | `0` | サブ寄りウォームベース |
| `wave_sub_lead_resonant` | `waveform` | `2,3` | レゾナント寄りのサブリード |

## 補足

- プリセットは `base.json` に差分適用される。
- `source.smoothing` は `waveform` のみ有効。
- 各方式の仕様は `docs/synth-methods/` 配下を正本とする。
