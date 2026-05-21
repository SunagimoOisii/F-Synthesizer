# チャンネル設定スキーマ（v0.2 draft）

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

## チャンネルオブジェクト

```json
{
  "amp": 0.40,
  "attackSec": 0.04,
  "decaySec": 0.22,
  "sustainLevel": 0.90,
  "releaseSec": 0.35,
  "attackLayer": {
    "enabled": true,
    "type": "pick",
    "level": 0.12,
    "decaySec": 0.03,
    "brightness": 0.45,
    "bodyMix": 0.65,
    "pitchOffsetSemis": -12.0,
    "drive": 0.08
  },
  "bassLayer": {
    "enabled": true,
    "type": "drive",
    "level": 0.28,
    "subLevel": 0.42,
    "bodyLevel": 0.50,
    "gritLevel": 0.34,
    "cutoffHz": 1050.0,
    "drive": 0.32,
    "pitchOffsetSemis": -12.0,
    "velocityToDrive": 0.18,
    "focusHz": 185.0,
    "focusLevel": 0.24,
    "bodySaturation": 0.22,
    "gritTone": 0.46,
    "attackBoost": 0.18,
    "attackDecaySec": 0.04
  },
  "leadLayer": {
    "enabled": true,
    "type": "blade",
    "level": 0.10,
    "edgeLevel": 0.35,
    "bodyLevel": 0.28,
    "detuneCents": 4.0,
    "pitchBendSemis": 0.4,
    "bendDecaySec": 0.04,
    "attackBoost": 0.16,
    "attackDecaySec": 0.035,
    "drive": 0.08
  },
  "source": {
    "type": "fm",
    "...type specific fields..."
  }
}
```

`attackLayer` は省略可能。NoteOn直後だけ鳴る内部生成の補助音で、`source` 本体を置き換えずにアタックだけを足す。`type` は `pick|brass|metal`。ピック感、ブラスの吹き始め、金属打撃を足す用途で、強くしすぎるとピーク過多やチープなクリック感につながる。

`bassLayer` は省略可能。ベース向けに持続する内部生成の補助音で、`source` 本体へ低域の芯、胴、歪んだ倍音、曲中で読める中低域の焦点を重ねる。`type` は `sub|drive|grit`。強くしすぎると低域過多、音割れ、キックとの衝突につながるため、ベースチャンネルへ薄くから中程度に使う。

`leadLayer` は省略可能。主旋律向けに内部生成の硬い頭、短いしゃくり、薄い二重化を重ねる。`source` 本体を置き換えず、FM/analog/waveformのリードを前に出す用途で使う。`type` は `blade|brass|edge`。強くしすぎると濁り、過剰な金属感、ピーク過多につながる。

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
  "algorithm": 0,
  "feedback": 0.08,
  "ops": [
    {
      "wave": "sine|square|saw|triangle",
      "ratio": 2.0,
      "level": 1.0,
      "index": 2.4,
      "levelEnv": {
        "attackSec": 0.0,
        "decaySec": 0.08,
        "sustainLevel": 0.85,
        "releaseSec": 0.06,
        "curve": 0.3
      },
      "indexEnv": {
        "attackSec": 0.0,
        "decaySec": 0.06,
        "sustainLevel": 0.4,
        "releaseSec": 0.04,
        "curve": 0.7
      }
    }
  ],
  "filterMode": "bypass|lowpass|highpass|bandpass",
  "filterCutoffHz": 8000.0,
  "filterResonance": 0.707,
  "drive": 0.0
}
```

`ops` は最大4要素。`levelEnv` はそのopの出力レベル、`indexEnv` は変調深さに掛かる個別エンベロープ。旧2op省略形式は使わない。

### 4) drum

```json
{
  "type": "drum",
  "drumType": "kick|snare|hat|tom|rim|clap|crash|ride|none",
  "gain": 0.6,
  "bodyFreq": 58.0,
  "bodyLevel": 0.9,
  "bodyDecaySec": 0.18,
  "pitchStart": 4.5,
  "pitchDecaySec": 0.06,
  "transientLevel": 0.3,
  "transientDecaySec": 0.008,
  "noiseLevel": 0.35,
  "snapLevel": 0.85,
  "snapDecaySec": 0.055,
  "metalLevel": 0.6,
  "airLevel": 0.3,
  "decaySec": 0.05,
  "hpCut": 900.0,
  "lpCut": 5600.0,
  "drive": 0.25,
  "noiseColor": "white|pink|brown|blue",
  "velocityToTone": 0.25,
  "velocityToDecay": 0.10,
  "humanizePitchCents": 2.0,
  "humanizeDecayPct": 0.08
}
```

`drumType` ごとに使う主な項目は異なる。`kick` は `bodyFreq` / `pitchStart` / `transientLevel` / `bodyLevel`、`snare` は `bodyFreq` / `bodyLevel` / `snapLevel` / `snapDecaySec`、`hat` / `crash` / `ride` は `metalLevel` / `airLevel` / `decaySec`、`tom` は `bodyFreq` / `pitchStart` / `bodyDecaySec`、`rim` / `clap` は `transientLevel` / `noiseLevel` を中心に調整する。`velocityToTone` / `velocityToDecay` はMIDI velocityによる音色変化、`humanizePitchCents` / `humanizeDecayPct` は同音連打の微差に使う。

### 5) drumkit

```json
{
  "type": "drumkit",
  "map": {
    "36": { "drumType": "kick", "gain": 0.8, "bodyFreq": 54.0, "pitchStart": 4.8 },
    "38": { "drumType": "snare", "gain": 0.8, "bodyFreq": 230.0, "snapLevel": 0.85 },
    "41": { "drumType": "tom", "gain": 0.8, "bodyFreq": 110.0, "pitchStart": 2.4 },
    "49": { "drumType": "crash", "gain": 0.7, "metalLevel": 0.8, "decaySec": 0.45 }
  }
}
```

## バリデーション範囲

- channel key: `0..15`
- `amp`: `0.0..16.0`
- `attackSec/decaySec/releaseSec`: `0.0..30.0`
- `sustainLevel`: `0.0..1.0`
- `attackLayer.level/bodyMix/brightness/drive`: `0.0..1.0`
- `attackLayer.decaySec`: `0.001..0.25`
- `attackLayer.pitchOffsetSemis`: `-24.0..24.0`
- `bassLayer.level/subLevel/bodyLevel/gritLevel/drive/velocityToDrive`: `0.0..1.0`
- `bassLayer.focusLevel/bodySaturation/gritTone/attackBoost`: `0.0..1.0`
- `bassLayer.cutoffHz`: `40.0..8000.0`
- `bassLayer.focusHz`: `60.0..1200.0`
- `bassLayer.pitchOffsetSemis`: `-24.0..24.0`
- `bassLayer.attackDecaySec`: `0.005..0.25`
- `leadLayer.level/edgeLevel/bodyLevel/attackBoost/drive`: `0.0..1.0`
- `leadLayer.detuneCents`: `-50.0..50.0`
- `leadLayer.pitchBendSemis`: `-12.0..12.0`
- `leadLayer.bendDecaySec/attackDecaySec`: `0.005..0.25`
- ratio / index / level: `>= 0.0`
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

古いフェーズ別検証サンプルは保持しない。必要な検証ケースは
`scripts/check.ps1 -RunRuntimeSmoke` へ直接追加する。
