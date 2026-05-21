# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモです。履歴は Git 履歴を参照します。

## 設計方針

- 軸1: `retro_heavy_*`。90年代アーケード/FM寄りの、実曲に馴染む重厚レトロ音色。
- 軸2: `demo_*`。1プリセット1テーマで既存機能を聴いて確認する音色。
- 初期代表: `retro_heavy_fm_brass_ensemble`
- `retro_heavy_*` では、PSG / bell / pluck / SFX を主役にしない。曲全体へ当てる場合は主役、低域、和音、背景、ドラムを中心に使う。
- FMは各opの `levelEnv` / `indexEnv` でアタック、胴、余韻を分ける。ベースのピック感やブラスの立ち上がりはプリセット内のop個別Envで作る。
- `attackLayer` は外部PCMなしで短いピック、吹き始め、金属打撃を足す補助レイヤー。強くしすぎるとピークやチープなクリック感が出るため、主役/低域/ベルへ薄く使う。
- `bassLayer` はベース向けに低域の芯、胴、歪んだ倍音、中低域の焦点を足す補助レイヤー。使いすぎると低域過多やキックとの衝突が出るため、ベースch中心に使う。
- `leadLayer` は主旋律向けに硬い頭、短いしゃくり、薄い二重化、FM金属寄りのクセを足す補助レイヤー。強くしすぎると濁るため、前に出したいリードへ中程度までで使う。
- `expressionMap` はvelocityを音量だけでなく、明るさ、FM index、attack/bass/lead layer量、driveへ薄く反映する。主要リード、ベース、ドラムでは強velocityほど輪郭と押し出しが増えるように使う。
- 命名: `<軸>_<音源方式>_<役割または機能>_<キャラクター>`

## 重厚レトロ

### 主役

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_brass_ensemble` | FM | op個別Env、brass attackLayer、leadLayer v2、expressionMapで立ち上がりとvelocity由来の前面感を押す厚いFMコード/ブラス |
| `retro_heavy_fm_lead_steelblade` | FM | 短いindex Env、metal attackLayer、leadLayer v2、expressionMapで強velocity時にbiteと金属倍音が増えるFMリード |
| `retro_heavy_analog_lead_syncstack` | analog | hard syncの押し出しにpick attackLayer、控えめなleadLayer v2、expressionMapを足した太いアナログ風リード |
| `retro_heavy_analog_lead_pwm_wide` | analog | PWMの揺れにbrass attackLayer、控えめなleadLayer v2、expressionMapを重ねた広めの上物リード |

### 低域

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_bass_pickcore` | FM | 短いindex Env、pick attackLayer、drive系bassLayer v2、expressionMapで強velocity時のピック感と荒い圧を両立するFMベース |
| `retro_heavy_fm_bass_subdrive` | FM | bassLayer v2とexpressionMapで沈むサブ、中低域の胴、強velocity時の押し出しを足すFMサブドライブ |
| `retro_heavy_analog_bass_drive` | analog | 本体drive、grit系bassLayer v2、expressionMapを重ねた、荒い圧と粘りのあるアナログ風ベース |
| `retro_heavy_wave_bass_subsupport` | waveform | FM/analogベースの下に敷くサブ補助。bassLayer v2は薄い胴鳴り用途 |

### 和音と背景

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_chord_stack` | FM | op Envで濁りの立ち上がりを整えた厚いFMコード |
| `retro_heavy_analog_chord_brass` | analog | brass attackLayerを薄く足し、FM群に混ぜるブラスコードの支え役 |
| `retro_heavy_fm_pad_organ_dark` | FM | ゆっくりしたop Envで主旋律を邪魔しない暗めのFMオルガンパッド |
| `retro_heavy_analog_pad_tape` | analog | driftと低いfilterで背景に置きやすいパッド |
| `retro_heavy_wave_pad_sweep_dark` | waveform | 暗めにゆっくりfilterが動く重厚パッド |

### ドラム

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_drumkit_arcade` | drumkit | kick/snare/hatにtom、rim、clap、crash、rideを加え、expressionMapでvelocity差を音量と明るさへ反映する基本キット |
| `retro_heavy_drumkit_impact` | drumkit | 深いkick、太いsnare、派手なtom/crash/rideにexpressionMapを足し、強velocityで押し出す強めのキット |
| `retro_heavy_noise_hat_support` | noise | drumkitのhatへ薄く足す高域補助。単体主役にはしない |

### 補助

| プリセット | 音源方式 | 用途メモ |
|---|---|---|
| `retro_heavy_fm_pluck_support` | FM | level/index Envとpick attackLayerで短く減衰し、フレーズの輪郭を足すFM補助プラック |
| `retro_heavy_fm_bell_support` | FM | 長めのlevel Envとmetal attackLayerで控えめに響くFMベル補助 |
| `retro_heavy_wave_bell_glass_support` | waveform | 透明感を薄く足すガラス系ベル補助 |
| `retro_heavy_psg_pulse_layer` | PSG | FM/analogの輪郭を足す薄いpulseレイヤー |
| `retro_heavy_psg_triangle_layer` | PSG | FM/analogベースを主役化せず下から支える薄いtriangle低域レイヤー |
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
| `demo_attack_layer` | FM/analog | attackLayer | pick、brass、metalの内部生成アタックを比較 |
| `demo_bass_layer_drive` | analog | bassLayer v2 | 低域の芯、フォーカス成分、歪んだ倍音、頭の押し出しを重ねるBassLayer確認 |
| `demo_drumkit_types` | drumkit | drum types | kick、snare、hat、tom、rim、clap、crash、rideの内部生成ドラム確認 |
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
