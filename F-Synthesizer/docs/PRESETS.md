# PRESETS

最終更新: 2026-03-26

`config/presets/` 配下のプリセット一覧と用途メモ。
既存プリセットを破棄し、2026-03-26 に新セットへ全面刷新。

## 設計方針（D-1）

- 軸1: 役割網羅（`Lead / Bass / Chord / Pad / Pluck / Bell / Drumkit / SFX`）
- 軸2: 世界観分離（`NES/GB`, `MD/PC-88`, `SNES-hybrid`, `Modern`）
- 命名: `<source>_<world>_<role>_<character>`

## プリセット一覧（新セット）

### retro_nesgb

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `psg_nesgb_lead_square` | `psg` | Lead | `0` | 8bitメロディ向けスクエア |
| `psg_nesgb_bass_triangle` | `psg` | Bass | `0` | 低域土台の三角ベース |
| `psg_noise_nesgb_sfx` | `psg` | SFX | `0` | ノイズ短音・効果音向け |
| `drumkit_nesgb_core` | `drumkit` | Drumkit | `9` | NES系基本3点（kick/snare/hat） |

### retro_mdpc88

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `fm_mdpc88_lead_pierce` | `fm` | Lead | `0` | 抜けるFMリード |
| `fm_mdpc88_bass_pick` | `fm` | Bass | `0` | ピック感のあるFMベース |
| `fm_mdpc88_chord_stack` | `fm` | Chord | `0` | 積層FMコード |
| `fm_mdpc88_pluck_arcade` | `fm` | Pluck | `0` | アーケード寄り短音 |
| `fm_mdpc88_bell_tine` | `fm` | Bell | `0` | FMベル/ティン系 |
| `drumkit_mdpc88_arcade` | `drumkit` | Drumkit | `9` | MD系エレクトロ3点 |

### hybrid_snes

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `wave_snes_lead_vibrato` | `waveform` | Lead | `0` | ビブラート付き主旋律 |
| `wave_snes_bass_sub` | `waveform` | Bass | `0` | サブ強めの低域 |
| `wave_snes_chord_glass` | `waveform` | Chord | `0` | 透明感のある和音 |
| `wave_snes_pad_sweep` | `waveform` | Pad | `0` | 緩やかなフィルタスイープPad |
| `wave_snes_pluck_soft` | `waveform` | Pluck | `0` | 柔らかい短音 |

### modern

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `wave_modern_lead_wide` | `waveform` | Lead | `0` | ユニゾン広がりリード |
| `wave_modern_bass_subdrive` | `waveform` | Bass | `0` | ドライブ付きサブベース |
| `wave_modern_chord_bright` | `waveform` | Chord | `0` | 明るい広がりコード |
| `wave_modern_pad_air` | `waveform` | Pad | `0` | 空気感のある長音Pad |
| `wave_modern_pluck_click` | `waveform` | Pluck | `0` | クリック感のある短音 |
| `fm_modern_bell_digital` | `fm` | Bell | `0` | デジタル寄りベル |
| `wave_modern_sfx_riser` | `waveform` | SFX | `0` | 上昇系ライザー |

## 使い方

CLI:

```powershell
# 例: SNES系リード
F-Synthesizer.exe --cli --preset wave_snes_lead_vibrato

# 例: MD系FMプラック
F-Synthesizer.exe --cli --preset fm_mdpc88_pluck_arcade
```

GUI:

- Main 画面の `Preset` コンボから選択して適用する。

## 補足

- プリセットは `base.json` に差分適用される。
- `source.smoothing` は `waveform` のみ有効。
- FM の `modulation` ブロックは `source` 内に記述する。
- 各方式の仕様は `docs/synth-methods/` 配下を正本とする。
- 旧プリセットは `config/presets/_archive/` に退避済み。
