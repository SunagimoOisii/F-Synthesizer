# 髻ｳ貅先磁邯壹・繝ｬ繧､繝悶ャ繧ｯ・亥・騾壼悄蜿ｰ縺ｨ譁ｹ蠑丞挨繧ｬ繧､繝会ｼ・
譛邨よ峩譁ｰ: 2026-02-23  
迥ｶ諷・ Draft・亥ｮ溯｣・°逕ｨ逕ｨ・・
## 1. 逶ｮ逧・
- 蜈ｱ騾壼悄蜿ｰ繧貞・蛻ｩ逕ｨ縺励∵婿蠑上＃縺ｨ縺ｮ驥崎､・ｮ溯｣・ｒ髦ｲ縺舌・- 縲後←縺薙↓螳溯｣・☆繧九°縲阪後←縺・磁邯壹☆繧九°縲阪ｒ蝗ｺ螳壹＠縲∵隼菫ｮ邊ｾ蠎ｦ繧剃ｸ翫￡繧九・- 螳溯｣・凾縺ｮ譖ｴ譁ｰ貍上ｌ繧呈ｸ帙ｉ縺吶・
髢｢騾｣:
- `docs/synth-methods/method-boundaries.md`
- `docs/SYNTH_METHODS.md`

## 2. 蜈ｱ騾壼悄蜿ｰ・育樟迥ｶ・・
### 2.1 Oscillator / Source

- 譁ｹ蠑丞崋譛峨・逋ｺ謖ｯ繝ｭ繧ｸ繝・け
- 荳ｻ縺ｪ蝣ｴ謇:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
  - `src/synth/Oscillator.cpp`

### 2.2 Filter・亥・騾夲ｼ・
- 蠖ｹ蜑ｲ:
  - 逋ｺ謖ｯ蠕後・髻ｳ濶ｲ謌仙ｽ｢・・ypass/LP/HP/BP・・- 荳ｻ縺ｪ蝣ｴ謇:
  - `include/SynthEngine/Filter.h`
  - `src/SynthEngine/Filter.cpp`
  - `src/SynthEngine/Voices.cpp`・・oice縺斐→縺ｮ菫晄戟/蛻晄悄蛹厄ｼ・
### 2.3 Modulation・亥・騾夲ｼ・
- 蠖ｹ蜑ｲ:
  - LFO/Envelope/Route 縺ｫ繧医ｋ source 蜃ｺ蜉帙・譎る俣螟牙喧蛻ｶ蠕｡
- 荳ｻ縺ｪ蝣ｴ謇:
  - `include/SynthEngine/Modulation.h`
  - `src/SynthEngine/Modulation.cpp`
  - `src/SynthEngine/Voices.cpp`・・oice縺斐→縺ｮ菫晄戟/蛻晄悄蛹・NoteOn/NoteOff・・  - `src/SynthEngine/Renderer.cpp`・郁ｩ穂ｾ｡邨先棡縺ｮ驕ｩ逕ｨ・・- 迴ｾ迥ｶ莉墓ｧ・
  - Source: `none/lfo1/env2`
  - Destination: `none/pitch/amp/filterCutoff`
  - 螳牙・遲・ amount/depth/rate 縺ｮ clamp縲∫┌蜉ｹroute譎ゅ・譌ｩ譛殲eturn

