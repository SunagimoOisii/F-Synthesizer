# Channel Config Schema (v0.2 draft)

## 目的

- ch0-15 の音設定を JSON で定義し、CLI/GUI で共通利用する。
- 既存の `base.json` / `presets/*.json` 互換を維持したまま段階導入する。

## 適用位置

- 既存キー（`midiPath`, `wavPath`, `sampleRate` など）はそのまま使用。
- 新規に `channels` キーを追加する。

```json
{
  "channels": {
    "0": { "... channel config ..." },
    "1": { "... channel config ..." }
  }
}
```

- `channels` 未指定のチャンネルは `DefaultConfig()` の既定値を使用する。
- 指定されたチャンネルのみ上書き（後勝ち）。

## Channel Object

```json
{
  "amp": 0.40,
  "attackSec": 0.04,
  "decaySec": 0.22,
  "sustainLevel": 0.90,
  "releaseSec": 0.35,
  "source": {
    "type": "fm",
    "...type specific fields..."
  }
}
```

## source.type ごとの定義

### 1) waveform

```json
{
  "type": "waveform",
  "wave": "sine|square|saw|triangle",
  "unisonVoices": 1,
  "unisonDetuneCents": 0.0,
  "unisonSpread": 0.0,
  "subOscLevel": 0.0
}
```

### 2) noise

```json
{
  "type": "noise",
  "noise": "white|pink|brown"
}
```

### 3) fm

```json
{
  "type": "fm",
  "carrierWave": "sine|square|saw|triangle",
  "modWave": "sine|square|saw|triangle",
  "carrierRatio": 1.0,
  "modRatio": 2.0,
  "index": 1.2,
  "outLevel": 1.0
}
```

### 4) drum

```json
{
  "type": "drum",
  "drumType": "kick|snare|hat|none",
  "gain": 0.6,
  "baseFreq": 60.0,
  "pitchDrop": 3.0,
  "pitchDecaySec": 0.06,
  "toneFreq": 220.0,
  "toneLevel": 0.55,
  "noiseLevel": 0.35,
  "hpCut": 700.0,
  "lpCut": 6000.0,
  "toneWave": "triangle",
  "noiseType": "white"
}
```

### 5) drumkit

```json
{
  "type": "drumkit",
  "map": {
    "36": { "drumType": "kick", "gain": 0.6, "baseFreq": 60.0 },
    "38": { "drumType": "snare", "gain": 0.6, "toneFreq": 220.0 }
  }
}
```

## バリデーション範囲

- channel key: `0..15`
- `amp`: `0.0..16.0`
- `attackSec/decaySec/releaseSec`: `0.0..30.0`
- `sustainLevel`: `0.0..1.0`
- ratio/index/outLevel: `>= 0.0`
- drum map key: `0..127`
- `unisonVoices`: `1..8`
- `unisonDetuneCents`: `0.0..120.0`
- `unisonSpread`: `0.0..1.0`
- `subOscLevel`: `0.0..2.0`

## 後方互換ルール

- v0.1 形式（`channels` なし）は有効。
- v0.2 形式（`channels` あり）は、既存設定に対する差分上書きとして扱う。
- 不明キーは警告ログのみ（実行継続）。
- 不正値（範囲外・未知 type）はエラー扱い。

## AppConfig 対応方針

- `AppConfig::channelConfigs` を最終反映先として維持（実行時使用）。
- ローダーで `channels` 差分を `channelConfigs` へマージして `Run(config)` に渡す。
- GUIは内部状態を `channels` 相当で持ち、保存時に同形式で書き出す。

## サンプル

- 最小: `config/samples/channel_minimal.json`
- 完全: `config/samples/channel_full.json`
