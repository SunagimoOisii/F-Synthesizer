# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモです。履歴は Git 履歴を参照します。

## 設計方針

- 軸1: `retro_heavy_*`。90年代ゲーム音源風の重厚な実用音色。
- 軸2: `demo_*`。1プリセット1テーマで既存機能を聴いて確認する音色。
- 初期代表: `retro_heavy_fm_lead_brasswall`
- 命名: `<軸>_<音源方式>_<役割または機能>_<キャラクター>`

## 重厚レトロ

| プリセット | 音源方式 | 役割 | 機能テーマ | 用途メモ |
|---|---|---|---|---|
| `retro_heavy_fm_lead_brasswall` | FM | リード | FM algorithm 6 / Env2 index | 厚いFMブラスリード。初回体験の代表音色 |
| `retro_heavy_fm_lead_blade` | FM | リード | FM algorithm 7 / feedback | 硬く切り込む金属的リード |
| `retro_heavy_fm_bass_pickgrit` | FM | ベース | FM algorithm 3 / Env2 index | ピック感とザラつきのあるFMベース |
| `retro_heavy_fm_bass_depth` | FM | ベース | 低域FM / 暗めフィルター | 深く沈む低域用FMベース |
| `retro_heavy_fm_chord_stack` | FM | コード | FM積み重ね / ゆっくりしたフィルター | 厚い和音と少し濁った倍音 |
| `retro_heavy_fm_pad_organ` | FM | パッド | LFO delay/fade | 太いFMオルガンパッド |
| `retro_heavy_fm_bell_steel` | FM | ベル | 非整数ratio | 抜けるスチールベル |
| `retro_heavy_fm_pluck_arcade` | FM | プラック | 短いEnv2 index | アーケード風FMプラック |
| `retro_heavy_fm_sfx_laser` | FM | 効果音 | pitch / Env2 sweep | レーザー系効果音 |
| `retro_heavy_analog_lead_sync` | analog | リード | hard sync | sync倍音の太いリード |
| `retro_heavy_analog_lead_pwm` | analog | リード | PWM / LFO | 揺れる矩形波リード |
| `retro_heavy_analog_bass_drive` | analog | ベース | drive / sub oscillator | 荒い倍音の重いベース |
| `retro_heavy_analog_chord_brass` | analog | コード | unison / filter LFO | FM群を支えるブラスコード |
| `retro_heavy_analog_pad_tape` | analog | パッド | drift / slow LFO | テープ風に揺れる厚いパッド |
| `retro_heavy_analog_arp_motor` | analog | アルペジオ | arpeggio | 推進力のある短いアルペジオ |
| `retro_heavy_wave_bass_sub` | waveform | ベース | sub oscillator | FMベース下に敷く低域 |
| `retro_heavy_wave_pad_sweep` | waveform | パッド | filter sweep | 滑らかな重厚パッド |
| `retro_heavy_wave_bell_glass` | waveform | ベル | ring modulation | 柔らかいガラス系ベル |
| `retro_heavy_psg_chord_pulse` | PSG | コード | pulse / 最大ボイス数 | PSGを厚く重ねたコード |
| `retro_heavy_psg_bass_triangle` | PSG | ベース | triangle | 硬い低域補強 |
| `retro_heavy_noise_hat_metal` | noise | ドラム | highpass noise | 金属的なハット層 |
| `retro_heavy_noise_sweep_riser` | noise | 効果音 | bandpass noise | 場面転換向けノイズスイープ |
| `retro_heavy_drumkit_arcade` | drumkit | ドラムキット | kick / snare / hat map | 90年代アーケード寄り基本キット |
| `retro_heavy_drumkit_impact` | drumkit | ドラムキット | 重めのdrum map | 強いkickと金属hatのimpactキット |

## 機能デモ

| プリセット | 音源方式 | 役割 | 機能テーマ | 用途メモ |
|---|---|---|---|---|
| `demo_fm_algo_stack` | FM | デモ | FM algorithm stack | 縦積み変調の濃い倍音 |
| `demo_fm_algo_parallel` | FM | デモ | FM algorithm parallel | 複数carrierの並列感 |
| `demo_fm_feedback_edge` | FM | デモ | FM feedback | feedbackで増えるエッジ |
| `demo_fm_env2_index` | FM | デモ | Env2 -> fm.index | アタック時のFM index変化 |
| `demo_pwm_lfo` | waveform | デモ | LFO -> pulseWidth | PWMによる矩形波の揺れ |
| `demo_hard_sync` | analog | デモ | hard sync | sync ratioによる鋭い倍音 |
| `demo_ring_mod` | waveform | デモ | ring modulation | 金属的な非整数倍音 |
| `demo_arpeggio_steps` | analog | デモ | arpeggio | 8step半音列の音程変化 |
| `demo_lfo_sample_hold` | analog | デモ | Sample&Hold LFO | ランダム段階filter変調 |
| `demo_lfo_delay_fade` | analog | デモ | LFO delay/fade | 後から立ち上がるビブラート |
| `demo_env2_curve` | FM | デモ | Env2 curve | 指数的なindex減衰 |
| `demo_filter_resonance` | analog | デモ | filter resonance | 共振スイープの癖 |
| `demo_modwheel_filter` | analog | デモ | modWheel -> filterCutoffHz | MIDI CCで明るさを操作 |
| `demo_pressure_amp` | FM | デモ | pressure -> amp/filter | Aftertouch入力の確認 |
| `demo_noise_filter` | noise | デモ | noise bandpass | ノイズに音程感を付ける |
| `demo_master_fx_chain` | FM | デモ | master effects | bit crushからreverbまでのチェーン確認 |

## 使い方

CLI:

```powershell
F-Synthesizer.exe --cli --preset retro_heavy_fm_lead_brasswall
F-Synthesizer.exe --cli --preset demo_hard_sync
```

GUI:

- メイン画面の `Preset` コンボから選択して適用する。
- `tags` と説明文は Layer1 Discovery の絞り込み・説明表示に使う。

## 補足

- プリセットは `base.json` に差分適用される。
- ファイル名は CLI 指定しやすいように英数字と underscore で統一する。
- 説明文とドキュメントは日本語で記述する。
- 音源方式とレンダリング契約は `docs/Architecture.md` を正本とする。