### 2.4 Parameter Smoothing・亥・騾夲ｼ・
- 蠖ｹ蜑ｲ:
  - 繝代Λ繝｡繝ｼ繧ｿ諤･螟画凾縺ｮ繧ｯ繝ｪ繝・け/繧ｸ繝・ヱ繝ｼ繝弱う繧ｺ菴取ｸ・- 荳ｻ縺ｪ蝣ｴ謇:
  - `include/SynthEngine/Smoothing.h`
  - `src/SynthEngine/Smoothing.cpp`
  - `src/SynthEngine/Voices.cpp`・・oice縺斐→縺ｮ菫晄戟/蛻晄悄蛹厄ｼ・  - `src/SynthEngine/Renderer.cpp`・磯←逕ｨ・・- 迴ｾ迥ｶ莉墓ｧ・
  - one-pole: `current += alpha * (target - current)`
  - `timeMs` 謖・ｮ夲ｼ・0ms` 縺ｯ繝舌う繝代せ・・  - 蛻晄悄謗･邯・ Waveform 縺ｮ `amp/pitch/filterCutoff`

### 2.5 Mix / Output・亥・騾夲ｼ・
- 蠖ｹ蜑ｲ:
  - mute/solo/level/pan/gain
- 荳ｻ縺ｪ蝣ｴ謇:
  - `include/SynthEngine/SynthEngine.h` (`ChannelMixState`)
  - `src/SynthEngine/Renderer.cpp`

### 2.6 Config I/O・亥・騾夲ｼ・
- 蠖ｹ蜑ｲ:
  - source繝代Λ繝｡繝ｼ繧ｿ縺ｮ菫晏ｭ・蠕ｩ蜈・讀懆ｨｼ
- 荳ｻ縺ｪ蝣ｴ謇:
  - `src/config/ConfigLoad.cpp`
  - `src/config/ConfigJsonUtils.cpp`
  - `src/config/ConfigFileInternal.h`
  - `src/config/SourceRegistry.cpp`

### 2.7 GUI・亥・騾夲ｼ・
- 蠖ｹ蜑ｲ:
  - source繝代Λ繝｡繝ｼ繧ｿ邱ｨ髮・・蟾ｮ蛻・ｿ晏ｭ・- 荳ｻ縺ｪ蝣ｴ謇:
  - `src/gui/GUIChannelEditor.cpp`
  - `src/gui/GUIConfigUtils.cpp`

## 3. 謗･邯壹・讓呎ｺ匁焔鬆・ｼ亥・譁ｹ蠑丞・騾夲ｼ・
1. Core螳溯｣・
   - 蜈ｱ騾壼悄蜿ｰ縺ｸ霑ｽ蜉縺吶ｋ縺九∵婿蠑丞崋譛峨↓逡吶ａ繧九°繧貞・縺ｫ豎ｺ繧√ｋ
2. 繝・・繧ｿ繝｢繝・Ν:
   - `SynthEngine.h` 縺ｮ蟇ｾ蠢廚onfig縺ｸ鬆・岼霑ｽ蜉
3. Config:
   - `ConfigLoad` 縺ｫ隱ｭ霎ｼ + 繝舌Μ繝・・繧ｷ繝ｧ繝ｳ
   - `ConfigJsonUtils` 縺ｫ譖ｸ霎ｼ
4. GUI:
   - `GUIChannelEditor` 縺ｫ邱ｨ髮・I霑ｽ蜉
   - `GUIConfigUtils` 縺ｮ豈碑ｼ・未謨ｰ譖ｴ譁ｰ
5. 繝ｬ繝ｳ繝謗･邯・
   - `Renderer` / `Voices` 縺ｫ譛蟆乗磁邯・6. 繝峨く繝･繝｡繝ｳ繝・
   - 譁ｹ蠑丞挨md + `SYNTH_METHODS.md` + 蠢・ｦ√↑繧・`method-boundaries.md`

## 4. 譁ｹ蠑丞挨縺ｮ謗･邯壽婿驥・
### 4.1 Waveform

- 蠖ｹ蜑ｲ:
  - 蝓ｺ譛ｬ豕｢蠖｢ + unison/sub-osc +・亥ｿ・ｦ√↓蠢懊§縺ｦ・牙・騾哥ilter謗･邯・- 謗･邯壼・:
  - `WaveformConfig` -> `Voices` 蛻晄悄蛹・-> `Renderer` 驕ｩ逕ｨ
- 迴ｾ迥ｶ:
  - Filter/Modulation/Smoothing 蜈ｱ騾壼渕逶､縺ｸ謗･邯壽ｸ医∩
  - Config load/save + GUI邱ｨ髮・+ preset diff 豈碑ｼ・∪縺ｧ謗･邯壽ｸ医∩
- 豕ｨ諢・
  - 貂帷ｮ怜粋謌先悽菴難ｼ郁､・尅縺ｪFilter螟芽ｪｿ・峨・ waveform縺ｫ蝓九ａ霎ｼ縺ｾ縺ｪ縺・
### 4.2 Noise

- 蠖ｹ蜑ｲ:
  - 繝弱う繧ｺ逕滓・
- 謗･邯壽耳螂ｨ:
  - 蜈ｱ騾哥ilter繧貞・蛻ｩ逕ｨ縺励※髻ｳ濶ｲ謌仙ｽ｢
- Parameter Smoothing 譁ｹ驥晢ｼ・hase E 蛻､螳夲ｼ・
  - 迴ｾ譎らせ縺ｯ譛ｪ謗･邯夲ｼ亥愛譁ｭ縺ｮ縺ｿ遒ｺ螳夲ｼ・  - 逅・罰: 迴ｾ迥ｶNoise縺ｯ蜊倡ｴ泌・蜉帙〒縲√∪縺壹・Filter謗･邯壹・蜆ｪ蜈亥ｺｦ縺碁ｫ倥＞
  - 谺｡蛟呵｣・ Filter蟆主・蠕後・ `filterCutoff` smoothing
- 豕ｨ諢・
  - 繝弱う繧ｺ蝗ｺ譛画紛蠖｢縺ｨ貂帷ｮ怜・騾壽ｩ溯・繧呈ｷｷ蝨ｨ縺輔○縺ｪ縺・
### 4.3 FM

- 蠖ｹ蜑ｲ:
  - 繧ｪ繝壹Ξ繝ｼ繧ｿ螟芽ｪｿ
- 謗･邯壽耳螂ｨ:
  - FM蜃ｺ蜉帛ｾ梧ｮｵ縺ｫ蜈ｱ騾哥ilter繧呈磁邯壼庄閭ｽ
- Parameter Smoothing 譁ｹ驥晢ｼ・hase E 蛻､螳夲ｼ・
  - 迴ｾ譎らせ縺ｯ譛ｪ謗･邯夲ｼ亥愛譁ｭ縺ｮ縺ｿ遒ｺ螳夲ｼ・  - 逅・罰: 蜈医↓FM蛛ｴ縺ｮ諡｡蠑ｵ霆ｸ・・index`/operator險ｭ險茨ｼ峨ｒ蝗ｺ螳壹＠縺ｦ縺九ｉ謗･邯壹＠縺滓婿縺悟・險ｭ險医ｒ貂帙ｉ縺帙ｋ
  - 谺｡蛟呵｣・ `index` smoothing・育洒譎ょｮ壽焚・峨→ `pitchMul` smoothing
