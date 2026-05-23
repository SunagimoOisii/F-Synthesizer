# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモです。現在のプリセット体系は、互換性よりも「何の楽器として使うか」を優先します。

## 設計方針

- `demo_*` は機能検証用です。1プリセット1テーマで、完成音色ではなく差分を聴くために使います。
- `retro_heavy_*` は実戦用です。名前は `retro_heavy_<instrument>_<role_or_character>` を基本にし、音源方式名は必要な場合だけタグや説明文で補足します。
- 実戦プリセットは「バンド + ゲーム」編成を想定し、Bass / Lead / Guitar / Strings / Keys / Brass / Pad / Drums / SFX / Support に分けます。
- PSG、bell、noise、SFX は主役にしすぎず、補助用途として音量と説明を抑えています。
- DrumKit は `drumBus` 前提です。音量を下げるだけでなく、attack、glue、presence、room で太さを残しながら前に出すぎない配置を作ります。
- `layers.pluck`、`layers.string`、`layers.body` は、アコギ代役、弦風レイヤー、箱鳴り/胴鳴りを作る共通レイヤーです。
- `source.filter.mode = ladderLowpass` は、clean biquad より太く癖のあるローパスとして使います。

## 機能デモ

| プリセット | 確認する機能 | 聴くポイント |
|---|---|---|
| `demo_fm_algorithm_stack` | FM stack algorithm | 縦積み変調の濃さ |
| `demo_fm_algorithm_parallel` | FM parallel algorithm | 並列carrierの分離感 |
| `demo_fm_feedback_edge` | FM feedback | feedbackで増える金属的エッジ |
| `demo_fm_env_index` | FM index envelope | アタックから減衰する倍音 |
| `demo_filter_ladder_drive` | Ladder lowpass + drive | clean filterでは出ない太さと歪み |
| `demo_filter_resonance_sweep` | Filter resonance | cutoff/resonanceの癖 |
| `demo_pluck_body_layer` | Pluck + Body | ピック頭と胴鳴りの足され方 |
| `demo_string_layer_motion` | String layer | detune、bow/noise、揺れ、spread |
| `demo_drum_bus_placement` | Drum Bus | ドラムのattack抑制、glue、room配置 |
| `demo_drumkit_models` | DrumKit models | kick/snare/hat/tom/rim/clap/cymbalの基本音 |
| `demo_modulation_lfo_motion` | LFO modulation | 周期的な明るさや揺れ |
| `demo_expression_velocity` | Expression map | velocityで音量以外の表情が変わる挙動 |
| `demo_master_fx_chain` | Master effects | reducer、crusher、chorus、flanger、delay、reverb |

## 実戦プリセット

### Bass

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_bass_synth_drive` | 歪んだ芯のあるシンセベース。主低域向け。 |
| `retro_heavy_bass_pick_fm` | ピック感のあるFMベース。刻みや速いリフ向け。 |
| `retro_heavy_bass_sub_anchor` | 曲の底を支えるサブ。目立たせすぎない低域用。 |
| `retro_heavy_bass_sub_support` | 他のベースを補強する薄いサブ補助。単体主役には不向き。 |
| `retro_heavy_bass_chip_triangle` | PSG/三角波系の低域補助。チップ感を足す用途。 |

### Lead

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_lead_hardsync_main` | hard sync系の主旋律。太いが和音には向かない。 |
| `retro_heavy_lead_metal_fm` | 金属的なFMリード。強いメロディやソロ向け。 |
| `retro_heavy_lead_wide_pwm` | 広がりのあるpulse lead。中央を空けたい上物向け。 |
| `retro_heavy_lead_motion_pulse` | 動きのあるpulse lead。長く伸ばす旋律に向く。 |
| `retro_heavy_lead_brass_fm` | ブラス寄りの押し出しを持つリード。短いフレーズ向け。 |

