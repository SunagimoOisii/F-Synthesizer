# Migration Guide v0.3

## 対象

- v0.2 形式（`channels` 中心）から v0.3 形式（`channelMix` + 単体試聴運用）へ移行するユーザー

## 変更概要

- 旧: `channels` で音色差分を管理
- 新: 上記に加えて `channelMix` で `mute/solo/level/pan/gain` を管理

## 後方互換

- `channelMix` が無い設定ファイルは引き続き有効
- v0.3 では `DefaultConfig().channelMixStates` をベースに差分適用する

## 最小移行手順

1. 既存 `config/presets/<name>.json` をバックアップ
2. 必要なチャンネルだけ `channelMix` に差分記述
3. `.\build\x64\Debug\F-Synthesizer.exe --cli --preset <name>` で反映確認
4. GUI で `Solo Preview` / `Preview Play` を使ってチャンネル単体試聴
5. `Save Preset As` で `channels + channelMix` 差分を保存

## 例（最小）

```json
{
  "midiPath": "../../assets/midi/solstice_intro.mid",
  "wavPath": "../../output/solstice_mix_custom.wav",
  "channelMix": {
    "0": {
      "mute": false,
      "solo": true,
      "level": 1.0,
      "pan": -0.2,
      "gain": 1.1
    },
    "1": {
      "mute": false,
      "solo": false,
      "level": 0.8,
      "pan": 0.2,
      "gain": 1.0
    }
  }
}
```

## 注意点

- `channelMix` のキーは `0..15`
- `level` は `0.0..2.0`
- `pan` は `-1.0..1.0`
- `gain` は `0.0..4.0`
- `Preview Play` は試聴時に一時solo化し、終了時に元状態へ復帰する

## 検証コマンド

```powershell
.\scripts\gui_smoke.ps1
```

追加サンプル:
- `config/samples/mix_all_mute.json`（全chミュート）
- `config/samples/channel_mix_invalid.json`（失敗系テスト用）
