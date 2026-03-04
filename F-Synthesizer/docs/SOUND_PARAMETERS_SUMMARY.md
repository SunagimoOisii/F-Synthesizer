# SOUND_PARAMETERS_SUMMARY

日常編集で使う最小要約。詳細仕様は `SOUND_PARAMETERS.md` を参照。

## Source Type

- `waveform`: 一般的な音色作成向け
- `noise`: ホワイト/ピンク/ブラウン等のノイズ素材
- `fm`: 金属音・ベル系・倍音変化
- `drum` / `drumkit`: 打楽器向け

## Waveform

- 基本波形: `sine/square/saw/triangle`
- 重要パラメータ: `gain`, `detune`, `pan`, `unison`, `filter`
- 品質関連: `quality`（帯域制限の有無に影響）

## Noise

- `type`: `white/pink/brown/blue`
- 重要パラメータ: `gain`, `filter`, `envelope`

## FM

- `carrierRatio`, `modRatio`, `modIndex` が音色の核
- `modIndex` が大きいほど倍音が増える

## Drum / DrumKit

- Drum: 単発のキック/スネア/ハット系パラメータ
- DrumKit: キー別に Drum を割り当て
- 重要観点: レベル過大によるクリップ回避

## Modulation

- `lfo1` / `env2` をルーティングで対象へ適用
- ルートは最小限から開始し、過変調を避ける

## Smoothing

- 急峻なパラメータ変化のクリック低減用
- 音色比較時は ON/OFF の A/B を推奨

## 実務の推奨手順

1. `waveform` で土台を作る
2. `filter` と `envelope` を調整
3. 必要なら `modulation` を追加
4. 最後に `gain/pan` とクリップを確認
