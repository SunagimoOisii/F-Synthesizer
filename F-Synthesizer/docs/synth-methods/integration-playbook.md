# 鬮ｻ・ｳ雋・・逎・け螢ｹ繝ｻ郢晢ｽｬ郢ｧ・､郢晄じ繝｣郢ｧ・ｯ繝ｻ莠･繝ｻ鬨ｾ螢ｼ謔・愾・ｰ邵ｺ・ｨ隴・ｽｹ陟台ｸ樊肩郢ｧ・ｬ郢ｧ・､郢昜ｼ夲ｽｼ繝ｻ
隴崢驍ｨ繧亥ｳｩ隴・ｽｰ: 2026-02-23  
霑･・ｶ隲ｷ繝ｻ Draft繝ｻ莠･・ｮ貅ｯ・｣繝ｻﾂｰ騾包ｽｨ騾包ｽｨ繝ｻ繝ｻ
## 1. 騾ｶ・ｮ騾ｧ繝ｻ
- 陷茨ｽｱ鬨ｾ螢ｼ謔・愾・ｰ郢ｧ雋槭・陋ｻ・ｩ騾包ｽｨ邵ｺ蜉ｱﾂ竏ｵ蟀ｿ陟台ｸ奇ｼ・ｸｺ・ｨ邵ｺ・ｮ鬩･蟠趣ｽ､繝ｻ・ｮ貅ｯ・｣繝ｻ・帝ｫｦ・ｲ邵ｺ闊個繝ｻ- 邵ｲ蠕娯・邵ｺ阮吮・陞ｳ貅ｯ・｣繝ｻ笘・ｹｧ荵敖ｰ邵ｲ髦ｪﾂ蠕娯・邵ｺ繝ｻ逎・け螢ｹ笘・ｹｧ荵敖ｰ邵ｲ髦ｪ・定摎・ｺ陞ｳ螢ｹ・邵ｲ竏ｵ髫ｼ闖ｫ・ｮ驍奇ｽｾ陟趣ｽｦ郢ｧ蜑・ｽｸ鄙ｫ・｡郢ｧ荵敖繝ｻ- 陞ｳ貅ｯ・｣繝ｻ蜃ｾ邵ｺ・ｮ隴厄ｽｴ隴・ｽｰ雋堺ｸ奇ｽ檎ｹｧ蜻茨ｽｸ蟶呻ｽ臥ｸｺ蜷ｶﾂ繝ｻ
鬮｢・｢鬨ｾ・｣:
- `docs/synth-methods/method-boundaries.md`
- `docs/SYNTH_METHODS.md`

## 2. 陷茨ｽｱ鬨ｾ螢ｼ謔・愾・ｰ繝ｻ閧ｲ讓溯ｿ･・ｶ繝ｻ繝ｻ
### 2.1 Oscillator / Source

- 隴・ｽｹ陟台ｸ槫ｴ玖ｭ帛ｳｨ繝ｻ騾具ｽｺ隰厄ｽｯ郢晢ｽｭ郢ｧ・ｸ郢昴・縺・- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
  - `src/synth/Oscillator.cpp`

### 2.2 Filter繝ｻ莠･繝ｻ鬨ｾ螟ｲ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - 騾具ｽｺ隰厄ｽｯ陟募ｾ後・鬮ｻ・ｳ豼ｶ・ｲ隰御ｻ呻ｽｽ・｢繝ｻ繝ｻypass/LP/HP/BP繝ｻ繝ｻ- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `include/SynthEngine/Filter.h`
  - `src/SynthEngine/Filter.cpp`
  - `src/SynthEngine/Voices.cpp`繝ｻ繝ｻoice邵ｺ譁絶・邵ｺ・ｮ闖ｫ譎・亜/陋ｻ譎・ｄ陋ｹ蜴・ｽｼ繝ｻ
