# PIANO_ROLL_ACCEPTANCE_TEST

このドキュメントは、ピアノロール実装（Phase1-5）に対する受け入れテスト手順です。  
テスト結果は `PASS/FAIL` と簡単なメモを残してください。

## 前提

- `Debug x64` でビルド済み
- 実行ファイル: `build/x64/Debug/F-Synthesizer.exe`
- テストMIDI: `assets/midi/solstice_intro.mid`

## 0. 既存GUIスモーク

1. `powershell -ExecutionPolicy Bypass -File scripts/gui_smoke.ps1 -Configuration Debug -Platform x64` を実行
2. 13ステップがすべて通ることを確認

期待結果:
- CLI成功/失敗の期待が一致する
- GUI起動スモーク（通常起動と `--gui`）が通る

## 1. ピアノロール表示

1. GUI起動
2. MIDIを `assets/midi/solstice_intro.mid` に設定
3. ピアノロールにノートが表示されることを確認
4. `PR Channel`, `PR Zoom`, `PR Tick Offset`, `PR Note Offset` を操作

期待結果:
- 表示が破綻しない
- 操作に応じて表示が更新される

## 2. 選択と編集

1. ノートをクリックして単体選択
2. 空白ドラッグで範囲選択
3. ドラッグで移動
4. ノート右端ドラッグでリサイズ

期待結果:
- 単体/範囲選択が可能
- 移動/リサイズ結果が即時反映される

## 3. Undo/Redo

1. ノートを移動またはリサイズ
2. `Undo` ボタンまたは `Ctrl+Z`
3. `Redo` ボタンまたは `Ctrl+Y`

期待結果:
- `1ドラッグ = 1コマンド` 単位で取り消し/やり直しされる

## 4. Preview反映

1. ノートを移動またはリサイズ
2. `Play Preview (Selected ch)` を実行
3. `Play`（Export）を実行

期待結果:
- Preview/Exportともに編集結果が反映される

## 5. ダブルクリック起点と再生追従

1. グリッド上をダブルクリックして開始位置を設定
2. `Play Preview (Selected ch)` を実行
3. 再生ヘッド表示を確認
4. `PR Follow` ON/OFF で追従挙動を確認

期待結果:
- ダブルクリック地点からPreview開始される
- 再生ヘッドが表示される
- `PR Follow` ONで追従、OFFで固定表示

## 6. Stop挙動

1. Preview再生中に `Stop`
2. しばらく待機

期待結果:
- 再生が停止し、勝手に再開しない

## 7. 永続化

1. ピアノロール表示状態を変更してGUI終了
2. GUI再起動
3. 表示状態（ズーム/オフセット/Follow/開始tick）が復元されることを確認
4. 編集ノートが復元されることを確認

期待結果:
- `config/gui_state.json` と `config/piano_roll_project.json` から復元される

