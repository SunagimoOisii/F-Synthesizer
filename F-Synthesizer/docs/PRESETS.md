# PRESETS

`config/presets/` 配下のプリセット一覧と用途メモです。Phase 4 以降、実戦用 `sound_*` は Sound Card として扱い、Play では8カテゴリに整理して表示します。

## 設計方針

- 実戦用 Sound Card は `sound_<category>_<name>` の形式で管理します。
- 実戦カテゴリは `Lead / Guitar / Bass / Pad / Keys / Drums / SFX / Support` に固定します。
- Play では `internal: false` の `sound_*` だけを表示します。
- Advanced では `internal: true` の `demo_*` 検証 preset に到達できます。
- 表示名は短い英語名、説明文は初心者が用途を判断できる日本語短文にします。
- tags は用途と質感を表す英語 lowercase を基本にします。
- `recommendedRange` は Play の preview note と Inspector 表示に使う推奨音域 metadata です。
- `macroHints` は Play の4 macro label / tooltip に使う表示補助 metadata です。DSP 挙動は `GUIMacroMapping` が担当します。
- 実戦 Sound Card は重厚なレトロ感を残しつつ、不要な hiss、耳につく aliasing、過剰 drive、用途不明な noise を避けます。
- `demo_*` は完成音色ではなく機能差分を確認するための検証 preset です。

## 実戦 Sound Card

### Lead

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_lead_blade` | Lead Blade | 鋭い芯を保ちつつ、耳に刺さりにくく整えた主旋律リード。 |
| `sound_lead_metal` | Lead Metal | 金属的な倍音を控えめに整えた強いリード。 |
| `sound_lead_sync` | Lead Sync | 同期感のあるレトロリード。反復フレーズやオブリ向け。 |
| `sound_lead_wide` | Lead Wide | ノイズ感を抑えた、広がりのある太いリード。 |
| `sound_lead_guitarish` | Lead Guitarish | ギター風の歪みを持つロック向けリード。 |
| `sound_lead_arcade_brass` | Lead Arcade Brass | 明るいブラス風リード。短いファンファーレや主旋律のアクセント向け。 |
| `sound_lead_chip_stab` | Lead Chip Stab | チップ音らしい短いスタブ。レトロな合いの手や反復フレーズ向け。 |
| `sound_lead_pwm_fanfare` | Lead PWM Fanfare | PWMの押し出しを使ったファンファーレ系リード。 |
| `sound_lead_sync_bite` | Lead Sync Bite | 同期感の噛みつきを抑えて残したリード。 |
| `sound_lead_airy_pipe` | Lead Airy Pipe | 息の軽さを持つパイプ風リード。 |
| `sound_lead_arcade_reed` | Lead Arcade Reed | レトロなリード楽器風のコール音。 |
| `sound_lead_bright_reed` | Lead Bright Reed | 明るいリード楽器風ソロ音。 |
| `sound_lead_chip_sax` | Lead Chip Sax | チップサックス風の軽いソロ音。 |

### Guitar

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_guitar_clean` | Guitar Clean | クリーンギター風。アルペジオや軽いコード向け。 |
| `sound_guitar_crunch` | Guitar Crunch | 軽く歪んだリズムギター風。 |
| `sound_guitar_drive` | Guitar Drive | 太く歪んだギター風。前に出るリフや単音向け。 |
| `sound_guitar_power` | Guitar Power | 5度とオクターブを足すパワーコード風。 |
| `sound_guitar_mute` | Guitar Mute | 短く刻むミュートギター風。 |