### 2.3 Modulation繝ｻ莠･繝ｻ鬨ｾ螟ｲ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - LFO/Envelope/Route 邵ｺ・ｫ郢ｧ蛹ｻ・・source 陷・ｽｺ陷牙ｸ吶・隴弱ｋ菫｣陞溽甥蝟ｧ陋ｻ・ｶ陟包ｽ｡
- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `include/SynthEngine/Modulation.h`
  - `src/SynthEngine/Modulation.cpp`
  - `src/SynthEngine/Voices.cpp`繝ｻ繝ｻoice邵ｺ譁絶・邵ｺ・ｮ闖ｫ譎・亜/陋ｻ譎・ｄ陋ｹ繝ｻNoteOn/NoteOff繝ｻ繝ｻ  - `src/SynthEngine/Renderer.cpp`繝ｻ驛・ｽｩ遨ゑｽｾ・｡驍ｨ蜈域｣｡邵ｺ・ｮ鬩包ｽｩ騾包ｽｨ繝ｻ繝ｻ- 霑ｴ・ｾ霑･・ｶ闔牙｢難ｽｧ繝ｻ
  - Source: `none/lfo1/env2`
  - Destination: `none/pitch/amp/filterCutoff`
  - 陞ｳ迚吶・驕ｲ繝ｻ amount/depth/rate 邵ｺ・ｮ clamp邵ｲ竏ｫ笏瑚怏・ｹroute隴弱ｅ繝ｻ隴鯉ｽｩ隴帶ｮｲeturn