### Guitar

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_guitar_acoustic_pluck` | レトロなアコギ代役。アルペジオや刻み向け。 |
| `retro_heavy_guitar_muted_chop` | ミュートギター風の短い刻み。持続音には不向き。 |
| `retro_heavy_guitar_power_chord` | パワーコード代役。低中域を埋めるリフ向け。 |
| `retro_heavy_guitar_glass_harmonics` | ハーモニクス/ベル寄りの補助ギター。主役にしすぎない。 |

### Strings

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_strings_synth_section` | シンセ弦セクション。コードや持続音で厚みを作る。 |
| `retro_heavy_strings_dark_pad` | 暗いストリングスパッド。背景に沈める用途。 |
| `retro_heavy_strings_bowed_lead` | 弓弾き風のリード。対旋律や長い旋律向け。 |
| `retro_heavy_strings_ensemble_wide` | 広い弦アンサンブル。左右の背景を作る用途。 |

### Keys

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_keys_electric_piano` | FM電気ピアノ代役。コード、リフ、アルペジオ向け。 |
| `retro_heavy_keys_dark_organ` | 暗いオルガンパッド。中域の支えに使う。 |
| `retro_heavy_keys_glass_bell` | ガラス/ベル系キー。アクセントや高域補助向け。 |
| `retro_heavy_keys_chip_pulse` | チップ系キー。細い輪郭を足す補助パート向け。 |

### Brass

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_brass_fm_stab` | FMブラススタブ。短いアクセントやヒット向け。 |
| `retro_heavy_brass_analog_chord` | アナログ風ブラスコード。和音の押し出し用。 |
| `retro_heavy_brass_dark_ensemble` | 暗いブラス集合音。派手すぎない中域補強向け。 |

### Pad

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_pad_tape_warm` | テープ風の温かい背景パッド。 |
| `retro_heavy_pad_dark_sweep` | 暗いfilter sweepパッド。場面転換や長いコード向け。 |
| `retro_heavy_pad_wide_background` | 左右に広い背景パッド。主旋律の後ろに置く。 |
| `retro_heavy_pad_motion_pulse` | pulseが揺れる動的パッド。単純な白玉を避けたい時に使う。 |

### Drums

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_drums_arcade_kit` | 軽めの標準アーケードキット。MIDIドラムの基本置換向け。 |
| `retro_heavy_drums_impact_kit` | 強いが前に出すぎないインパクトキット。主ドラム向け。 |
| `retro_heavy_drums_glued_backbeat` | Drum Busを強めたミックス向けキット。太く奥へ置く用途。 |

### SFX / Support

| プリセット | 用途メモ |
|---|---|
| `retro_heavy_sfx_laser_fm` | FMレーザー効果音。曲全体へ割り当てない。 |
| `retro_heavy_sfx_noise_riser` | ノイズライザー。場面転換やビルドアップ向け。 |
| `retro_heavy_support_noise_hat` | ハイハット補助。DrumKitの高域へ薄く混ぜる。 |
| `retro_heavy_support_psg_pulse` | PSG pulse補助。FM/analogの輪郭を薄く足す。 |
| `retro_heavy_support_psg_triangle` | PSG triangle補助。低域の芯を薄く支える。 |

## 使い方

CLI:

```powershell
F-Synthesizer.exe --cli --preset retro_heavy_bass_synth_drive
F-Synthesizer.exe --cli --preset demo_filter_ladder_drive
```

GUI:

- メイン画面の `Preset` コンボから選択して適用します。
- `tags` と `description` はプリセット検索・説明表示に使います。
- 実曲へ割り当てる場合は、Bass / Lead / Guitar / Strings / Keys / Brass / Pad / Drums から役割で選び、SFX / Support は補助として薄く使います。

## 補足

- プリセットは `base.json` に差分適用されます。
- ファイル名は CLI 指定しやすいように英数字と underscore で統一します。
- 説明文とドキュメントは日本語で記述します。
- PSG の `maxVoices` はチップ制約を表す設定値ですが、現行レンダーでは未適用です。
- 音源方式とレンダリング契約は `docs/Architecture.md` を正本とします。
