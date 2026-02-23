# PIANO_ROLL_TASKS

このドキュメントは `docs/PIANO_ROLL_PHASES.md` の実装チェックリストです。

## Global Rules（全Phase共通）

- [ ] 依存方向 `gui -> app -> core` を維持している
- [ ] 公開I/F（`Run` / `AppConfig`）の互換を壊していない
- [ ] 既存GUI機能（Preset/Preview/Save）を破壊していない
- [ ] コメントは `docs/COMMENT_GUIDELINE.md` に準拠している
- [ ] 各Phase完了時に `docs/STATUS.md` / `docs/Architecture.md` を更新している
- [ ] `docs/PRODUCT_POLICY.md` の優先軸（音の気持ちよさ/即時フィードバック）に整合している
- [ ] 時間単位は `tick保持 + 描画時sample換算` を維持している
- [ ] Preview反映は `編集バッファ優先` を維持している
- [ ] Preview開始位置は `ピアノロールのダブルクリック地点` を基準にできる

## Phase 1: Model and View Base

- [x] `PianoRollState` を定義
- [x] ノート表示中間モデルを定義
- [x] グリッド描画を実装
- [x] 鍵盤描画を実装
- [x] ノート矩形描画を実装
- [x] チャンネル切替UIを実装
- [x] ズーム/スクロールを実装
- [x] 単一ch表示を維持（複数ch重ね表示は入れない）
- [x] スナップ切替UI（`OFF/1-4/1-8/1-16/1-32` 等）を実装
- [x] 非対象を固定: Preview/Export連動は実装しない（Phase 3で実装）

完了条件:
- [x] MIDIイベントの可視化が成立
- [x] 表示操作（ズーム/スクロール）が成立
- [x] 同時ノート重複を保持したまま描画できる

## Phase 2: Selection and Edit

- [x] 単体選択を実装
- [x] 範囲選択を実装
- [x] ドラッグ移動を実装
- [x] リサイズを実装
- [x] 編集結果をバッファへ反映
- [x] Noteのみ編集対象に限定（CC/PitchBendは対象外）
- [x] 非対象を維持: Preview/Export連動は実装しない（Phase 3で実装）

完了条件:
- [x] 選択/移動/リサイズが一連で動作
- [x] 編集結果が即時表示反映
- [x] 同pitch重複ノートを破壊せず編集できる

## Phase 3: Run Integration

- [x] 編集バッファ -> 実行イベント変換を実装
- [x] Preview/Export連動をこのPhaseで初回実装する
- [x] Preview経路へ接続
- [x] Export経路へ接続
- [x] 既存config/preset経路との整合確認
- [x] 編集バッファ優先の反映順を固定

完了条件:
- [x] Previewに編集結果が反映
- [x] Exportに編集結果が反映

## Phase 4: Persistence and Safety

- [x] 表示状態の保存/復元を実装
- [x] 編集データの保存単位を実装（専用project JSON）
- [x] Undoを実装
- [x] Redoを実装

完了条件:
- [x] 表示状態が再起動後に復元
- [x] 基本編集の取り消し/やり直しが可能
- [x] Undo/Redoが `1ドラッグ=1コマンド` で動作

## Phase 5: Polish and Perf

- [x] 可視範囲のみ描画を実装
- [x] ノート矩形キャッシュを実装
- [x] スナップ操作を実装
- [x] ショートカットを実装
- [x] 再生ヘッド表示を実装
- [x] ピアノロール上のダブルクリック地点をPreview開始位置に反映
- [x] `scripts/gui_smoke.ps1` 互換確認

完了条件:
- [x] 大規模MIDIで操作遅延が許容範囲
- [x] 既存GUI機能との干渉なし
- [x] ダブルクリック起点のPreview再生開始が手動確認できる

## 受け入れテスト方針

- [x] 手動テスト手順を文書化
- [x] 既存 `scripts/gui_smoke.ps1` が通る
- [x] ピアノロール最小スモーク（表示/選択/移動/Preview反映）を追加