### 2.4 Parameter Smoothing繝ｻ莠･繝ｻ鬨ｾ螟ｲ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - 郢昜ｻ｣ﾎ帷ｹ晢ｽ｡郢晢ｽｼ郢ｧ・ｿ隲､・･陞溽判蜃ｾ邵ｺ・ｮ郢ｧ・ｯ郢晢ｽｪ郢昴・縺・郢ｧ・ｸ郢昴・繝ｱ郢晢ｽｼ郢晏ｼｱ縺・ｹｧ・ｺ闖ｴ蜿厄ｽｸ繝ｻ- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `include/SynthEngine/Smoothing.h`
  - `src/SynthEngine/Smoothing.cpp`
  - `src/SynthEngine/Voices.cpp`繝ｻ繝ｻoice邵ｺ譁絶・邵ｺ・ｮ闖ｫ譎・亜/陋ｻ譎・ｄ陋ｹ蜴・ｽｼ繝ｻ  - `src/SynthEngine/Renderer.cpp`繝ｻ逎ｯ竊宣包ｽｨ繝ｻ繝ｻ- 霑ｴ・ｾ霑･・ｶ闔牙｢難ｽｧ繝ｻ
  - one-pole: `current += alpha * (target - current)`
  - `timeMs` 隰悶・・ｮ螟ｲ・ｼ繝ｻ0ms` 邵ｺ・ｯ郢晁・縺・ｹ昜ｻ｣縺帙・繝ｻ  - 陋ｻ譎・ｄ隰暦ｽ･驍ｯ繝ｻ Waveform 邵ｺ・ｮ `amp/pitch/filterCutoff`

### 2.5 Mix / Output繝ｻ莠･繝ｻ鬨ｾ螟ｲ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - mute/solo/level/pan/gain
- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `include/SynthEngine/SynthEngine.h` (`ChannelMixState`)
  - `src/SynthEngine/Renderer.cpp`

### 2.6 Config I/O繝ｻ莠･繝ｻ鬨ｾ螟ｲ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - source郢昜ｻ｣ﾎ帷ｹ晢ｽ｡郢晢ｽｼ郢ｧ・ｿ邵ｺ・ｮ闖ｫ譎擾ｽｭ繝ｻ陟包ｽｩ陷医・隶諛・ｽｨ・ｼ
- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `src/config/ConfigLoad.cpp`
  - `src/config/ConfigJsonUtils.cpp`
  - `src/config/ConfigFileInternal.h`
  - `src/config/SourceRegistry.cpp`

### 2.7 GUI繝ｻ莠･繝ｻ鬨ｾ螟ｲ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - source郢昜ｻ｣ﾎ帷ｹ晢ｽ｡郢晢ｽｼ郢ｧ・ｿ驍ｱ・ｨ鬮ｮ繝ｻ繝ｻ陝ｾ・ｮ陋ｻ繝ｻ・ｿ譎擾ｽｭ繝ｻ- 闕ｳ・ｻ邵ｺ・ｪ陜｣・ｴ隰・:
  - `src/gui/GUIChannelEditor.cpp`
  - `src/gui/GUIConfigUtils.cpp`

## 3. 隰暦ｽ･驍ｯ螢ｹ繝ｻ隶灘綜・ｺ蛹∫・鬯・・・ｼ莠･繝ｻ隴・ｽｹ陟台ｸ槭・鬨ｾ螟ｲ・ｼ繝ｻ
1. Core陞ｳ貅ｯ・｣繝ｻ
   - 陷茨ｽｱ鬨ｾ螢ｼ謔・愾・ｰ邵ｺ・ｸ髴托ｽｽ陷会｣ｰ邵ｺ蜷ｶ・狗ｸｺ荵敖竏ｵ蟀ｿ陟台ｸ槫ｴ玖ｭ帛ｳｨ竊馴｡蜷ｶ・∫ｹｧ荵敖ｰ郢ｧ雋槭・邵ｺ・ｫ雎趣ｽｺ郢ｧ竏夲ｽ・2. 郢昴・繝ｻ郢ｧ・ｿ郢晢ｽ｢郢昴・ﾎ・
   - `SynthEngine.h` 邵ｺ・ｮ陝・ｽｾ陟｢蟒嗤nfig邵ｺ・ｸ鬯・・蟯ｼ髴托ｽｽ陷会｣ｰ
3. Config:
   - `ConfigLoad` 邵ｺ・ｫ髫ｱ・ｭ髴趣ｽｼ + 郢晁・ﾎ懃ｹ昴・繝ｻ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ
   - `ConfigJsonUtils` 邵ｺ・ｫ隴厄ｽｸ髴趣ｽｼ
4. GUI:
   - `GUIChannelEditor` 邵ｺ・ｫ驍ｱ・ｨ鬮ｮ繝ｻI髴托ｽｽ陷会｣ｰ
   - `GUIConfigUtils` 邵ｺ・ｮ雎育｢托ｽｼ繝ｻ譛ｪ隰ｨ・ｰ隴厄ｽｴ隴・ｽｰ
5. 郢晢ｽｬ郢晢ｽｳ郢敖隰暦ｽ･驍ｯ繝ｻ
   - `Renderer` / `Voices` 邵ｺ・ｫ隴崢陝・ｹ礼｣・け繝ｻ6. 郢晏ｳｨ縺冗ｹ晢ｽ･郢晢ｽ｡郢晢ｽｳ郢昴・
   - 隴・ｽｹ陟台ｸ樊肩md + `SYNTH_METHODS.md` + 陟｢繝ｻ・ｦ竏壺・郢ｧ繝ｻ`method-boundaries.md`

## 4. 隴・ｽｹ陟台ｸ樊肩邵ｺ・ｮ隰暦ｽ･驍ｯ螢ｽ蟀ｿ鬩･繝ｻ
### 4.1 Waveform

- 陟厄ｽｹ陷托ｽｲ:
  - 陜難ｽｺ隴幢ｽｬ雎包ｽ｢陟厄ｽ｢ + unison/sub-osc +繝ｻ莠･・ｿ繝ｻ・ｦ竏壺・陟｢諛環ｧ邵ｺ・ｦ繝ｻ迚吶・鬨ｾ蜩･ilter隰暦ｽ･驍ｯ繝ｻ- 隰暦ｽ･驍ｯ螢ｼ繝ｻ:
  - `WaveformConfig` -> `Voices` 陋ｻ譎・ｄ陋ｹ繝ｻ-> `Renderer` 鬩包ｽｩ騾包ｽｨ
- 霑ｴ・ｾ霑･・ｶ:
  - Filter/Modulation/Smoothing 陷茨ｽｱ鬨ｾ螢ｼ貂暮ｶ・､邵ｺ・ｸ隰暦ｽ･驍ｯ螢ｽ・ｸ蛹ｻ竏ｩ
  - Config load/save + GUI驍ｱ・ｨ鬮ｮ繝ｻ+ preset diff 雎育｢托ｽｼ繝ｻ竏ｪ邵ｺ・ｧ隰暦ｽ･驍ｯ螢ｽ・ｸ蛹ｻ竏ｩ
- 雎包ｽｨ隲｢繝ｻ
  - 雋ょｸｷ・ｮ諤懃ｲ玖ｬ悟・謔ｽ闖ｴ髮｣・ｼ驛・ｽ､繝ｻ蟆・ｸｺ・ｪFilter陞溯歓・ｪ・ｿ繝ｻ蟲ｨ繝ｻ waveform邵ｺ・ｫ陜謎ｹ晢ｽ・恷・ｼ邵ｺ・ｾ邵ｺ・ｪ邵ｺ繝ｻ
### 4.2 Noise

- 陟厄ｽｹ陷托ｽｲ:
  - 郢晏ｼｱ縺・ｹｧ・ｺ騾墓ｻ薙・
- 隰暦ｽ･驍ｯ螢ｽ閠ｳ陞ゑｽｨ:
  - 陷茨ｽｱ鬨ｾ蜩･ilter郢ｧ雋槭・陋ｻ・ｩ騾包ｽｨ邵ｺ蜉ｱ窶ｻ鬮ｻ・ｳ豼ｶ・ｲ隰御ｻ呻ｽｽ・｢
- Parameter Smoothing 隴・ｽｹ鬩･譎｢・ｼ繝ｻhase E 陋ｻ・､陞ｳ螟ｲ・ｼ繝ｻ
  - 霑ｴ・ｾ隴弱ｉ縺帷ｸｺ・ｯ隴幢ｽｪ隰暦ｽ･驍ｯ螟ｲ・ｼ莠･諢幄ｭ・ｽｭ邵ｺ・ｮ邵ｺ・ｿ驕抵ｽｺ陞ｳ螟ｲ・ｼ繝ｻ  - 騾・・鄂ｰ: 霑ｴ・ｾ霑･・ｶNoise邵ｺ・ｯ陷雁｡・ｴ豕後・陷牙ｸ吶堤ｸｲ竏壺穐邵ｺ螢ｹ繝ｻFilter隰暦ｽ･驍ｯ螢ｹ繝ｻ陷・ｽｪ陷井ｺ･・ｺ・ｦ邵ｺ遒・ｽｫ蛟･・・  - 隹ｺ・｡陋溷揃・｣繝ｻ Filter陝・ｸｻ繝ｻ陟募ｾ後・ `filterCutoff` smoothing
- 雎包ｽｨ隲｢繝ｻ
  - 郢晏ｼｱ縺・ｹｧ・ｺ陜暦ｽｺ隴帷判邏幄厄ｽ｢邵ｺ・ｨ雋ょｸｷ・ｮ諤懊・鬨ｾ螢ｽ・ｩ貅ｯ繝ｻ郢ｧ蜻茨ｽｷ・ｷ陜ｨ・ｨ邵ｺ霈披雷邵ｺ・ｪ邵ｺ繝ｻ
### 4.3 FM

- 陟厄ｽｹ陷托ｽｲ:
  - 郢ｧ・ｪ郢晏｣ｹﾎ樒ｹ晢ｽｼ郢ｧ・ｿ陞溯歓・ｪ・ｿ
- 隰暦ｽ･驍ｯ螢ｽ閠ｳ陞ゑｽｨ:
  - FM陷・ｽｺ陷牙ｸ幢ｽｾ譴ｧ・ｮ・ｵ邵ｺ・ｫ陷茨ｽｱ鬨ｾ蜩･ilter郢ｧ蜻育｣・け螢ｼ蠎・妙・ｽ
- Parameter Smoothing 隴・ｽｹ鬩･譎｢・ｼ繝ｻhase E 陋ｻ・､陞ｳ螟ｲ・ｼ繝ｻ
  - 霑ｴ・ｾ隴弱ｉ縺帷ｸｺ・ｯ隴幢ｽｪ隰暦ｽ･驍ｯ螟ｲ・ｼ莠･諢幄ｭ・ｽｭ邵ｺ・ｮ邵ｺ・ｿ驕抵ｽｺ陞ｳ螟ｲ・ｼ繝ｻ  - 騾・・鄂ｰ: 陷亥現竊擢M陋幢ｽｴ邵ｺ・ｮ隲｡・｡陟托ｽｵ髴・ｽｸ繝ｻ繝ｻindex`/operator髫ｪ・ｭ髫ｪ闌ｨ・ｼ蟲ｨ・定摎・ｺ陞ｳ螢ｹ・邵ｺ・ｦ邵ｺ荵晢ｽ芽ｬ暦ｽ･驍ｯ螢ｹ・邵ｺ貊灘ｩｿ邵ｺ謔溘・髫ｪ・ｭ髫ｪ蛹ｻ・定ｲょｸ呻ｽ臥ｸｺ蟶呻ｽ・  - 隹ｺ・｡陋溷揃・｣繝ｻ `index` smoothing繝ｻ閧ｲ豢定ｭ弱ｇ・ｮ螢ｽ辟壹・蟲ｨ竊・`pitchMul` smoothing
