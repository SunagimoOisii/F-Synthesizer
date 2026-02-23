# REFACTOR_TASKS

このドキュメントは `REFACTOR_PHASES.md` の実装チェックリストです。

このファイルの扱い:
- リファクタ中: 更新対象
- 完了後: `docs/archive/refactor/REFACTOR_TASKS.md` へ移動して凍結

## Global Rules（全Phase共通）

- [x] 互換性ポリシーを確認（CLI引数/設定キー/主要ログ）
- [x] 移行順序ルールに従う（ラッパー追加 -> 切替 -> 実体移動 -> 旧経路削除）
- [x] 各Phase完了時に `STATUS.md` / `Architecture.md` / `REFACTOR_TASKS.md` を更新
- [x] 主要運用変更がある場合は `README.md` も更新

## Phase 1: Structure Baseline

- [x] 現行ソースの責務マップを作成（app/core/config/midi/io/gui）
- [x] 新フォルダ構成を作成（空ディレクトリ + 最小README）
- [x] 依存方向ルールを `Architecture.md` に追記
- [x] 移行時の命名規約を確定（`PascalCase`/`snake_case` 等）
- [x] 移行順序に沿った「移動順リスト」を作成

完了条件:
- [x] どのファイルをどこへ移すか一覧化されている
- [x] レビュー時に依存違反が判定できる

## Phase 2: Entry and Config Split

- [x] `main` 近辺から CLI解析を `app` 層へ切り出し
- [x] 設定解決ロジックを `config` 層へ移動
- [x] `LoadConfigFile/SaveConfigFile` の呼び出し経路を一本化
- [x] `--config/--preset/--gui/--cli` の互換確認
- [x] 旧経路はラッパー経由にしてから段階削除

完了条件:
- [x] `SoundGenerate.cpp` の責務が薄くなっている
- [x] 既存実行オプションの回帰がない

## Phase 3: MIDI and Engine Boundary

- [x] `MIDIParser` / `Sequencer` を `src/midi` へ再配置
- [x] `MIDIParser` / `Sequencer` の呼び出し境界を `midi` パイプラインへ集約
- [x] Engine呼び出し境界に `core/RenderGateway` を導入
- [x] running status関連の修正ポイントをテスト化
- [x] イベント処理の回帰ケースを追加
- [x] `Internal` 依存の整理順序を定義して段階移行

完了条件:
- [x] MIDI変更時にGUI/IOへの影響が最小化される
- [x] 既知不具合の再発防止ケースが追加される

## Phase 4: IO and Platform Hardening

- [x] WAV保存とパス処理の共通ユーティリティを作成
- [x] UTF-8/UTF-16変換の利用箇所を共通化
- [x] エラーメッセージにパス・原因・復旧ヒントを含める
- [x] 日本語パス環境で保存・読込スモークを追加
- [x] 文字コード/パス処理の直呼び箇所を段階的に置換

完了条件:
- [x] 文字化け/保存失敗の原因切り分けが即座にできる
- [x] 同種の実装修正が単一箇所で済む

## Phase 5: GUI File Split

- [x] `src/GUIMain.cpp` を責務別に分割（state/view/action/preview など）
- [x] Preview 再生処理を `src/gui/PreviewAudio.cpp` へ分離
- [x] ファイルダイアログ・パスUI補助を `src/gui/GUIPlatform.cpp` へ分離
- [x] 音源変換/比較/プリセットJSON補助を `src/gui/GUIConfigUtils.cpp` へ分離
- [x] `gui_state.json` 読込/保存を `src/gui/GUIStateStorage.cpp` へ分離
- [x] 実行前バリデーション/出力WAVパス生成を `src/gui/GUIRunHelpers.cpp` へ分離
- [x] preset差分保存/一覧収集/適用読込を `src/gui/GUIPresetIO.cpp` へ分離
- [x] `GUIState` と `BuildConfig/EnsureChannel` を `src/gui/GUIStateModel.cpp` へ分離
- [x] state初期化/保存値修復（`Init`/`Repair`）を `src/gui/GUIStateModel.cpp` へ分離
- [x] チャンネル編集UI（`DrawChannelEditor`）を `src/gui/GUIChannelEditor.cpp` へ分離
- [x] GUI の設定I/O・実行・プレビュー再生境界を整理
- [x] 既存 GUI 操作導線（Play/Preview/Stop/Preset）の互換確認
- [x] 分割の段階移行（ラッパー -> 切替 -> 実体移動 -> 旧経路削除）を記録

完了条件:
- [x] GUI 関連変更の影響範囲が局所化されている
- [x] GUI 主要スモークが通る

## Phase 6: Config File Split

- [x] `src/config/ConfigFileIO.cpp` を読込・検証・保存整形で分割
- [x] JSON 補助処理とドメイン変換を分離
- [x] `LoadConfigFile/SaveConfigFile` 公開I/F互換を維持
- [x] 設定キー互換（`config/*.json`）の回帰確認

完了条件:
- [x] Config 変更の影響範囲が限定されている
- [x] Config サンプル群で回帰がない

## Phase 7: App Run File Split

- [x] `src/SoundGenerate.cpp` を実行フロー/統計/保存制御で分割
- [x] `Run` 系 API と主要ログ意味（`EventStats`/`RenderStats`）を維持
- [x] app 層から core/midi/io 呼び出し境界を整理

完了条件:
- [x] 実行経路の変更差分がレビューしやすい粒度になっている
- [x] CLI 実行互換が維持される

## Phase 8: Test and Release

- [x] `scripts/check.ps1` を新構成へ追従
- [x] `scripts/gui_smoke.ps1` を新パス構成へ追従
- [x] `README.md` / `Architecture.md` / `STATUS.md` を更新
- [x] `docs/migration_refactor.md` を作成
- [x] フェーズ履歴を `docs/archive/refactor/` へ整理する方針を確定

完了条件:
- [x] `Debug x64` ビルド成功
- [x] CLI/GUIの主要スモークが通る
- [x] ドキュメントだけで再開できる
