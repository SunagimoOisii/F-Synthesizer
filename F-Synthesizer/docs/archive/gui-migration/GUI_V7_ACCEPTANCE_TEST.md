# GUI_V7_ACCEPTANCE_TEST

v7 受け入れ手順（手動）  
対象: `Sound/Music` 責務分離、Music編集コア、保存ガード、エラーUX
このファイルは 2026-02-23 時点で凍結（v7基準の受け入れ記録）。

## 前提

- `build/x64/Debug/F-Synthesizer.exe` が存在する
- `assets/midi/solstice_intro.mid` が存在する

## 1. 初回導線（音出し/書き出し）

1. アプリを起動する
2. `Sound` タブで `Play Tone (C4)` を実行し、即時に音が出る
3. `Music` タブへ移動し `Export WAV` を実行できる

期待結果:
- 起動直後に音出し・書き出しへ迷わず到達できる

## 2. Music編集コア

1. `Music` タブで `Music Mixer / Assignment` テーブルを確認する
2. 任意チャンネルの `Mute/Solo/Level/Pan/Gain` を変更する
3. `assign` を変更し、割当先チャンネルが変更できることを確認する
4. `Output Target` を `All Channels` と `Single Channel` で切り替える
5. `Target Ch` を指定し `Export WAV` を実行する

期待結果:
- Musicタブのみで演奏調整、割当、出力対象指定が完結する

## 3. ドラムch特別扱い

1. `Music` タブで `Drum ch10 Special Handling` を有効にする
2. `Auto Setup Drum ch10` を押す
3. テーブルの `ch9` 行に `Drum OK` / `Not Drum` 表示が出ることを確認する

期待結果:
- ch10想定運用のガード表示とセットアップ導線が機能する

## 4. 未保存ガード

1. 値を変更して `Preset: modified (unsaved)` を表示させる
2. `Close` を押す
3. `変更が未保存です。どうしますか？` ダイアログで `キャンセル` を押す
4. 再度 `Close` を押し、`保存せず続行` で終了する

追加確認（プリセット切替）:
1. 起動後に値を変更する
2. `Preset` を別項目へ切り替える
3. 同ダイアログが表示される

期待結果:
- 未保存終了・未保存切替で警告が必ず出る

## 5. エラーUX

1. `Music` タブで `MIDI Path` を不正値にして `Export WAV`
2. エラーダイアログ表示を確認
3. 上部の `Error:` バナー表示と `Fix: Browse MIDI` を確認
4. 有効なMIDIを再選択しエラーを解除する
5. `Output Path` を空にして `Export WAV`
6. `Fix: Browse Output` が表示されることを確認

期待結果:
- ログ + 画面内エラー + 修正アクション + ダイアログの導線が成立する

## 6. 回帰観点

1. `Stop` が再生中・レンダ中の両方で有効
2. タブ切替時に再生が停止
3. `Play Preview` が毎回再生成経路で実行される

期待結果:
- v7以前の主要再生/停止/書き出し導線が壊れていない

## 判定

- PASS: 上記 1-6 の期待結果を満たす
- CONDITIONAL PASS: 重大でないUI文言の差異のみ
- FAIL: 導線断絶（音が出ない、書き出せない、復旧不能エラー）