- 雎包ｽｨ隲｢繝ｻ
  - FM郢ｧ・｢郢晢ｽｫ郢ｧ・ｴ郢晢ｽｪ郢ｧ・ｺ郢晢｣ｰ邵ｺ・ｨ雋ょｸｷ・ｮ遉ｼ・ｳ・ｻ郢昜ｻ｣ﾎ帷ｹ晢ｽ｡郢晢ｽｼ郢ｧ・ｿ郢ｧ蜻茨ｽｷ・ｷ陷ｷ蠕鯉ｼ邵ｺ・ｪ邵ｺ繝ｻ
### 4.4 Drum / DrumKit

- 陟厄ｽｹ陷托ｽｲ:
  - one-shot隰・瑳・･・ｽ陜趣ｽｨ
- 隰暦ｽ･驍ｯ螢ｽ閠ｳ陞ゑｽｨ:
  - 陟｢繝ｻ・ｦ竏ｵ諤呵氣蝓主応邵ｺ・ｧ陷茨ｽｱ鬨ｾ蜩･ilter郢ｧ蜻育｣・け繝ｻ- Parameter Smoothing 隴・ｽｹ鬩･譎｢・ｼ繝ｻhase E 陋ｻ・､陞ｳ螟ｲ・ｼ繝ｻ
  - 霑ｴ・ｾ隴弱ｉ縺帷ｸｺ・ｯ隴幢ｽｪ隰暦ｽ･驍ｯ螟ｲ・ｼ莠･諢幄ｭ・ｽｭ邵ｺ・ｮ邵ｺ・ｿ驕抵ｽｺ陞ｳ螟ｲ・ｼ繝ｻ  - 騾・・鄂ｰ: one-shot邵ｺ・ｮ郢ｧ・｢郢ｧ・ｿ郢昴・縺醍ｹｧ蟶昴・郢ｧ蟲ｨ笳狗ｹｧ荵斟懃ｹｧ・ｹ郢ｧ・ｯ邵ｺ遒・ｽｫ蛟･・・  - 隹ｺ・｡陋溷揃・｣繝ｻ 郢晢ｽｪ郢晢ｽｪ郢晢ｽｼ郢ｧ・ｹ陝ｶ・ｯ陜捺ｺ倥・邵ｺ・ｿ鬮ｯ莉呻ｽｮ螟絶・騾包ｽｨ繝ｻ繝ｻrum type陋ｻ・･邵ｺ・ｫ陷ｿ・ｯ陷ｷ・ｦ陋ｻ繝ｻ・ｲ謦ｰ・ｼ繝ｻ- 雎包ｽｨ隲｢繝ｻ
  - 郢ｧ・｢郢ｧ・ｿ郢昴・縺題怏・｣陋ｹ謔ｶ・帝ｩ包ｽｿ邵ｺ莉｣・狗ｸｺ貅假ｽ∫ｸｲ竏ｵ蟀ｿ陟台ｸ樊肩邵ｺ・ｫ鬩包ｽｩ騾包ｽｨ陷ｿ・ｯ陷ｷ・ｦ郢ｧ雋樊・隴・ｽｭ

