# PIANO_ROLL_PHASES

このドキュメントは、GUIへのピアノロール導入を段階的に進めるためのフェーズ定義です。

## 決定事項（2026-02-21）

- 時間単位: `tick保持 + 描画時sample換算`
- 編集対象: `Noteのみ`（MVP）
- 同時ノート: `重複許可`
- スナップ: `切替式 + OFF`
- チャンネル表示: `単一ch表示`
- Drum表示: `切替可能`（MVPは鍵盤、次段でドラム名）
- Preview反映: `編集バッファ優先`
- Preview開始位置: `ピアノロール上でダブルクリックした地点（tick）を起点にする`
- 保存形式: `専用project JSON`
- Undo/Redo単位: `1ドラッグ = 1コマンド`
- 非目標（MVP外）: `CCレーン`, `クオンタイズ詳細`, `複数ノート一括編集`
- 受け入れテスト: `手動 + スモーク`（段階的に自動化）

## 運用前提（固定）

- 依存方向は `gui -> app -> core` を維持する
- 公開I/F（`Run` / `AppConfig`）の互換性を優先する
- まずは MVP（表示 + 基本編集 + Preview反映）を完了させる
- コメント規約は `docs/COMMENT_GUIDELINE.md` に従う
- プロダクト方針は `docs/PRODUCT_POLICY.md` を優先する

## 優先軸（PRODUCT_POLICY連動）

優先順位:
1. 音の気持ちよさに直結する体験（編集 -> 即時試聴 -> 納得）
2. 完全初心者でも迷わない操作（目的語ベースのUI文言、単純な導線）
3. 反復試行の速さ（再生遅延/描画遅延の低減）
4. 高度機能よりも安定した基本操作（非目標: 高度DAW機能）

この優先軸により、Phase5以降の追加は「深い機能」より「気持ちよく試せる体験」を先に実装する。

## PIANO_ROLL_PHASE_1_MODEL_AND_VIEW_BASE

Status: DONE (2026-02-21)

- `PianoRollState`（ズーム、スクロール、選択状態）を導入
- ノート表示用の中間モデル（tick/sample -> GUI座標）を導入
- ImGui上でグリッド、鍵盤、ノート矩形の描画を実装（表示のみ）
- 成果物: `include/gui/GUIPianoRoll.h` / `src/gui/GUIPianoRoll.cpp` と `GUIMain` への統合
- 非対象: Run/Preview連動（Phase 3で実装）

完了条件:
- MIDIイベントがGUI上で可視化できる
- チャンネル切替とズーム/スクロールが動作する

## PIANO_ROLL_PHASE_2_SELECTION_AND_EDIT

Status: IN PROGRESS (2026-02-21)

- ノート選択（単体/範囲）を実装
- ノート移動（tick, note）を実装
- ノート長変更（resize）を実装
- 変更を編集バッファへ反映（元データは破壊しない）
- 非対象: Preview/Exportへの反映（Phase 3で実装）

完了条件:
- 選択/移動/長さ変更がUI操作で完結する
- 編集結果がピアノロール表示に即時反映される

## PIANO_ROLL_PHASE_3_RUN_INTEGRATION

Status: DONE (2026-02-21)

- 編集バッファを既存Run経路に接続
- Preview/Exportで編集後イベントを使う経路を追加
- 既存の設定読込/保存フローと競合しないように統合

完了条件:
- Previewで編集結果の音が反映される
- Exportでも編集結果が反映される
- Previewは `編集バッファ優先` で反映される

## PIANO_ROLL_PHASE_4_PERSISTENCE_AND_SAFETY

Status: DONE (2026-02-21)

- 表示状態（ズーム、スクロール、選択）を `gui_state` に保存/復元
- 編集データ本体は `専用project JSON` として保存/復元
- Undo/Redoの最小実装（移動/長さ変更）を導入

完了条件:
- GUI再起動後に表示状態が復元される
- 最低限の編集取り消しができる
- Undo/Redoは `1ドラッグ = 1コマンド` で記録される

## PIANO_ROLL_PHASE_5_POLISH_AND_PERF

Status: IN PROGRESS (2026-02-21)

- 可視範囲のみ描画する最適化
- ノート矩形キャッシュ導入（ズーム変更時のみ再計算）
- 操作性調整（スナップ、ショートカット、再生ヘッド表示）
- ピアノロール上のダブルクリック地点をPreview開始位置に反映
- `gui_smoke` 互換と基本回帰確認

完了条件:
- 大きいMIDIでも操作遅延が目立たない
- 既存GUI機能と干渉せず運用できる
- ダブルクリック起点のPreview開始が意図した位置から再生される
