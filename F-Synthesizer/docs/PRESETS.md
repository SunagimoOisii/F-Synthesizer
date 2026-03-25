# PRESETS

最終更新: 2026-03-25

`config/presets/` 配下のプリセット一覧と用途メモ。
80-90年代ゲーム音源（NES/GB/MD/PC-88）をイメージしたラインナップ。

## 使い方

CLI:

```powershell
# 例: MD系FMブラス
F-Synthesizer.exe --cli --preset fm_brass_md

# 例: NES風ドラム
F-Synthesizer.exe --cli --preset drumkit_nes_style
```

GUI:

- Main 画面の `Preset` コンボから選択して適用する。

## プリセット一覧

### PSG（NES/GB系）

| Preset | wave | 対象ch | 用途メモ |
|---|---|---|---|
| `psg_lead_sq50` | `square` | `0` | NES ch1相当・50%スクエアメロディリード |
| `psg_lead_pulse25` | `pulse` | `0` | NES ch2相当・25%パルスブライトリード |
| `psg_bass_tri` | `triangle` | `0` | NES ch3相当・トライアングルベース |
| `psg_noise_sfx` | `noise` | `0` | NESノイズch相当・ハット/効果音用 |

### FM（Mega Drive/PC-88系）

FM プリセットはすべて `Env2 → fm.index` ルートを持ち、アタック時に倍音が広がって減衰する。

| Preset | algorithm | 対象ch | 用途メモ |
|---|---|---|---|
| `fm_brass_md` | `1`（2+2ペア） | `0` | MD系ブラス・明るいアタックから減衰 |
| `fm_lead_pierce` | `0`（2-op） | `0` | 抜けるリード・Env2+LFOビブラート |
| `fm_bass_pick` | `3`（チェーン） | `0` | ピック感FMベース・瞬間的なindex増大 |
| `fm_string_warm` | `2`（1mod+3car） | `0` | 温かみのある弦・緩やかなindex減衰 |
| `fm_bell_ep` | `0`（2-op） | `0` | ベル/EP混合・ratio=3.5のindex全減衰 |

### Waveform（SNES寄り・汎用補完）

| Preset | wave | 対象ch | 用途メモ |
|---|---|---|---|
| `wave_lead_vib` | `saw` | `0` | LFOビブラート付きJRPG風リード |
| `wave_pad_sweep` | `square` | `0` | Env2フィルタースイープPad（4ユニゾン） |
| `wave_bass_solid` | `saw` | `0` | サブオシレータ強め・低域土台ベース |

### Drumkit

| Preset | 対象ch | 用途メモ |
|---|---|---|
| `drumkit_nes_style` | `9` | 短ADSR・triangleキック・NES感 |
| `drumkit_md_style` | `9` | 深いsineキック・blueノイズスネア・MD感 |

## modulation 設計メモ

| route | 使用プリセット | 効果 |
|---|---|---|
| `Env2 → fm.index` | 全FMプリセット | アタックで倍音展開→減衰で音色変化 |
| `LFO → pitchMul` | `fm_lead_pierce`, `wave_lead_vib` | ビブラート（±24cents, 5.5Hz） |
| `Env2 → filterCutoffHz` | `wave_pad_sweep`, `wave_bass_solid` | アタックでフィルター開放→徐々に閉じる |

## 補足

- プリセットは `base.json` に差分適用される。
- `source.smoothing` は `waveform` のみ有効。
- FM の `modulation` ブロックは `source` 内に記述する。
- 各方式の仕様は `docs/synth-methods/` 配下を正本とする。
- 旧プリセットは `config/presets/_archive/` に退避している。
