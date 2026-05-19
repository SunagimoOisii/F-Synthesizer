# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモ。
詳細な履歴は Git history を参照する。

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
| `psg_nesgb_chord_pulse` | `psg` | Chord | `0` | PSG 3-voice 和音レイヤー |
| `drumkit_nesgb_core` | `drumkit` | Drumkit | `9` | NES系基本3点（kick/snare/hat） |
| `wave_nesgb_pad_hollow` | `waveform` | Pad | `0` | NES三角波風ホローパッド |
| `analog_nesgb_arp_heroic` | `analog` | Lead | `0` | root-fifth-octave アルペジオ 10Hz + keySync LFO |
| `analog_nesgb_arp_fanfare` | `analog` | Lead | `0` | メジャートライアド アルペジオ 12Hz |
| `analog_nesgb_lead_sync16` | `analog` | Lead | `0` | hardSync ratio2 + keySync saw LFO |
| `analog_nesgb_chord_ring` | `analog` | Chord | `0` | ring mod 1.5x でエイリアンベル倍音 |

### retro_mdpc88

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `fm_mdpc88_lead_pierce` | `fm` | Lead | `0` | 抜けるFMリード |
| `fm_mdpc88_bass_pick` | `fm` | Bass | `0` | ピック感のあるFMベース |
| `fm_mdpc88_bass_algo3` | `fm` | Bass | `0` | algo 3 深いFMベース + exponential env2 |
| `fm_mdpc88_chord_stack` | `fm` | Chord | `0` | 積層FMコード |
| `fm_mdpc88_pluck_arcade` | `fm` | Pluck | `0` | アーケード寄り短音 |
| `fm_mdpc88_bell_tine` | `fm` | Bell | `0` | FMベル/ティン系 |
| `fm_mdpc88_pad_organ` | `fm` | Pad | `0` | algo 4 オルガンパッド + LFO delay/fade |
| `fm_mdpc88_lead_algo6` | `fm` | Lead | `0` | algo 6 ブラスリード（Mega Drive風） |
| `fm_mdpc88_lead_algo7` | `fm` | Lead | `0` | algo 7 四並列シマーリード |
| `fm_mdpc88_sfx_laser` | `fm` | SFX | `0` | FM ピッチドロップ レーザーSFX |
| `analog_mdpc88_chord_brass` | `analog` | Chord | `0` | MD/PC-88寄りのブラスコード |
| `analog_mdpc88_lead_stabwarm` | `analog` | Lead | `0` | driven saw + exponential env2 フィルタースイープ |
| `drumkit_mdpc88_arcade` | `drumkit` | Drumkit | `9` | MD系エレクトロ3点 |

### hybrid_snes

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `wave_snes_lead_vibrato` | `waveform` | Lead | `0` | ビブラート付き主旋律 |
| `wave_snes_bass_sub` | `waveform` | Bass | `0` | サブ強めの低域 |
| `wave_snes_chord_glass` | `waveform` | Chord | `0` | 透明感のある和音 |
| `wave_snes_pad_sweep` | `waveform` | Pad | `0` | 緩やかなフィルタスイープPad |
| `wave_snes_pluck_soft` | `waveform` | Pluck | `0` | 柔らかい短音 |
| `wave_snes_bell_water` | `waveform` | Bell | `0` | 三角波ベル + keySync LFO delay/fade |
| `analog_snes_pluck_reso` | `analog` | Pluck | `0` | SNESハイブリッド寄りのレゾプラック |
| `analog_snes_lead_pwm` | `analog` | Lead | `0` | PWM lead keySync unison （pulse width呼吸） |
| `drumkit_snes_core` | `drumkit` | Drumkit | `9` | SNESソフトドラム（sine kick, pink hat） |
| `noise_snes_pad_airwash` | `noise` | Pad | `0` | SNES風の空気感ノイズパッド |
| `noise_snes_drum_rim` | `noise` | SFX | `0` | white noise highpass リムショット |

