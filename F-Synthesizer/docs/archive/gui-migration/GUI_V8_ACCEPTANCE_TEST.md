# GUI_V8_ACCEPTANCE_TEST

v8 受け入れ手順（手動）  
対象: Sound/Music責務整理、Music編集磨き込み、保存導線、エラー復旧導線

## 前提

- `build/x64/Debug/F-Synthesizer.exe` が存在する
- `assets/midi/solstice_intro.mid` が存在する

## 1. 初回導線（迷わず音出し/書き出し）

1. アプリを起動する
2. `Sound` タブで `Play Tone (C4)` を実行する
3. `Music` タブへ移動して `Export WAV` を実行する

期待結果:
- 起動直後でも `Sound` で音色確認、`Music` で書き出しに迷わず到達できる

## 2. Sound責務の確認（重複機能なし）

1. `Sound` タブを開く
2. 音色編集、`Play Tone`、`Play Preview`、`Stop` を確認する
3. `MIDI Path` / `Output Path` / `Output Target` 系UIが存在しないことを確認する

期待結果:
- `Sound` は音色作成/試聴に集中し、楽曲出力設定を持たない

## 3. Music編集コア（割当/ミックス/出力）

1. `Music` タブで `Music Mixer / Assignment` テーブルを確認する
2. 任意chの `Mute/Solo/Level/Pan/Gain` を変更する
3. `Source` 列で音色割当を変更する
4. `Output Target` を `All Channels` と `Single Channel` で切り替える
5. `Target Ch` を指定して `Export WAV` を実行する

期待結果:
- Musicタブのみで、割当・ミックス・出力対象指定・書き出しが完結する

## 4. ドラムch特別扱い（重複排除後）

1. `Music` タブで `Drum ch10 Special Handling` を有効にする
2. `Auto Setup Drum ch10` を実行する
3. `Focus PR ch10` を実行し、PRチャンネルがch10へ移動することを確認する
4. テーブル `ch9` 行にドラム適合表示（`Drum OK` / 警告）が出ることを確認する

期待結果:
- ch10運用の着手が短手数で行える
- `Solo ch10` / `Export ch10` / `Save SoundAsset` などの重複導線は存在しない

## 5. 保存導線と未保存ガード

1. 値を変更し未保存状態を作る
2. `Save Project` を実行し、MusicProject + Workspace が保存されることを確認する
3. さらに値を変更し、`Close` を押す
4. 未保存警告ダイアログで `キャンセル` を押し、アプリ継続を確認する
5. 再度 `Close` を押し、`保存せず続行` で終了できることを確認する
6. もう一度起動し、変更後にプリセット切替して同警告が出ることを確認する

期待結果:
- 未保存終了/切替で警告が必ず出る
- `Save Project` と `Save All` の対象境界を誤認しない

## 6. エラー復旧導線（Problem/Suggested Fix/Recover）

1. `Music` タブで `MIDI Path` を不正にして `Export WAV` を実行する
2. エラーダイアログ表示を確認する
3. 画面内エラーに `Problem:` と `Suggested Fix:` と `Recover:*` ボタンが出ることを確認する
4. `Recover: Browse MIDI` で修正し、エラーが解除されることを確認する
5. `Output Path` を空にして同様に `Recover: Browse Output` を確認する
6. `Clear Error` で画面内エラーを消せることを確認する

期待結果:
- エラー時に次操作が明確で、復旧まで一貫して誘導される

## 7. 共通回帰観点

1. `Stop` が再生中・レンダ中の両方で有効
2. タブ切替時に再生が停止
3. `Play Preview` が毎回再生成経路で実行される
4. 再生中に `Export WAV` を押すと再生停止後に書き出し開始

期待結果:
- v7で固定したランタイム契約が維持される

## 判定

- PASS: 上記 1-7 の期待結果を満たす
- CONDITIONAL PASS: 重大でない文言差分のみ
- FAIL: 音が出ない、書き出せない、復旧不能エラー、責務重複の再発
