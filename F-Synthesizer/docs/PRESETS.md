# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモです。履歴は Git 履歴を参照します。

## 設計方針

- 軸1: `retro_heavy_*`。90年代アーケード/FM寄りの、実曲に馴染む重厚レトロ音色。
- 軸2: `demo_*`。1プリセット1テーマで既存機能を聴いて確認する音色。
- 初期代表: `retro_heavy_fm_brass_ensemble`
- `retro_heavy_*` では、PSG / bell / pluck / SFX を主役にしない。曲全体へ当てる場合は主役、低域、和音、背景、ドラムを中心に使う。
- 命名: `<軸>_<音源方式>_<役割または機能>_<キャラクター>`

## 重厚レトロ

### 主役

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_brass_ensemble` | FM | 厚いFMコード/ブラスの代表音色。単音でも和音でも曲に馴染む主役向け |
| `retro_heavy_fm_lead_steelblade` | FM | 硬さを残しつつ明るすぎないFMリード |
| `retro_heavy_analog_lead_syncstack` | analog | hard syncの押し出しを持つ太いアナログ風リード |
| `retro_heavy_analog_lead_pwm_wide` | analog | PWMの揺れを控えめにした広めの上物リード |

### 低域

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_bass_pickcore` | FM | ピック感を中低域に寄せたFMベース |
| `retro_heavy_fm_bass_subdrive` | FM | 深く沈むFMサブドライブ |
| `retro_heavy_analog_bass_drive` | analog | driveを効かせた重いアナログ風ベース |
| `retro_heavy_wave_bass_subsupport` | waveform | FM/analogベースの下に薄く敷くサブ補助 |

### 和音と背景

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_chord_stack` | FM | 高域を抑えた濁りで厚みを作るFMコード |
| `retro_heavy_analog_chord_brass` | analog | FM群に混ぜるブラスコードの支え役 |
| `retro_heavy_fm_pad_organ_dark` | FM | 主旋律を邪魔しない暗めのFMオルガンパッド |
| `retro_heavy_analog_pad_tape` | analog | driftと低いfilterで背景に置きやすいパッド |
| `retro_heavy_wave_pad_sweep_dark` | waveform | 暗めにゆっくりfilterが動く重厚パッド |

### ドラム

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_drumkit_arcade` | drumkit | 芯のある90年代アーケード寄り基本キット |
| `retro_heavy_drumkit_impact` | drumkit | 強いpitch kickと短いトーンsnare/hat |
| `retro_heavy_noise_hat_support` | noise | ドラム本体を邪魔しない薄いチップ風ハット補助 |

### 補助

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_pluck_support` | FM | フレーズの輪郭を少し足す短いFM補助プラック |
| `retro_heavy_fm_bell_support` | FM | 常時主役にしない控えめなFMベル補助 |
| `retro_heavy_wave_bell_glass_support` | waveform | 透明感を薄く足すガラス系ベル補助 |
| `retro_heavy_psg_pulse_layer` | PSG | FM/analogの輪郭を足す薄いpulseレイヤー |
| `retro_heavy_psg_triangle_layer` | PSG | FM/analogベースの下支え用triangleレイヤー |
| `retro_heavy_analog_motion_pulse` | analog | アルペジオではなく緩い動きで支えるpulse補助 |

### 効果音

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_sfx_laser` | FM | 通常曲の主役ではなく、短い効果音用途 |
| `retro_heavy_noise_sweep_riser` | noise | 場面転換や効果音向けのノイズスイープ |

## 機能デモ

| プリセット | 音源方式 | 機能テーマ | 用途メモ |
|---|---|---|---|
| `demo_fm_algo_stack` | FM | FM algorithm stack | 縦積み変調の濃い倍音 |
| `demo_fm_algo_parallel` | FM | FM algorithm parallel | 複数carrierの並列感 |
| `demo_fm_feedback_edge` | FM | FM feedback | feedbackで増えるエッジ |
| `demo_fm_env2_index` | FM | Env2 -> fm.index | アタック時のFM index変化 |
| `demo_pwm_lfo` | waveform | LFO -> pulseWidth | PWMによる矩形波の揺れ |
| `demo_hard_sync` | analog | hard sync | sync ratioによる鋭い倍音 |
| `demo_ring_mod` | waveform | ring modulation | 金属的な非整数倍音 |
| `demo_arpeggio_steps` | analog | arpeggio | 8step半音列の音程変化 |
| `demo_lfo_sample_hold` | analog | Sample&Hold LFO | ランダム段階filter変調 |
| `demo_lfo_delay_fade` | analog | LFO delay/fade | 後から立ち上がるビブラート |
| `demo_env2_curve` | FM | Env2 curve | 指数的なindex減衰 |
| `demo_filter_resonance` | analog | filter resonance | 共振スイープの癖 |
| `demo_modwheel_filter` | analog | modWheel -> filterCutoffHz | MIDI CCで明るさを操作 |
| `demo_pressure_amp` | FM | pressure -> amp/filter | Aftertouch入力の確認 |
| `demo_noise_filter` | noise | noise bandpass | ノイズに音程感を付ける |
| `demo_master_fx_chain` | FM | master effects | bit crushからreverbまでのチェーン確認 |

## 使い方

CLI:

```powershell
F-Synthesizer.exe --cli --preset retro_heavy_fm_brass_ensemble
F-Synthesizer.exe --cli --preset demo_hard_sync
```

GUI:

- メイン画面の `Preset` コンボから選択して適用する。
- `tags` と説明文は Layer1 Discovery の絞り込み・説明表示に使う。
- 実曲へ割り当てる場合は、主役、低域、和音、背景、ドラムを中心に選ぶ。補助と効果音は全体へ広く当てない。

## 補足

- プリセットは `base.json` に差分適用される。
- ファイル名は CLI 指定しやすいように英数字と underscore で統一する。
- 説明文とドキュメントは日本語で記述する。
- 音源方式とレンダリング契約は `docs/Architecture.md` を正本とする。