- 豕ｨ諢・
  - FM繧｢繝ｫ繧ｴ繝ｪ繧ｺ繝縺ｨ貂帷ｮ礼ｳｻ繝代Λ繝｡繝ｼ繧ｿ繧呈ｷｷ蜷後＠縺ｪ縺・
### 4.4 Drum / DrumKit

- 蠖ｹ蜑ｲ:
  - one-shot謇捺･ｽ蝎ｨ
- 謗･邯壽耳螂ｨ:
  - 蠢・ｦ∵怙蟆城剞縺ｧ蜈ｱ騾哥ilter繧呈磁邯・- Parameter Smoothing 譁ｹ驥晢ｼ・hase E 蛻､螳夲ｼ・
  - 迴ｾ譎らせ縺ｯ譛ｪ謗･邯夲ｼ亥愛譁ｭ縺ｮ縺ｿ遒ｺ螳夲ｼ・  - 逅・罰: one-shot縺ｮ繧｢繧ｿ繝・け繧帝・繧峨○繧九Μ繧ｹ繧ｯ縺碁ｫ倥＞
  - 谺｡蛟呵｣・ 繝ｪ繝ｪ繝ｼ繧ｹ蟶ｯ蝓溘・縺ｿ髯仙ｮ夐←逕ｨ・・rum type蛻･縺ｫ蜿ｯ蜷ｦ蛻・ｲ撰ｼ・- 豕ｨ諢・
  - 繧｢繧ｿ繝・け蜉｣蛹悶ｒ驕ｿ縺代ｋ縺溘ａ縲∵婿蠑丞挨縺ｫ驕ｩ逕ｨ蜿ｯ蜷ｦ繧貞愛譁ｭ

