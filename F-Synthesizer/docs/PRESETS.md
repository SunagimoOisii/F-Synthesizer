# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモです。現在の実戦 Sound Card は、Play で選んで短く試聴した時の気持ちよさを最優先にします。

## 設計方針

- 実戦用 Sound Card は `sound_<category>_<name>` の形式で管理します。
- Play では `internal: false` の実戦 preset だけを表示します。
- Advanced では `internal: true` の `demo_*` 検証 preset に到達できます。
- 実戦カテゴリは `Lead / Bass / Pad / Keys / Drums / SFX / Support` に固定します。
- 表示名は短い英語名、説明文は初心者が用途を判断できる日本語短文にします。
- tags は用途と質感を表す英語 lowercase を基本にします。
- 旧実戦 preset は廃止し、CLI でも新しい `sound_*` 名を使います。
- 実戦 Sound Card は重厚なレトロ感を残しつつ、不要な hiss、耳につく aliasing、過剰な drive、用途不明な noise を避けます。
- drive / FM feedback / hard sync / ring mod は主役にせず、質感付けに留めます。
- noise は Drums / SFX / Support など用途が明確な preset で使い、Lead / Bass / Pad / Keys の主成分にはしません。

## 実戦 Sound Card

### Lead

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_lead_blade` | Lead Blade | 鋭い芯を保ちつつ、耳に刺さりにくく整えた主旋律リード。 |
| `sound_lead_metal` | Lead Metal | 金属的な倍音を控えめに整えた強いリード。 |
| `sound_lead_sync` | Lead Sync | 同期感のあるレトロリード。反復フレーズやオブリ向け。 |
| `sound_lead_wide` | Lead Wide | ノイズ感を抑えた、広がりのある太いリード。 |

### Bass

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_bass_drive` | Bass Drive | 歪みを整理した主ベース。 |
| `sound_bass_pick` | Bass Pick | ざらつきを抑えた、ピック感のあるFMベース。 |
| `sound_bass_sub` | Bass Sub | 低域の芯を優先したサブベース。 |
| `sound_bass_chip` | Bass Chip | 階段感を抑えたチップ系低域補助。 |

### Pad

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_pad_warm` | Pad Warm | 温かく、濁りにくい背景パッド。 |
| `sound_pad_dark` | Pad Dark | 高域のざらつきを避けた暗めの背景パッド。 |
| `sound_pad_wide` | Pad Wide | 明るさを足しすぎない広がりのあるパッド。 |
| `sound_pad_motion` | Pad Motion | ノイズ感を抑えた、ゆっくり動くパッド。 |

### Keys

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_keys_electric` | Keys Electric | 丸いエレクトリックキー。コードや短いリフ向け。 |
| `sound_keys_organ` | Keys Organ | 常時ざらつかない、整えたオルガン系キー。 |
| `sound_keys_bell` | Keys Bell | 金属感を控えめにしたベルキー。高域アクセント向け。 |
| `sound_keys_pluck` | Keys Pluck | 耳に残るざらつきを抑えた短いプラック音。 |

### Drums

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_drums_arcade` | Drums Arcade | ノイズ成分を整理した標準的なアーケードドラム。 |
| `sound_drums_impact` | Drums Impact | 押し出しを残しつつ、過剰な歪みを抑えたドラム。 |
| `sound_drums_glue` | Drums Glue | 曲中でまとまりやすい、派手すぎないドラム。 |

### SFX

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_sfx_laser` | SFX Laser | 高域の刺さりを抑えた短いレーザー効果音。 |
| `sound_sfx_riser` | SFX Riser | 目立ちすぎない場面転換用ノイズライザー。 |

### Support

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_support_hat` | Support Hat | 耳につきにくい薄いハット補助。 |
| `sound_support_pulse` | Support Pulse | 控えめな PSG pulse 補助。 |
| `sound_support_triangle` | Support Triangle | 目立ちすぎない PSG triangle 補助。 |

## 機能デモ

`demo_*` は完成音色ではなく、機能差分を確認するための検証 preset です。Play には表示せず、Advanced から到達します。

| Preset | 確認する機能 |
|---|---|
| `demo_fm_algorithm_stack` | FM stack algorithm |
| `demo_fm_algorithm_parallel` | FM parallel algorithm |
| `demo_fm_feedback_edge` | FM feedback |
| `demo_fm_env_index` | FM index envelope |
| `demo_filter_ladder_drive` | Ladder lowpass + drive |
| `demo_filter_resonance_sweep` | Filter resonance |
| `demo_pluck_body_layer` | Pluck + Body |
| `demo_string_layer_motion` | String layer |
| `demo_drum_bus_placement` | Drum Bus |
| `demo_drumkit_models` | DrumKit models |
| `demo_modulation_lfo_motion` | LFO modulation |
| `demo_expression_velocity` | Expression map |
| `demo_master_fx_chain` | Master effects |

## 使い方

CLI:

```powershell
F-Synthesizer.exe --cli --preset sound_lead_blade
F-Synthesizer.exe --cli --preset sound_drums_arcade
F-Synthesizer.exe --cli --preset demo_filter_ladder_drive
```

GUI:

- Play で Sound Card を選び、Tone Preview や Drum Pad で短く試聴します。
- 4 マクロで感覚的に調整し、`曲で使う` から Compose の MIDI チャンネルへ割り当てます。
- Compose では曲中の音量と左右を調整します。
- 詳細音色、Master FX、Mixer/割当、検証 preset は Advanced で扱います。

## 補足

- Preset は `config/base.json` の上に差分適用されます。
- `config/default.json` は `sound_lead_blade` 相当を代表音として含みます。
- ファイル名は CLI 指定しやすいように英数字と underscore で統一します。
- 音源方式とレンダリング契約は `docs/Architecture.md` を正本とします。
