# Migration Guide v0.2

## 対象

- v0.1 形式（`channels` 未使用）から v0.2 形式（`channels` 差分）へ移行するユーザー

## 変更概要

- 旧: `midiPath`, `wavPath`, `sampleRate` などのグローバル設定中心
- 新: 上記に加えて `channels` で ch0-15 を個別上書き可能

## 後方互換

- `channels` が無い設定ファイルは引き続き有効
- v0.2 では `DefaultConfig().channelConfigs` をベースに差分適用する

## 最小移行手順

1. 既存 `config/presets/<name>.json` をバックアップ
2. 必要なチャンネルだけ `channels` に差分記述
3. `.\build\x64\Debug\F-Synthesizer.exe --cli --config <file>` で確認
4. GUIから微調整し `Save Preset As` で保存

## 例（最小）

```json
{
  "midiPath": "../../assets/midi/solstice_intro.mid",
  "wavPath": "../../output/solstice_custom.wav",
  "channels": {
    "0": {
      "amp": 0.5,
      "attackSec": 0.02,
      "decaySec": 0.2,
      "sustainLevel": 0.8,
      "releaseSec": 0.3,
      "source": {
        "type": "waveform",
        "wave": "sine"
      }
    }
  }
}
```

## 注意点

- `targetChannel` は `-1` または `0..15`
- `bits` は現在 `16` のみサポート
- `channels` のキーは `0..15`
- `drumkit.map` のキーは `0..127`

## 検証コマンド

```powershell
.\scripts\gui_smoke.ps1
```

追加サンプル:
- `config/samples/channel_minimal.json`
- `config/samples/channel_full.json`
- `config/samples/channel_invalid.json`（失敗系テスト用）