### 4.5 貂帷ｮ怜粋謌撰ｼ域悴螳溯｣・ｼ・
- 蠖ｹ蜑ｲ:
  - 蜈ｱ騾哥ilter蝓ｺ逶､ + 螟芽ｪｿ蝓ｺ逶､縺ｮ譛ｬ菴灘喧
- 謗･邯壽婿驥・
  - waveform/noise/fm縺ｮ逋ｺ謖ｯ邨先棡繧貞・蜉帙→縺励※蜿励￠繧・- 豕ｨ諢・
  - 逋ｺ謖ｯ蝎ｨ繝ｭ繧ｸ繝・け繧呈ｸ帷ｮ怜・縺ｫ蜀榊ｮ溯｣・＠縺ｪ縺・
### 4.6 蜉邂怜粋謌撰ｼ域悴螳溯｣・ｼ・
- 蠖ｹ蜑ｲ:
  - partial鄒､縺ｮ蜷域・
- 謗･邯壽婿驥・
  - 譁ｹ蠑丞崋譛峨・partial邂｡逅・ｒ螳溯｣・＠縲∝ｾ梧ｮｵ縺ｯ蜈ｱ騾哺ix縺ｸ謗･邯・- 豕ｨ諢・
  - waveform縺ｮunison/sub-osc繧偵◎縺ｮ縺ｾ縺ｾ隍・｣ｽ縺励↑縺・
### 4.7 PSG・域悴螳溯｣・ｼ・
- 蠖ｹ蜑ｲ:
  - 蛻ｶ邏・ｻ倥″繝√ャ繝鈴浹貅・- 謗･邯壽婿驥・
  - PSG蝗ｺ譛牙宛邏・ｒ菫晄戟縺励◆縺ｾ縺ｾ蜈ｱ騾哺ix縺ｸ謗･邯・- 豕ｨ諢・
  - 豎守畑繧ｷ繝ｳ繧ｻ讖溯・繧帝℃蠎ｦ縺ｫ豺ｷ縺懊↑縺・
### 4.8 PCM・域悴螳溯｣・ｼ・
- 蠖ｹ蜑ｲ:
  - 繧ｵ繝ｳ繝励Ν蜀咲函
- 謗･邯壽婿驥・
  - PCM蜀咲函繧ｨ繝ｳ繧ｸ繝ｳ繧呈婿蠑丞崋譛峨〒螳溯｣・＠縲∝ｾ梧ｮｵ縺ｯ蜈ｱ騾哺ix縺ｸ謗･邯・- 豕ｨ諢・
  - 豕｢蠖｢逋ｺ謖ｯ蝎ｨ螳溯｣・・繧ｳ繝斐・繧帝∩縺代ｋ

