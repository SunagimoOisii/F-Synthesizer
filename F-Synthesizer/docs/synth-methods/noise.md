# ノイズ（Noise）

最終更新: 2026-02-23
状態: 実装済み
source type: `noise`

## A. 現状の実装内容

- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
  - `src/Oscillator.cpp`
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
- Parameter Smoothing は未接続（Phase E 判定）

## D. 改善案

- 短期:
  - ノイズ色・簡易成形プリセットの拡充
- 中期:
  - 減算合成の共通フィルタへ接続
  - Filter導入後に `filterCutoff` smoothing 接続を検討
- 長期:
  - ノイズ成形モジュールの独立化（tone/shape/tilt）

## E. 保守 / 拡張メモ

- ノイズ固有パラメータを増やす場合は、`source.type=noise` の後方互換を維持する
- GUI と Config の保存形式は共通 writer を使って同期させる
