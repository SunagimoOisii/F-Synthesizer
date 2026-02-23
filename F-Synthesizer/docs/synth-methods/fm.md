# FM（2オペレータ）

最終更新: 2026-02-23
状態: 実装済み
source type: `fm`

## A. 現状の実装内容

- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
- 実行経路:
  - キャリア/モジュレータ位相を更新
  - `index` と `outLevel` を適用して出力
  - ADSR とチャンネルミックスを適用
- 現状仕様:
  - 2オペレータ構成
  - `carrierWave`, `modWave`, `carrierRatio`, `modRatio`, `index`, `outLevel`

## B. 作れる音 / 向いている用途

- 音のキャラクター:
  - 明るい、金属的、パーカッシブ、デジタル感
- 得意用途:
  - ベル
  - EP 系
  - プラック
  - 抜けるリード
- 苦手用途:
  - フィルタ主体の暖かいアナログスイープ

## C. 問題点

- 2オペ限定でアルゴリズム自由度が低い
- パラメータ感度が高く、操作難度が上がりやすい
- Parameter Smoothing は未接続（Phase E 判定）

## D. 改善案

- 短期:
  - 安全域プリセットと初期値整備
- 中期:
  - 4オペの限定トポロジ導入検討
  - `index` / `pitchMul` への smoothing 接続を検討
- 長期:
  - 方式間共通の変調制御（LFO/Env 連携）へ統合

## E. 保守 / 拡張メモ

- 方式拡張時は `SourceRegistry` の kind を起点に反映箇所を管理する
- JSON パラメータ追加時は preset 差分保存との互換を優先する
