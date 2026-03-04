# GUI導入要件（最小）

## 目的

- 既存CLI運用を維持しつつ、設定編集と実行操作をGUI化する
- 合成コアは `Run(config)` を再利用し、表示層のみ追加する

## UI方針（採用済み）

1. 速度重視: ワンクリック生成型
2. 試行錯誤: パラメータ微調整中心
3. 可視化: 詳細イベント/デバッグ表示
4. 制作ワークフロー: 単一曲ループ制作型
5. 学習/検証: エンジン挙動理解重視
6. 個性デザイン: ミニマル実務ツール風
7. 拡張性: 機能追加しやすい構造優先

この方針に基づき、初期GUIは「1画面完結 + 詳細ログパネル + 最小操作」を優先する。

## UIモード分離方針（確定）

- 画面は `Design` / `Preview` の2タブで運用する
- `Design` は値編集・音作りを主目的にする
- `Preview` は確認再生・比較試聴を主目的にする
- 目的の異なる操作を同一タブへ混在させない

### Design タブ

- 編集対象: パラメータ値、ピアノロール編集
- 実行操作:
  - `Export WAV`: 常にフル実行してWAV出力
  - `Play Preview`: 常に再生成して試聴（最新編集を反映）
- 期待挙動: 編集直後の状態が確実に音へ反映される

### Preview タブ

- 編集対象: なし（確認用）
- 実行操作:
  - `Replay` 相当のキャッシュ再生を許可
  - 必要時のみ再生成を明示操作に分離
- 期待挙動: 比較/確認の反復操作を高速化する

### 遷移ルール

- タブ遷移で内部状態を暗黙リセットしない
- 再生中のタブ遷移は許可するが、状態表示（Running/Preview）は維持する
- ボタン文言は目的を直接表す（例: `Export WAV`, `Play Preview`, `Replay`）

## 実装決定事項（確定）

1. GUI技術スタック
- Dear ImGui + GLFW + OpenGL3

2. 実行方式
- バックグラウンド実行（UI非ブロック）

3. ログ連携方式
- `Run` にログコールバックを追加

4. 設定保存方針
- `config/gui_state.json` に前回値を保存

5. エラー表示ルール
- 画面下ログ + ステータスバー表示

6. 出力運用ルール
- 既定は上書き、トグルで連番保存

7. 実行中UI制御
- 実行中は設定入力をロック、停止ボタンのみ有効

8. v0.1 スコープ
- MIDI選択 / preset選択 / 出力先 / Run / ログ表示

9. プリセット編集範囲
- GUIでは選択のみ、編集はJSONで実施

10. 検証ルール
- 実行ごとに `EventStats/RenderStats` を表示

## 最低限の機能要件

1. MIDIファイル選択
2. プリセット選択（`frog` / `solstice`）
3. 出力先WAVパス指定
4. 実行ボタン（合成開始）
5. ログ表示（標準出力を表示）
6. 実行中の状態表示（Running / Done / Failed）

## 初期画面構成（Dear ImGui想定）

- 上段: 入力/出力設定（MIDI, Preset, Output, Run）
- 中段: 微調整パネル（sampleRate, extraReleaseSec, defaultWave など）
- 下段: 詳細ログ/イベント統計（`[EventStats]`, `[RenderStats]` を含む）

操作原則:
- Runボタンは常に視認しやすい位置に固定
- 直近設定は次回起動時に復元（単一曲ループ向け）
- 失敗時はポップアップよりログ詳細表示を優先

## 非機能要件（最小）

- Windows環境で動作（Visual Studioビルド資産を利用）
- 既存CLIと同じ設定入力で同等WAVを生成できる
- 実行失敗時に原因をUI上で確認できる

## 操作フロー

1. ユーザーが MIDI / Preset / Output を選択
2. GUIが `AppConfig` を構築
3. GUIが `Run(config)` を呼び出し
4. 結果（成功/失敗）とログを表示

## Run(config) インターフェース契約

- 入力: `AppConfig`
- 出力: `int`（`0` = 成功, 非`0` = 失敗）
- 副作用:
  - `config.wavPath` へWAVを書き出す
  - 進行ログを標準出力へ出す
- エラー時:
  - 戻り値を非0にし、ログに失敗理由を出す

GUI移行時の拡張契約（確定）:

- 署名案: `int Run(const AppConfig& config, IRunObserver* observer = nullptr)`
- `observer` でログ/進捗/完了通知を受け取る
- `observer == nullptr` の場合は従来CLI互換で標準出力へ出す

## GUI側の実装ルール

- 合成ロジック（Parser/Sequencer/SynthEngine/Writer）に依存しない
- `main()` 相当の設定解決ロジックはGUI側で実装してよい
- ただし実行本体は必ず `Run(config)` を呼ぶ

## 実装固定ルール（開発前確定）

1. 画面レイアウト
- 上段: MIDI / Preset / Output / Run / Stop
- 中段: 最小パラメータ（`sampleRate`, `extraReleaseSec`, `defaultWave`, `overwrite/serial`）
- 下段: ログ + `EventStats/RenderStats`

2. 状態遷移
- `Idle -> Running -> Done/Failed`
- `Running` 中は入力ロック、`Stop` のみ有効

3. 設定読み込み優先順
- 起動時: `base -> preset -> gui_state`
- GUIで明示指定した設定値は最優先

4. 出力ルール
- 既定: 上書き
- `Serial Save` ON: `name_YYYYMMDD_HHMMSS.wav`

5. ログ粒度
- 常時表示: 開始/入力出力/完了/失敗理由
- 詳細表示: `EventStats/RenderStats`
- デバッグ詳細は折りたたみ表示