### 4.5 雋ょｸｷ・ｮ諤懃ｲ玖ｬ梧腸・ｼ蝓滓ざ陞ｳ貅ｯ・｣繝ｻ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - 陷茨ｽｱ鬨ｾ蜩･ilter陜難ｽｺ騾ｶ・､ + 陞溯歓・ｪ・ｿ陜難ｽｺ騾ｶ・､邵ｺ・ｮ隴幢ｽｬ闖ｴ轣伜密
- 隰暦ｽ･驍ｯ螢ｽ蟀ｿ鬩･繝ｻ
  - waveform/noise/fm邵ｺ・ｮ騾具ｽｺ隰厄ｽｯ驍ｨ蜈域｣｡郢ｧ雋槭・陷牙ｸ吮・邵ｺ蜉ｱ窶ｻ陷ｿ蜉ｱ・郢ｧ繝ｻ- 雎包ｽｨ隲｢繝ｻ
  - 騾具ｽｺ隰厄ｽｯ陜趣ｽｨ郢晢ｽｭ郢ｧ・ｸ郢昴・縺醍ｹｧ蜻茨ｽｸ蟶ｷ・ｮ諤懊・邵ｺ・ｫ陷讎奇ｽｮ貅ｯ・｣繝ｻ・邵ｺ・ｪ邵ｺ繝ｻ
