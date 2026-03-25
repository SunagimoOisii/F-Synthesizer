# FM（4オペレータ）

最終更新: 2026-03-25
状態: 実装済み
source type: `fm`

## A. 現状の実装内容

- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/SynthEngine/Renderer.cpp`
- 実行経路:
  - 4オペレータ位相を更新
  - `algorithm` に応じて接続トポロジを切り替えて合成
  - `feedback`（ops[0]自己帰還）を適用
  - ADSR とチャンネルミックスを適用
- 現状仕様:
  - 4オペレータ構成（`ops[4]`）
  - `algorithm`（0-3）
  - `feedback`（0.0-1.0）
  - `ops[i].wave`, `ops[i].ratio`, `ops[i].level`, `ops[i].index`

## B. 作れる音 / 向いている用途

- 音のキャラクター:
  - 明るい、金属的、パーカッシブ、デジタル感
- 得意用途:
  - ブラス（Mega Drive 系）
  - 80年代 FM 弦
  - オルガン（1変調3キャリア）
  - ディープベース（チェーンFM）
  - ベル
  - EP 系
  - プラック
  - 抜けるリード
- 苦手用途:
  - フィルタ主体の暖かいアナログスイープ

## C. 問題点

- パラメータ感度が高く、操作難度が上がりやすい
- Parameter Smoothing は非適用（契約上 waveform 専用）

## D. 改善案

- 短期:
  - 安全域プリセットと初期値整備
- 中期:
  - アルゴリズム別テンプレートの整理
  - `index` / `pitchMul` への smoothing 接続を再検討（下記条件を満たす場合のみ）
- 長期:
  - 方式間共通の変調制御（LFO/Env 連携）へ統合

## F. Smoothing 契約整合（2026-03-19）

- 適用状況:
  - `未適用`
- 非適用理由:
  - FMは`index`/`pitchMul`変化が倍音構造へ直接影響し、無条件 smoothing で音色意図が崩れやすい。
  - 現行Config/GUIは fm 用 smoothing パラメータを持たず、保存形式との整合を優先する。
- 再検討条件:
  - 対象を `fm.index` などに限定し、時定数上限を定義できること。
  - 代表MIDIで AB（peak/rms/clip + 聴感）を通過し、既存プリセット破綻がないこと。

## E. 保守 / 拡張メモ

- 方式拡張時は `SourceRegistry` の kind を起点に反映箇所を管理する
- JSON パラメータ追加時は preset 差分保存との互換を優先する