## 5.1 Parameter Smoothing 謗･邯壹ユ繝ｳ繝励Ξ繝ｼ繝茨ｼ亥ｰ・擂譁ｹ蠑冗畑・・
- 驕ｩ逕ｨ蟇ｾ雎｡:
  - `amp` / `pitchMul` / `filterCutoffHz` / ・亥ｿ・ｦ√↑繧・`index`・・- 譛蟆丞ｮ溯｣・
  - `VoicesSoA` 縺ｫ `SmoothedParam` state 繧定ｿｽ蜉
  - voice蛻晄悄蛹悶〒 `range/sampleRate/timeMs` 繧定ｨｭ螳・  - `Renderer` 縺ｧ `SetSmoothedTarget -> StepSmoothedParam` 繧帝←逕ｨ
- 蜈ｬ髢玖ｨｭ螳・
  - Config繝｢繝・Ν縺ｸ `smoothing` 繧定ｿｽ蜉
  - `ConfigLoad/ConfigJsonUtils/GUIChannelEditor/GUIConfigUtils` 繧貞酔譎よ峩譁ｰ
- 蜿励￠蜈･繧・
  - 蜷御ｸMIDI縺ｮ ON/OFF 豈碑ｼ・〒蟾ｮ蛻・｢ｺ隱・  - peak/rms/clip 繧定ｨ倬鹸

## 6. 遖∵ｭ｢莠矩・ｼ磯㍾隍・亟豁｢・・
- 蜷後§讖溯・繧定､・焚譁ｹ蠑上∈蛻･螳溯｣・＠縺ｪ縺・- 蜈ｱ騾壼喧縺ｧ縺阪ｋ繧ゅ・繧呈婿蠑丞崋譛峨さ繝ｼ繝峨∈蜈医↓譖ｸ縺九↑縺・- `source.type` 霑ｽ蜉譎ゅ↓ `SourceRegistry` 繧堤ｵ檎罰縺励↑縺・ｮ溯｣・ｒ縺励↑縺・
## 7. 螳溯｣・メ繧ｧ繝・け繝ｪ繧ｹ繝・
- [x] 蠅・阜蛻､譁ｭ繧・`method-boundaries.md` 縺ｫ辣ｧ繧峨＠縺ｦ遒ｺ隱・- [x] `SynthEngine.h` 縺ｮConfig繧呈峩譁ｰ
- [x] `ConfigLoad` / `ConfigJsonUtils` 繧呈峩譁ｰ
- [x] `GUIChannelEditor` / `GUIConfigUtils` 繧呈峩譁ｰ
- [x] 繝ｬ繝ｳ繝邨瑚ｷｯ繧呈磁邯・- [x] AB遒ｺ隱搾ｼ郁ｳ + 蠢・ｦ√↑繧牙酔荳MIDI豈碑ｼ・ｼ・- [x] 繝峨く繝･繝｡繝ｳ繝域峩譁ｰ

## Auto-Generated

<!-- AUTO-GENERATED:BEGIN -->
### Auto Snapshot

- Generated by `scripts/update_synth_docs.ps1`
- Generated at: 2026-02-26 16:16:42

### Foundation Files

- `include/SynthEngine/SynthEngine.h` (updated: 2026-02-23 22:11:53)
- `include/SynthEngine/Filter.h` (updated: 2026-02-23 19:58:17)
- `include/SynthEngine/Modulation.h` (updated: 2026-02-23 22:11:53)
- `src/SynthEngine/Filter.cpp` (updated: 2026-02-23 19:58:41)
- `src/SynthEngine/Modulation.cpp` (updated: 2026-02-24 16:23:05)
- `src/SynthEngine/Renderer.cpp` (updated: 2026-02-23 22:11:53)
- `src/SynthEngine/Voices.cpp` (updated: 2026-02-23 22:11:53)
- `src/config/ConfigLoad.cpp` (updated: 2026-02-23 22:26:30)
- `src/config/ConfigJsonUtils.cpp` (updated: 2026-02-23 20:43:49)
- `src/gui/GUIChannelEditor.cpp` (updated: 2026-02-26 15:36:31)
- `src/gui/GUIConfigUtils.cpp` (updated: 2026-02-23 20:44:11)
<!-- AUTO-GENERATED:END -->














































