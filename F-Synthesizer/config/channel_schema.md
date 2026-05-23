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
    "drive": 0.08,
    "characterLevel": 0.14,
    "characterTone": 0.62,
    "biteLevel": 0.12,
    "biteDecaySec": 0.024,
    "wobbleDepthCents": 4.0,
    "wobbleRateHz": 4.8
  },
  "chordLayer": {
    "enabled": true,
    "level": 0.12,
    "intervalsSemis": [0, 7, 12, 15],
    "voiceLevels": [1.0, 0.62, 0.42, 0.25],
    "detuneCents": 4.0,
    "spread": 0.35,
    "cutoffHz": 2400.0,
    "drive": 0.04
  },
  "padLayer": {
    "enabled": true,
    "level": 0.14,
    "octaveLevel": 0.18,
    "detuneCents": 6.0,
    "spread": 0.42,
    "fadeInSec": 0.18,
    "brightness": 0.28,
    "motionDepth": 0.08,
    "motionRateHz": 0.18,
    "cutoffHz": 1600.0,
    "drive": 0.02
  },
  "expressionMap": {
    "enabled": true,
    "velocityCurve": 0.9,
    "velocityToAmp": 0.9,
    "velocityToBrightness": 0.14,
    "velocityToFmIndex": 0.12,
    "velocityToAttack": 0.10,
    "velocityToBass": 0.0,
    "velocityToLead": 0.16,
    "velocityToChord": 0.12,
    "velocityToPad": 0.16,
    "modWheelToBrightness": 0.10,
    "modWheelToPad": 0.16,
    "pressureToDrive": 0.04,
    "cc74ToBrightness": 0.20,
    "cc74ToPadBrightness": 0.22
  },
  "source": {
    "type": "fm",
    "...type specific fields..."
  }
}
```

`attackLayer` は省略可能。NoteOn直後だけ鳴る内部生成の補助音で、`source` 本体を置き換えずにアタックだけを足す。`type` は `pick|brass|metal`。ピック感、ブラスの吹き始め、金属打撃を足す用途で、強くしすぎるとピーク過多やチープなクリック感につながる。

`bassLayer` は省略可能。ベース向けに持続する内部生成の補助音で、`source` 本体へ低域の芯、胴、歪んだ倍音、曲中で読める中低域の焦点を重ねる。`type` は `sub|drive|grit`。強くしすぎると低域過多、音割れ、キックとの衝突につながるため、ベースチャンネルへ薄くから中程度に使う。

`leadLayer` は省略可能。主旋律向けに内部生成の硬い頭、短いしゃくり、薄い二重化、FM金属寄りの持続倍音を重ねる。`source` 本体を置き換えず、FM/analog/waveformのリードを前に出す用途で使う。`type` は `blade|brass|edge`。`characterLevel` と `biteLevel` を強くしすぎると濁り、過剰な金属感、ピーク過多につながる。

`chordLayer` は省略可能。入力ノートに固定voicingの追加音を重ねる内部生成レイヤーで、和音やブラスの厚みを足す。`intervalsSemis` は入力ノートからの半音差、`voiceLevels` は各voiceの相対音量で、最大4要素まで使う。強くしすぎるとコードが濁り、主旋律やベースを覆う。

`padLayer` は省略可能。暗い持続音、遅いfade、軽いdetune、薄い揺れを重ねる背景用レイヤー。`brightness` と `cutoffHz` は控えめを基本にし、主旋律を邪魔しない厚みとして使う。上げすぎると全体が曇る。

`pluckLayer` は省略可能。短い撥弦感、軽い明るさ、薄いクリックを足す。`noiseMix` は主成分ではなく、立ち上がりの質感付けに限定する。

`stringLayer` は省略可能。弦のような持続、揺れ、広がりを足す。`bowLevel` は弓ノイズの量なので、Pad以外の実戦 preset では原則使わない。

`bodyLayer` は省略可能。入力音に共鳴を足す後段レイヤー。`mode` は `harmonic|box|metal`。`harmonic` は倍音寄りでPad向け、`box` は箱鳴りでPluck向け、`metal` は検証や効果音向け。Organ / clean Keys / Lead に入れると意図しない音程や濁りになりやすい。

`harmonicLayer` は省略可能。入力ノートの整数倍音だけを sine 加算で足す補助レイヤー。`harmonicLevels` は 1, 2, 3, 4, 5, 6, 8, 10 倍音の相対量で、ChordLayer のように別音程は足さない。Organ / clean Keys で、String や Body を混ぜずに中域の厚みを作る用途で使う。

`expressionMap` は省略可能。MIDI velocity を音量だけでなく、明るさ、FM index、attack/bass/lead/chord/pad/pluck/string/body layer量、driveへ薄く割り当てる。`velocityCurve` は 1.0 が標準で、1.0未満は弱音を持ち上げ、1.0超は強弱差を広げる。`modWheelToPad` はPad量、`cc74ToPadBrightness` はPadの明るさへ追加で効く。使いすぎると強velocityだけ音色が暴れ、ピーク過多や不自然な打ち込み感につながる。

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

### 4) drumkit

```json
{
  "type": "drumkit",
  "drumBus": {
    "enabled": true,
    "level": 0.98,
    "attackTrim": 0.30,
    "sustainLift": 0.18,
    "glue": 0.45,
    "presenceCut": 0.25,
    "lowTighten": 0.20,
    "roomSend": 0.12,
    "driveTrim": 0.18
  },
  "velocityCeiling": 0.95,
  "velocityCurve": 1.05,
  "map": {
    "36": { "drumType": "kick", "gain": 0.8, "bodyFreq": 54.0, "pitchStart": 4.8 },
    "38": { "drumType": "snare", "gain": 0.8, "bodyFreq": 230.0, "snapLevel": 0.85 },
    "41": { "drumType": "tom", "gain": 0.8, "bodyFreq": 110.0, "pitchStart": 2.4 },
    "49": { "drumType": "crash", "gain": 0.7, "metalLevel": 0.8, "decaySec": 0.45 }
  }
}
```

`drumBus` は drumkit 全体を合算した後の配置処理。`attackTrim` は先端を抑え、`sustainLift` は胴と余韻を補い、`glue` はピークをまとめ、`presenceCut` はスネア/ハットの前面感を抑える。`lowTighten` はキック低域を締め、`roomSend` は短い部屋鳴りを足し、`driveTrim` は歪みの張り出しを少し丸める。`velocityCeiling` / `velocityCurve` は外部MIDIの強velocityでドラムだけが飛び出すのを抑える。

`map` の各 note value は DrumConfig を表す。`drumType` ごとに使う主な項目は異なる。`kick` は `bodyFreq` / `pitchStart` / `transientLevel` / `bodyLevel`、`snare` は `bodyFreq` / `bodyLevel` / `snapLevel` / `snapDecaySec`、`hat` / `crash` / `ride` は `metalLevel` / `airLevel` / `decaySec`、`tom` は `bodyFreq` / `pitchStart` / `bodyDecaySec`、`rim` / `clap` は `transientLevel` / `noiseLevel` を中心に調整する。

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
- `leadLayer.level/edgeLevel/bodyLevel/attackBoost/drive/characterLevel/characterTone/biteLevel`: `0.0..1.0`
- `leadLayer.detuneCents`: `-50.0..50.0`
- `leadLayer.pitchBendSemis`: `-12.0..12.0`
- `leadLayer.bendDecaySec/attackDecaySec/biteDecaySec`: `0.005..0.25`
- `leadLayer.wobbleDepthCents`: `0.0..30.0`
- `leadLayer.wobbleRateHz`: `0.0..12.0`
- `chordLayer.level/drive/spread`: `0.0..1.0`
- `chordLayer.intervalsSemis`: 各要素 `-24..24`、最大4要素
- `chordLayer.voiceLevels`: 各要素 `0.0..1.0`、最大4要素
- `chordLayer.detuneCents`: `0.0..50.0`
- `chordLayer.cutoffHz`: `80.0..10000.0`
- `padLayer.level/octaveLevel/spread/brightness/motionDepth/drive`: `0.0..1.0`
- `padLayer.detuneCents`: `0.0..80.0`
- `padLayer.fadeInSec`: `0.005..5.0`
- `padLayer.motionRateHz`: `0.0..8.0`
- `padLayer.cutoffHz`: `80.0..10000.0`
- `pluckLayer.level/brightness/noiseMix/bodySend/drive`: `0.0..1.0`
- `pluckLayer.decaySec`: `0.02..2.0`
- `pluckLayer.pitchOffsetSemis`: `-24.0..24.0`
- `stringLayer.level/bowLevel/spread/brightness/motionDepth/bodySend/drive`: `0.0..1.0`
- `stringLayer.detuneCents`: `0.0..80.0`
- `stringLayer.fadeInSec`: `0.005..3.0`
- `stringLayer.motionRateHz`: `0.0..12.0`
- `bodyLayer.mode`: `harmonic|box|metal`
- `bodyLayer.mix/size/tone/damping/stereo/drive`: `0.0..1.0`
- `harmonicLayer.level/brightness/keyClick/releaseDamp/drive/stereo`: `0.0..1.0`
- `harmonicLayer.harmonicLevels`: 各要素 `0.0..1.0`、8要素、対象倍音は `1,2,3,4,5,6,8,10`
- `harmonicLayer.attackSec`: `0.001..0.25`
- `expressionMap.velocityCurve`: `0.2..3.0`
- `expressionMap.velocityToAmp/velocityToFmIndex/velocityToAttack/velocityToBass/velocityToLead/velocityToChord/velocityToPad/velocityToPluck/velocityToString/velocityToBody/modWheelToPad/modWheelToString/pressureToDrive/pressureToFilterDrive`: `0.0..1.0`
- `expressionMap.velocityToBrightness/modWheelToBrightness/cc74ToBrightness/cc74ToPadBrightness/cc74ToStringBrightness`: `-1.0..1.0`
- ratio / index / level: `>= 0.0`
- drum map key: `0..127`
- `drumBus.level`: `0.0..2.0`
- `drumBus.attackTrim/sustainLift/glue/presenceCut/lowTighten/roomSend/driveTrim`: `0.0..1.0`
- `drumkit.velocityCeiling`: `0.0..1.0`
- `drumkit.velocityCurve`: `0.2..3.0`
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