### modern

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `wave_modern_lead_wide` | `waveform` | Lead | `0` | ユニゾン広がりリード |
| `wave_modern_lead_pwm` | `waveform` | Lead | `0` | PWM + keySync LFO delay/fade（pulse width呼吸） |
| `wave_modern_bass_subdrive` | `waveform` | Bass | `0` | ドライブ付きサブベース |
| `wave_modern_chord_bright` | `waveform` | Chord | `0` | 明るい広がりコード |
| `wave_modern_pad_air` | `waveform` | Pad | `0` | 空気感のある長音Pad |
| `wave_modern_pluck_click` | `waveform` | Pluck | `0` | クリック感のある短音 |
| `fm_modern_bell_digital` | `fm` | Bell | `0` | デジタル寄りベル |
| `fm_modern_pad_vox` | `fm` | Pad | `0` | algo 5 vocal pad + modwheel→index |
| `wave_modern_sfx_riser` | `waveform` | SFX | `0` | 上昇系ライザー |
| `analog_modern_lead_drift` | `analog` | Lead | `0` | ドリフトと同期感のあるモダンリード |
| `analog_modern_lead_resowobble` | `analog` | Lead | `0` | LFO→filterResonance wobble（wah的キャラクター） |
| `analog_modern_bass_mono` | `analog` | Bass | `0` | 低域重視のモノベース |
| `analog_modern_pad_tape` | `analog` | Pad | `0` | ゆらぎを活かしたテープ風パッド |
| `analog_modern_sfx_glitch` | `analog` | SFX | `0` | 8step random arp + S&H LFO グリッチ |
| `drumkit_modern_punchy` | `drumkit` | Drumkit | `9` | モダン電子ドラム（deep kick, tight hat） |
| `noise_modern_sfx_risergrain` | `noise` | SFX | `0` | モダンな粒状ライザーノイズ |

### analog（world coverage）

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `analog_nesgb_lead_chipfat` | `analog` | Lead | `0` | NES/GB感を残した太いパルス系リード |
| `analog_mdpc88_chord_brass` | `analog` | Chord | `0` | MD/PC-88寄りのブラスコード |
| `analog_snes_pluck_reso` | `analog` | Pluck | `0` | SNESハイブリッド寄りのレゾプラック |
| `analog_modern_lead_drift` | `analog` | Lead | `0` | ドリフトと同期感のあるモダンリード |
| `analog_modern_bass_mono` | `analog` | Bass | `0` | 低域重視のモノベース |
| `analog_modern_pad_tape` | `analog` | Pad | `0` | ゆらぎを活かしたテープ風パッド |

### noise（world coverage）

| Preset | source | role | 対象ch | 用途メモ |
|---|---|---|---|---|
| `noise_nesgb_sfx_burst` | `noise` | SFX | `0` | NES/GB向け短いノイズバースト |
| `noise_mdpc88_drum_hatdust` | `noise` | Drum | `9` | MD/PC-88向けハット層ノイズ |
| `noise_mdpc88_sfx_sweep` | `noise` | SFX | `0` | pink noise bandpass 共鳴スイープ |
| `noise_snes_pad_airwash` | `noise` | Pad | `0` | SNES風の空気感ノイズパッド |
| `noise_snes_drum_rim` | `noise` | SFX | `0` | white noise highpass リムショット |
| `noise_modern_sfx_risergrain` | `noise` | SFX | `0` | モダンな粒状ライザーノイズ |

## 使い方

CLI:

```powershell
# 例: SNES系リード
F-Synthesizer.exe --cli --preset wave_snes_lead_vibrato

# 例: MD系FMプラック
F-Synthesizer.exe --cli --preset fm_mdpc88_pluck_arcade
```

GUI:

- メイン画面の `Preset` コンボから選択して適用する。

## 補足

- プリセットは `base.json` に差分適用される。
- `source.smoothing` は `waveform` のみ有効。
- FM の `modulation` ブロックは `source` 内に記述する。
- 音源方式とレンダリング契約は `docs/Architecture.md` を正本とする。
- 旧プリセットは削除済み（`config/presets/` には新セットのみ配置）。
