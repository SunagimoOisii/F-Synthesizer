# ノイズ（Noise）

最終更新: 2026-03-19
状態: 実装済み
source type: `noise`

## A. 現状の実装内容

- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
  - `src/synth/Oscillator.cpp`
- 実行経路:
  - ノイズサンプル生成
  - ADSR とチャンネルミックスを適用
- 対応ノイズ:
  - `white`
  - `pink`
  - `brown`
  - `blue`

## B. 作れる音 / 向いている用途

- 音のキャラクター:
  - 無音高のテクスチャ、アタック補強
- 得意用途:
  - スネア/ハット層
  - 効果音
  - 空気感付与
- 苦手用途:
  - 音程を持つメロディ

## C. 問題点

- source レベルでの成形手段がまだ限定的
- Parameter Smoothing は非適用（契約上 waveform 専用）

## D. 改善案

- 短期:
  - ノイズ色・簡易成形プリセットの拡充
- 中期:
  - 減算合成の共通フィルタへ接続
  - Filter導入後に `filterCutoff` smoothing 接続を再検討（下記条件を満たす場合のみ）
- 長期:
  - ノイズ成形モジュールの独立化（tone/shape/tilt）

## F. Smoothing 契約整合（2026-03-19）

- 適用状況:
  - `未適用`
- 非適用理由:
  - 現行 noise source は `noiseType` のみで、連続変化させる source パラメータが実質ない。
  - smoothing 対象が未定義のまま導入すると GUI/保存形式と齟齬が出る。
- 再検討条件:
  - noise 側に連続パラメータ（例: filterCutoff/tone/tilt）が追加され、schema と GUI が定義済みであること。
  - 代表MIDIで AB（peak/rms/clip + 聴感）を通過すること。

## E. 保守 / 拡張メモ

- ノイズ固有パラメータを増やす場合は、`source.type=noise` の後方互換を維持する
- GUI と Config の保存形式は共通 writer を使って同期させる

