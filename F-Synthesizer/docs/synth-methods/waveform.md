# 基本波形（Waveform）

最終更新: 2026-02-23
状態: 実装済み
source type: `waveform`

## A. 現状の実装内容

- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
  - `src/synth/Oscillator.cpp`
  - `src/gui/GUIChannelEditor.cpp`
- 実行経路:
  - ボイスごとに位相を進めて波形サンプルを生成
  - ADSR とチャンネルミックスを適用
- 対応波形:
  - `sine`
  - `square`
  - `saw`
  - `triangle`
- 波形品質:
  - `saw/square` は polyBLEP ベースで band-limited を常時有効
- 追加パラメータ:
  - `unisonVoices` (`1..8`)
  - `unisonDetuneCents` (`0..120`)
  - `unisonSpread` (`0..1`)
  - `subOscLevel` (`0..2`)
  - `filterMode` (`bypass/lowpass/highpass/bandpass`)
  - `filterCutoffHz` (`10..20000`)
  - `filterResonance` (`0.1..18`)
  - `modulation.lfo1` (`wave/rateHz/depth/bipolar`)
  - `modulation.env2` (`attack/decay/sustain/release`)
  - `modulation.routes[0..7]` (`source/destination/amount/enabled`)
- GUI対応:
  - Source Details から `wave/unison/sub-osc/filter/modulation` を直接編集可能
  - GUI編集値とJSON編集値で一致レンダを確認済み
  - modulation route は GUI で `0..7` を編集可能

## B. 作れる音 / 向いている用途

- 音のキャラクター:
  - シンプル音色から厚みのある音まで幅広く作成可能
- 得意用途:
  - リード（bright/soft の作り分け）
  - ベース（solid/wide の作り分け）
  - テスト基準音
- 苦手用途:
  - 生楽器系のリアル音
  - 複雑に時間変化する音色

## C. 問題点

- `unisonDetuneCents` を上げすぎるとベースの音程感が崩れやすい
- `subOscLevel` を上げすぎると低域が膨らみ過ぎる
- modulation の source/destination は現状 `LFO1/Env2 -> Pitch/Amp/FilterCutoff` の最小セット

## D. 改善案

- 中期:
  - modulation ルーティング候補（例: resonance, pan）の拡張判断
- 長期:
  - waveform で担う範囲を維持しつつ、減算合成本体へ段階移行

## D.1 推奨レンジ（Phase B）

- bass:
  - `unisonDetuneCents`: `2.0..7.0`
  - `subOscLevel`: `0.6..1.0`
  - `filterCutoffHz`: `2200..3800`
  - `filterResonance`: `0.8..1.2`
  - `smoothing.filterCutoffTimeMs`: `10..18`
- lead:
  - `unisonDetuneCents`: `3.0..9.0`
  - `subOscLevel`: `0.0..0.25`
  - `filterCutoffHz`: `4500..7000`
  - `filterResonance`: `0.8..1.4`
  - `smoothing.pitchTimeMs`: `1.5..3.0`

## E. 保守 / 拡張メモ

- 方式境界は `docs/synth-methods/method-boundaries.md` の `Waveform` 節を正とする
- 方式追加時は `source.type` の管理を `SourceRegistry` で一元化する
- JSON 出力は `config::WriteSourceJSON` を利用して重複実装を避ける
- waveform 拡張時は `ConfigLoad/ConfigJSONUtils/GUIConfigUtils` の3点セット更新を前提にする

## F. AB確認（Modulation ON/OFF）

- 実施日: 2026-02-23
- 入力MIDI: `assets/midi/test.mid`（`targetChannel=0`）
- 比較結果（RenderStats + WAV解析）:
  - normal: ON/OFF で差分あり（`diff_rms=0.005726`, `clips=0`）
  - extreme: ON/OFF で差分あり（`diff_rms=0.139520`, `clips=0`）
- 判定:
  - modulation ON/OFF 比較で音声差分を確認
  - ピーク上昇ケースでも今回条件ではクリップ未発生

## G. AB確認（Smoothing ON/OFF）

- 実施日: 2026-02-23
- 入力MIDI: `assets/midi/smoothing_phaseF_ab.mid`（Phase F用）
- 比較設定:
  - `config/samples/phaseF_smoothing_off.json`
  - `config/samples/phaseF_smoothing_on.json`
- 結果:
  - OFF: `peak=0.549608`, `rms=0.098088`, `clips=0`
  - ON: `peak=0.421827`, `rms=0.093258`, `clips=0`
  - 差分: `diff_rms=0.027361`

## H. Phase B結果（Presetブラッシュアップ）

- 更新プリセット:
  - `config/presets/wave_bass_solid.json`
  - `config/presets/wave_bass_wide.json`
  - `config/presets/wave_lead_bright.json`
  - `config/presets/wave_lead_soft.json`
- AB比較（before/after, 同一MIDI: `assets/midi/smoothing_phaseF_ab.mid`）:
  - bass: `diff_rms=0.011260`, clip `0 -> 0`
  - lead: `diff_rms=0.026905`, clip `0 -> 0`

## I. Wave/Drum Brushup Phase D

- 統合AB設定:
  - `config/samples/phaseD_wave_drum_integrated_before.json`
  - `config/samples/phaseD_wave_drum_integrated_after.json`
- 検証MIDI:
  - `assets/midi/wave_drum_phaseC_ab.mid`
- 結果（同一MIDI, waveform+drum 同居）:
  - before: peak `0.277023`, rms `0.044319`, clip `0`
  - after: peak `0.175695`, rms `0.0313929`, clip `0`
- 調整ポイント:
  - bassの `subOscLevel/filterCutoffHz` を抑えて kick と低域分離
  - modulation amount を軽減してミックス時の揺れを抑制
  - channel mix の level/pan/gain を固定してヘッドルームを確保

## J. 運用固定（Phase E）

- AB手順と記録テンプレートを以下へ固定:
  - `docs-archive/synth-migration/WAVE_DRUM_AB_RUNBOOK.md`