### 4.6 陷会｣ｰ驍よ懃ｲ玖ｬ梧腸・ｼ蝓滓ざ陞ｳ貅ｯ・｣繝ｻ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - partial驗抵ｽ､邵ｺ・ｮ陷ｷ蝓溘・
- 隰暦ｽ･驍ｯ螢ｽ蟀ｿ鬩･繝ｻ
  - 隴・ｽｹ陟台ｸ槫ｴ玖ｭ帛ｳｨ繝ｻpartial驍ゑｽ｡騾・・・定楜貅ｯ・｣繝ｻ・邵ｲ竏晢ｽｾ譴ｧ・ｮ・ｵ邵ｺ・ｯ陷茨ｽｱ鬨ｾ蜩ｺix邵ｺ・ｸ隰暦ｽ･驍ｯ繝ｻ- 雎包ｽｨ隲｢繝ｻ
  - waveform邵ｺ・ｮunison/sub-osc郢ｧ蛛ｵ笳守ｸｺ・ｮ邵ｺ・ｾ邵ｺ・ｾ髫阪・・｣・ｽ邵ｺ蜉ｱ竊醍ｸｺ繝ｻ
### 4.7 PSG繝ｻ蝓滓ざ陞ｳ貅ｯ・｣繝ｻ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - 陋ｻ・ｶ驍上・・ｻ蛟･窶ｳ郢昶・繝｣郢晞斡豬ｹ雋・・- 隰暦ｽ･驍ｯ螢ｽ蟀ｿ鬩･繝ｻ
  - PSG陜暦ｽｺ隴帷甥螳幃ｏ繝ｻ・定将譎・亜邵ｺ蜉ｱ笳・ｸｺ・ｾ邵ｺ・ｾ陷茨ｽｱ鬨ｾ蜩ｺix邵ｺ・ｸ隰暦ｽ･驍ｯ繝ｻ- 雎包ｽｨ隲｢繝ｻ
  - 雎主ｮ育舞郢ｧ・ｷ郢晢ｽｳ郢ｧ・ｻ隶匁ｺｯ繝ｻ郢ｧ蟶昶с陟趣ｽｦ邵ｺ・ｫ雎ｺ・ｷ邵ｺ諛岩・邵ｺ繝ｻ