### Bass

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_bass_drive` | Bass Drive | 歪みを整理した主ベース。 |
| `sound_bass_pick` | Bass Pick | ざらつきを抑えた、ピック感のあるFMベース。 |
| `sound_bass_sub` | Bass Sub | 低域の芯を優先したサブベース。 |
| `sound_bass_chip` | Bass Chip | 階段感を抑えたチップ系低域補助。 |
| `sound_bass_rock_pick` | Bass Rock Pick | ピック感を強めたロックベース。 |
| `sound_bass_fuzz` | Bass Fuzz | 荒さを整理したファズベース。 |

### Pad

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_pad_warm` | Pad Warm | 温かく、濁りにくい背景パッド。 |
| `sound_pad_dark` | Pad Dark | 高域のざらつきを避けた暗めの背景パッド。 |
| `sound_pad_wide` | Pad Wide | 明るさを足しすぎない広がりのあるパッド。 |
| `sound_pad_motion` | Pad Motion | ノイズ感を抑えた、ゆっくり動くパッド。 |
| `sound_pad_dark_brass` | Pad Dark Brass | 暗めのブラス質感を背景に置くパッド。 |
| `sound_pad_soft_brass` | Pad Soft Brass | 柔らかいブラス風パッド。 |
| `sound_pad_soft_reed` | Pad Soft Reed | 柔らかいリード合奏風パッド。 |
| `sound_pad_chip_strings` | Pad Chip Strings | チップストリングス風パッド。 |
| `sound_pad_dark_strings` | Pad Dark Strings | 暗めのストリングス風パッド。 |
| `sound_pad_string_motion` | Pad String Motion | ゆっくり動くストリングス風パッド。 |

### Keys

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_keys_electric` | Keys Electric | 丸いエレクトリックキー。コードや短いリフ向け。 |
| `sound_keys_organ` | Keys Organ | 倍音で厚みを足したオルガン系キー。 |
| `sound_keys_rock_organ` | Keys Rock Organ | 押し出しを足したロック向けオルガン。 |
| `sound_keys_bell` | Keys Bell | 金属感を控えめにしたベルキー。 |
| `sound_keys_pluck` | Keys Pluck | 耳に残るざらつきを抑えた短いプラック音。 |
| `sound_keys_chip_piano` | Keys Chip Piano | チップピアノ風のキー。 |
| `sound_keys_fm_clav` | Keys FM Clav | FMクラビ風のキー。 |

### Drums

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_drums_arcade` | Drums Arcade | ノイズ成分を整理した標準的なアーケードドラム。 |
| `sound_drums_impact` | Drums Impact | 押し出しを残しつつ、過剰な歪みを抑えたドラム。 |
| `sound_drums_glue` | Drums Glue | 曲中でまとまりやすい、派手すぎないドラム。 |
| `sound_drums_rock` | Drums Rock | ギターやベースと合わせる標準ロックドラム。 |
| `sound_drums_hard` | Drums Hard | 重いリフ向けの硬めのロックドラム。 |

### SFX

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_sfx_laser` | SFX Laser | 高域の刺さりを抑えた短いレーザー効果音。 |
| `sound_sfx_riser` | SFX Riser | 目立ちすぎない場面転換用ノイズライザー。 |
| `sound_sfx_hit_arcade` | SFX Hit Arcade | 短いアーケード風ヒット音。 |
| `sound_sfx_noise_sweep` | SFX Noise Sweep | ノイズを使った短いスイープ効果音。 |

### Support

| Preset | 表示名 | 用途メモ |
|---|---|---|
| `sound_support_hat` | Support Hat | 耳につきにくい薄いハット補助。 |
| `sound_support_pulse` | Support Pulse | 控えめな PSG pulse 補助。 |
| `sound_support_triangle` | Support Triangle | 目立ちすぎない PSG triangle 補助。 |
| `sound_support_bottle_fm` | Support Bottle FM | 瓶のようなFM質感の補助音。 |
| `sound_support_flute_chip` | Support Flute Chip | チップフルート風の補助音。 |
| `sound_support_recorder` | Support Recorder | 柔らかいリコーダー風補助音。 |
| `sound_support_fm_clarinet` | Support FM Clarinet | FMクラリネット風の補助音。 |
| `sound_support_nasal_reed` | Support Nasal Reed | 鼻にかかったリード風補助音。 |
| `sound_support_pizzicato` | Support Pizzicato | 短いピチカート風補助音。 |

## 機能デモ

`demo_*` は完成音色ではなく、機能差分を確認するための検証 preset です。Play には表示せず、Advanced から到達します。

## 使い方

CLI:

```powershell
F-Synthesizer.exe --cli --preset sound_lead_blade
F-Synthesizer.exe --cli --preset sound_guitar_power
F-Synthesizer.exe --cli --preset sound_drums_arcade
F-Synthesizer.exe --cli --preset demo_filter_ladder_drive
```

GUI:

- Play で Sound Card を選び、Tone Preview や Drum Pad で短く試聴します。
- Compose では曲中の音量と左右を調整します。
- 詳細音色、Master FX、Mixer/割当、検証 preset は Advanced で扱います。
