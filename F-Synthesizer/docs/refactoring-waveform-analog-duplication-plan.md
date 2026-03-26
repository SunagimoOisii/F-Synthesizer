# Waveform/Analog 重複リファクタリング案（分析結果ベース）

最終更新: 2026-03-26

## 方針（前提）

- `source.type` としての `waveform` / `analog` 分離は維持する（編集意図の分離は設計上の意図）。
- 分離を維持したまま、機械的重複（同一処理の二重実装）をテンプレート/共通ヘルパーで削減する。
- 仕様互換を最優先し、音挙動・JSONキー/順序・既存プリセット互換を壊さない。

## 優先度A（効果大・リスク中）

1. GUI Waveform/Analog 編集ブロックの共通化
- 対象:
  - `src/gui/GUIChannelEditor.cpp`
- 現状:
  - clamp、Filter UI、Arpeggio UI、Smoothing UI、Modulation UI がほぼ二重実装。
- 案:
  - `DrawWaveformLikeEditorCommon<T>(...)` を導入し、共通UIを集約。
  - `analog` 固有は `driftDepthCents` / `driftRateHz` のみ後段で追加描画。
  - `waveform_modulation` / `analog_modulation` の ID 接尾辞は引数化して保持。
- 期待効果:
  - 新パラメータ追加時の GUI 片側更新漏れを防止。

2. Renderer の Waveform/Analog 共通レンダリングを集約
- 対象:
  - `src/SynthEngine/Renderer.cpp`
- 現状:
  - `RenderWaveformSource` と `RenderAnalogSource` は drift 適用以外ほぼ同一。
- 案:
  - `RenderWaveformLikeSourceCommon<TSource, TVoiceState>(...)` を導入。
  - 前段フックで `analog` のみ drift を適用し、以降（unison/sub/ring/filter/hardSync）は共通実行。
- 期待効果:
  - 音響挙動の差分管理を drift 固有に限定できる。

3. Voices 初期化の Waveform/Analog 共通化
- 対象:
  - `src/SynthEngine/Voices.cpp`
- 現状:
  - smoothing/filter/modulation 初期化が二重。
- 案:
  - `InitWaveformLikeVoiceStateCommon<TSource, TVoiceState>(...)` を導入。
  - `analog` 固有の `driftPhase/driftPhaseOffset` のみ個別処理。
- 期待効果:
  - 初期化仕様の不整合防止。

## 優先度B（効果中・リスク低）

4. smoothing parse/validate の重複削減
- 対象:
  - `src/config/load/LoadModulation.cpp`
- 現状:
  - `ParseWaveformSmoothingObject` と `ValidateWaveformSmoothing` が waveform/analog で二重。
- 案:
  - 共通テンプレート化 + `contextPrefix` 引数化でエラーメッセージの差分を保持。

5. schema 検証ループの共通化
- 対象:
  - `src/config/load/LoadSource.cpp`
- 現状:
  - `Validate*BySchema` が同型ループを複数実装。
- 案:
  - `ValidateBySchemaCommon(kind, prefix, schemaValueFn, optionalRuleFn, ...)` を導入。
  - `Drum` の optionalWhenNonPositive はフック関数で吸収。

6. GUI equals の Waveform/Analog 共通比較
- 対象:
  - `src/gui/GUIConfigUtils.cpp`
- 現状:
  - 共通フィールド比較式が長く二重。
- 案:
  - `WaveformLikeConfigEqualsCommon(a, b)` を導入。
  - `analog` 側で drift 2 項目だけ追加比較。

## 非推奨（やらない）

- `waveform` と `analog` を単一 `source.type` に統合する変更。
  - 理由: 設計上「編集意図の分離」が明示されているため、運用方針と衝突する。

## 実施順（推奨）

1. `Load/Save` 系（完了済み）
2. `LoadModulation.cpp`（低リスク）
3. `GUIConfigUtils.cpp`（低リスク）
4. `Voices.cpp`（中リスク）
5. `Renderer.cpp`（中〜高リスク）
6. `GUIChannelEditor.cpp`（中リスク、表示回帰確認を厚めに）

## 受け入れ基準（各タスク共通）

- `./scripts/check.ps1` が通過すること。
- JSON load/save のキー名・順序・インデントに差分が出ないこと（意図した差分がないこと）。
- 既存プリセットで load → save → load の等価性が維持されること。
- waveform/analog の共通項目に追加変更した際、片側だけ修正が必要な構造になっていないこと。
