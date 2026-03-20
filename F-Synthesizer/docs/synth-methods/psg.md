# PSG

最終更新: 2026-03-20
状態: 実装済み（基礎版）
source type: `psg`

## A. 現状の実装内容（source type 名・主要ファイル・パラメータ一覧）

- source type:
  - `psg`
- 主なファイル:
  - `include/SynthEngine/SynthEngine.h`
  - `src/config/SourceRegistry.cpp`
  - `src/config/load/LoadSource.cpp`
  - `src/SynthEngine/Renderer.cpp`
  - `src/SynthEngine/Voices.cpp`
  - `src/gui/GUIChannelEditor.cpp`
- 実行経路:
  - Configで `source.type=psg` を解決し、`wave/duty/volumeSteps/maxVoices` をロード
  - Voice初期化で PSG 専用状態（phase/LFSR）を割当
  - Rendererで `square/pulse/triangle/noise` を生成
  - ADSR、チャンネルミックスを適用
- パラメータ一覧:
  - `wave` (`square/pulse/triangle/noise`)
  - `duty` (`0..7`, `wave=pulse` 時に有効)
  - `volumeSteps` (`0..15`)
  - `maxVoices` (`1..8`)
  - チャンネル共通パラメータ: `amp/attackSec/decaySec/sustainLevel/releaseSec`

## B. 作れる音 / 向いている用途

- 音のキャラクター:
  - 立ち上がりが明確で、輪郭がはっきりしたチップチューン系
  - `square/pulse` は鋭く、`triangle` は丸い低域、`noise` はザラついた成分
- 得意用途:
  - 8bit系リード
  - シンプルなベース
  - 効果音（短いビープ、ノイズ系）
- 苦手用途:
  - 滑らかな倍音推移や高解像度な音色変化
  - 厚いユニゾンや複雑なモジュレーション主体の音

## C. 問題点

- `method-boundaries.md` では PSG が未実装表記のままで、現状実装との差分がある
- 現状は最小実装で、PSG固有の拡張（例: ハードウェア寄りの詳細制約）は未導入
- `volumeSteps` は離散振幅再現だが、実機差分（チップ差や非線形特性）は未モデル化
- modulation/filter/smoothing は非対応で、音作りの可動域は waveform/fm より狭い

## D. 改善案

- 短期:
  - PSG用プリセットを追加し、`square/triangle/pulse/noise` の使い分け基準を固定化
  - `method-boundaries.md` の PSG 状態を現実装へ合わせて更新
- 中期:
  - PSG方式向けのAB確認テンプレート（代表MIDI + 聴感チェック項目）を整備
  - `maxVoices` 運用ルール（lead/bass/sfx の推奨値）を docs に明記
- 長期:
  - 必要に応じて、実機挙動寄りの制約（チャンネル役割差、疑似ミキサ特性）を検討

## E. Smoothing 契約整合（未適用・理由）

- 適用状況:
  - `未適用`
- 非適用理由:
  - 基盤契約（`docs/synth-methods/foundation-contract.md`）で `source.smoothing` は `waveform` のみ受理
  - PSG は離散制約（`duty`/`volumeSteps`）を前提にしており、無条件 smoothing で方式の個性が崩れやすい
  - 現行の Config/GUI 保存形式に PSG smoothing パラメータが存在しない
- 再検討条件:
  - 対象パラメータを限定し、時定数上限を定義できること
  - 代表MIDIの AB（peak/rms/clip + 聴感）で既存プリセット破綻がないこと
