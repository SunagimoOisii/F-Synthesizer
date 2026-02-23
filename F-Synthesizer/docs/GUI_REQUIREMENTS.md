# GUI_REQUIREMENTS

このドキュメントは、現行GUIの実装契約を定義する。  
過去要件・移行途中の仕様は `docs/archive/gui-requirements/GUI_REQUIREMENTS_HISTORY.md` を参照する。
プロダクト全体の価値判断は `docs/PRODUCT_POLICY.md` を優先する。

## 目的

- CLI互換を維持したまま、GUIで編集・試聴・書き出しを完結させる
- 実行コアは `Run(config, options, observer)` を再利用し、GUIは表示と操作に専念する

## 現行UI方針

- 画面は `Sound` / `Music` の2モードに分離する
- `Sound`: 音色作成と音色試聴を主目的とする
- `Music`: MIDI編集、演奏調整、楽曲試聴、書き出しを主目的とする
- ボタン名は目的を直接表す（例: `Export WAV`, `Play Preview`, `Stop`）

## 実装優先順（PRODUCT_POLICY連動）

1. 音の気持ちよさに直結する導線（編集 -> 試聴 -> 納得）
2. 即時フィードバック（操作直後に音/表示が変わる）
3. 初心者が迷わない文言とレイアウト（専門語より目的語）
4. 反復試行の速さ（待ち時間・操作回数の削減）
5. 高度機能は後回し（非目標: DAW深機能/プロ向け深パラメータ）

## 操作契約

### Sound

- `Export WAV`: 常にフル実行してWAVを生成する
- `Play Preview`: 常に再生成して最新編集を試聴する
- `Tone Preview` は MIDI 非依存で現在音色を確認する
- 音色編集操作は試聴導線（`Play Preview` または `Tone Preview`）へ最短で到達できること
- 編集中にプレビュー再生中であっても再生は継続する（自動停止しない）

### Music

- ピアノロール編集結果は `Play Preview` / `Export WAV` の双方へ反映する（編集バッファ優先）
- `Music` 側で MIDI チャンネル演奏調整（Mute/Solo/Level/Pan/Gain）を行えること
- 音色割当（MIDI ch -> SoundAsset）を確認・編集できること
- `link/snapshot` の参照方式は、通常UIで `snapshot` を推奨表示し、`link` は詳細設定として扱うこと
- `PR Channel` は表示/編集専用とし、書き出し対象の決定に連動させない
- 書き出し対象は `All Channels` / `Single Channel` を明示選択できること
- ドラム運用（MIDI ch10想定）向けに、`Auto Setup` / `Focus PR ch10` の短縮導線を提供すること

### 共通

- `Stop` はレンダ中・再生中の両方で有効
- 再生状態は `Idle -> Running -> Done/Failed` を基本とする
- プレビューは常に再生成し、キャッシュ再生を前提にしない
- ループ対象は全体/範囲の両方を許可し、範囲指定有効時は範囲を優先する
- タブ切替時は再生を停止する（編集状態は保持し、暗黙リセットしない）
- 再生中に `Export WAV` を実行した場合、再生を停止して書き出しを開始する
- 保存対象（`SoundAsset` / `MusicProject` / `Workspace`）をUIで明示する
- 保存導線は `Save Project`（MusicProject + Workspace） / `Save All`（全保存）を明示する

## 保存・遷移ガード

- 未保存状態で `終了` または `プロジェクト切替` を行う場合、警告ダイアログを表示する
- 警告ダイアログは `保存して続行` / `保存せず続行` / `キャンセル` を提示する

## エラー通知契約

- エラー時は推奨構成（ログ + 画面内エラー表示 + 修正アクション + ダイアログ通知）を採用する
- MIDI不正など再生不能エラーはダイアログで即時通知する
- ユーザーが次の操作を選べる修正導線（例: パス再選択、設定の見直し）を提供する
- 画面内エラーは `Problem`（原因）+ `Suggested Fix`（次アクション）+ `Recover:*`（実行ボタン）で統一する

## 永続化

- GUI状態は `config/gui_state.json` に保存/復元する
- ピアノロール編集データは `config/piano_roll_project.json` に保存/復元する

## 非機能要件

- Windows + Visual Studio ビルドで動作する
- 既存CLIと同じ設定入力で同等の出力を生成できる
- 実行失敗時は原因をログで確認できる

## 非目標（現時点）

- 高度なDAW機能（CCレーン編集、詳細クオンタイズ、複数ノート一括編集）
- GUI側での合成ロジック実装（Parser/Sequencer/SynthEngine/WriterをGUIへ持ち込まない）

## 受け入れ運用（v8）

- 自動回帰は `scripts/gui_smoke.ps1` を利用する
- 手動受け入れは `docs/GUI_V8_ACCEPTANCE_TEST.md` を利用する
- v7手順（`docs/GUI_V7_ACCEPTANCE_TEST.md`）は凍結記録として保持する