### 4.8 PCM繝ｻ蝓滓ざ陞ｳ貅ｯ・｣繝ｻ・ｼ繝ｻ
- 陟厄ｽｹ陷托ｽｲ:
  - 郢ｧ・ｵ郢晢ｽｳ郢晏干ﾎ晁怙蜥ｲ蜃ｽ
- 隰暦ｽ･驍ｯ螢ｽ蟀ｿ鬩･繝ｻ
  - PCM陷蜥ｲ蜃ｽ郢ｧ・ｨ郢晢ｽｳ郢ｧ・ｸ郢晢ｽｳ郢ｧ蜻亥ｩｿ陟台ｸ槫ｴ玖ｭ帛ｳｨ縲定楜貅ｯ・｣繝ｻ・邵ｲ竏晢ｽｾ譴ｧ・ｮ・ｵ邵ｺ・ｯ陷茨ｽｱ鬨ｾ蜩ｺix邵ｺ・ｸ隰暦ｽ･驍ｯ繝ｻ- 雎包ｽｨ隲｢繝ｻ
  - 雎包ｽ｢陟厄ｽ｢騾具ｽｺ隰厄ｽｯ陜趣ｽｨ陞ｳ貅ｯ・｣繝ｻ繝ｻ郢ｧ・ｳ郢晄鱒繝ｻ郢ｧ蟶昶茜邵ｺ莉｣・・
## 5.1 Parameter Smoothing 隰暦ｽ･驍ｯ螢ｹ繝ｦ郢晢ｽｳ郢晏干ﾎ樒ｹ晢ｽｼ郢晁肩・ｼ莠･・ｰ繝ｻ謫りｭ・ｽｹ陟大・逡代・繝ｻ
- 鬩包ｽｩ騾包ｽｨ陝・ｽｾ髮趣ｽ｡:
  - `amp` / `pitchMul` / `filterCutoffHz` / 繝ｻ莠･・ｿ繝ｻ・ｦ竏壺・郢ｧ繝ｻ`index`繝ｻ繝ｻ- 隴崢陝・ｸ橸ｽｮ貅ｯ・｣繝ｻ
  - `VoicesSoA` 邵ｺ・ｫ `SmoothedParam` state 郢ｧ螳夲ｽｿ・ｽ陷会｣ｰ
  - voice陋ｻ譎・ｄ陋ｹ謔ｶ縲・`range/sampleRate/timeMs` 郢ｧ螳夲ｽｨ・ｭ陞ｳ繝ｻ  - `Renderer` 邵ｺ・ｧ `SetSmoothedTarget -> StepSmoothedParam` 郢ｧ蟶昶・騾包ｽｨ
- 陷茨ｽｬ鬮｢邇厄ｽｨ・ｭ陞ｳ繝ｻ
  - Config郢晢ｽ｢郢昴・ﾎ晉ｸｺ・ｸ `smoothing` 郢ｧ螳夲ｽｿ・ｽ陷会｣ｰ
  - `ConfigLoad/ConfigJsonUtils/GUIChannelEditor/GUIConfigUtils` 郢ｧ雋樣・隴弱ｈ蟲ｩ隴・ｽｰ
- 陷ｿ蜉ｱ・陷茨ｽ･郢ｧ繝ｻ
  - 陷ｷ蠕｡・ｸﾂMIDI邵ｺ・ｮ ON/OFF 雎育｢托ｽｼ繝ｻ縲定淦・ｮ陋ｻ繝ｻ・｢・ｺ髫ｱ繝ｻ  - peak/rms/clip 郢ｧ螳夲ｽｨ蛟ｬ鮖ｸ

## 6. 驕問扱・ｭ・｢闔遏ｩ・ｰ繝ｻ・ｼ逎ｯ纃ｾ髫阪・莠溯ｱ・ｽ｢繝ｻ繝ｻ
- 陷ｷ蠕個ｧ隶匁ｺｯ繝ｻ郢ｧ螳夲ｽ､繝ｻ辟夊ｭ・ｽｹ陟台ｸ岩・陋ｻ・･陞ｳ貅ｯ・｣繝ｻ・邵ｺ・ｪ邵ｺ繝ｻ- 陷茨ｽｱ鬨ｾ螢ｼ蝟ｧ邵ｺ・ｧ邵ｺ髦ｪ・狗ｹｧ繧・・郢ｧ蜻亥ｩｿ陟台ｸ槫ｴ玖ｭ帛ｳｨ縺慕ｹ晢ｽｼ郢晏ｳｨ竏郁怦蛹ｻ竊楢ｭ厄ｽｸ邵ｺ荵昶・邵ｺ繝ｻ- `source.type` 髴托ｽｽ陷会｣ｰ隴弱ｅ竊・`SourceRegistry` 郢ｧ蝣､・ｵ讙守ｽｰ邵ｺ蜉ｱ竊醍ｸｺ繝ｻ・ｮ貅ｯ・｣繝ｻ・堤ｸｺ蜉ｱ竊醍ｸｺ繝ｻ
## 7. 陞ｳ貅ｯ・｣繝ｻ繝｡郢ｧ・ｧ郢昴・縺醍ｹ晢ｽｪ郢ｧ・ｹ郢昴・
- [x] 陟・・髦懆崕・､隴・ｽｭ郢ｧ繝ｻ`method-boundaries.md` 邵ｺ・ｫ霎｣・ｧ郢ｧ蟲ｨ・邵ｺ・ｦ驕抵ｽｺ髫ｱ繝ｻ- [x] `SynthEngine.h` 邵ｺ・ｮConfig郢ｧ蜻亥ｳｩ隴・ｽｰ
- [x] `ConfigLoad` / `ConfigJsonUtils` 郢ｧ蜻亥ｳｩ隴・ｽｰ
- [x] `GUIChannelEditor` / `GUIConfigUtils` 郢ｧ蜻亥ｳｩ隴・ｽｰ
- [x] 郢晢ｽｬ郢晢ｽｳ郢敖驍ｨ迹夲ｽｷ・ｯ郢ｧ蜻育｣・け繝ｻ- [x] AB驕抵ｽｺ髫ｱ謳ｾ・ｼ驛・・ｳ + 陟｢繝ｻ・ｦ竏壺・郢ｧ迚咎・闕ｳﾂMIDI雎育｢托ｽｼ繝ｻ・ｼ繝ｻ- [x] 郢晏ｳｨ縺冗ｹ晢ｽ･郢晢ｽ｡郢晢ｽｳ郢晏沺蟲ｩ隴・ｽｰ

## Auto-Generated

<!-- AUTO-GENERATED:BEGIN -->
### Auto Snapshot

- Generated by `scripts/update_synth_docs.ps1`
- Generated at: 2026-02-26 17:26:14

### Foundation Files

- `include/SynthEngine/SynthEngine.h` (updated: 2026-02-26 17:14:52)
- `include/SynthEngine/Filter.h` (updated: 2026-02-23 19:58:17)
- `include/SynthEngine/Modulation.h` (updated: 2026-02-23 22:11:53)
- `src/SynthEngine/Filter.cpp` (updated: 2026-02-23 19:58:41)
- `src/SynthEngine/Modulation.cpp` (updated: 2026-02-24 16:23:05)
- `src/SynthEngine/Renderer.cpp` (updated: 2026-02-23 22:11:53)
- `src/SynthEngine/Voices.cpp` (updated: 2026-02-23 22:11:53)
- `src/config/ConfigLoad.cpp` (updated: 2026-02-23 22:26:30)
- `src/config/ConfigJsonUtils.cpp` (updated: 2026-02-23 20:43:49)
- `src/gui/GUIChannelEditor.cpp` (updated: 2026-02-26 17:15:13)
- `src/gui/GUIConfigUtils.cpp` (updated: 2026-02-23 20:44:11)
<!-- AUTO-GENERATED:END -->

















































