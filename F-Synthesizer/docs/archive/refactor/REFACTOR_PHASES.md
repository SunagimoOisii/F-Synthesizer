# REFACTOR_PHASES

このドキュメントは、GUI以外を含む「ファイル分割・フォルダ整理」の段階定義です。

このファイルの扱い:
- リファクタ中: 更新対象
- 完了後: `docs/archive/refactor/REFACTOR_PHASES.md` へ移動して凍結

## 互換性ポリシー（固定）

- 外部I/Fは当面互換維持:
  - CLI引数（`--gui` / `--cli` / `--config` / `--preset`）
  - 設定キー構造（`config/*.json`）
  - 主要ログの意味（`EventStats` / `RenderStats`）
- 互換破壊が必要な場合は、先に `docs/migration_refactor.md` へ差分を記載してから実装する

## 移行順序ルール（固定）

- 1段階で「配置変更」と「ロジック変更」を同時に大きく行わない
- 推奨順序:
  1. ラッパー追加（旧配置から新配置を呼ぶ）
  2. 呼び出し元の切替
  3. 実体移動
  4. 旧経路削除
- 各フェーズは「ビルド可能な中間状態」を維持する

## フェーズ完了時ドキュメント更新ルール（固定）

- 各Phase完了時に以下を更新する:
  - `STATUS.md`: Current Snapshot / Next Actions
  - `Architecture.md`: 構成図・責務説明
  - `REFACTOR_TASKS.md`: チェック状態
- 主要な運用変更があれば `README.md` も同時更新する

## REFACTOR_PHASE_1_STRUCTURE_BASELINE

Status: DONE

- 現在の責務を棚卸しし、移動先ディレクトリを確定
- `src/app` / `src/core` / `src/config` / `src/midi` / `src/io` / `src/gui` の骨格を作成
- 依存方向ルールを明文化（`app -> core`、`gui -> app`、`core` はUI非依存）

完了条件:
- 新ディレクトリ構成が決まっている
- 依存方向ルールが文書化されている

## REFACTOR_PHASE_2_ENTRY_AND_CONFIG_SPLIT

Status: DONE

- `SoundGenerate.cpp` のエントリ責務を分離（CLI引数解釈/設定解決/実行呼び出し）
- 設定読込・保存・プリセット解決を `config` 層へ集約
- 既存CLIオプション互換を維持

完了条件:
- エントリ側に業務ロジックが残っていない
- `--config` / `--preset` の挙動が既存と一致する

## REFACTOR_PHASE_3_MIDI_AND_ENGINE_BOUNDARY

Status: DONE

- MIDI解釈と時間変換を `midi` 層に整理
- SynthEngine 内部依存を段階整理し、境界を明確化
- running status など既知不具合の修正ポイントを局所化

完了条件:
- MIDI関連の入口が `midi` 層に集約されている
- Engine関連の変更影響範囲が縮小している

## REFACTOR_PHASE_4_IO_AND_PLATFORM_HARDENING

Status: DONE

- WAV書き出し、パス正規化、文字コード変換を `io/platform` 層へ整理
- 日本語パスと保存失敗系の共通ハンドリングを統一
- ログ・エラー文言を統一フォーマット化
- 実施内容:
  - `include/io/PlatformPaths.h` / `src/io/PlatformPaths.cpp` を追加し、`PathToUtf8` / `Utf8ToPath` / `ResolvePathFromBase` / 診断整形を共通化
  - `Writer` に `WavWriteError` を導入し、保存失敗時に `code/cause/hint/errno/winerr/path` を返すよう拡張
  - `SoundGenerate` / `ConfigFileIO` / `GUIMain` の文字コード・パス処理を共通関数へ切替
  - `scripts/gui_smoke.ps1` に日本語パスの CLI スモーク（設定読込 + WAV 保存）を追加

完了条件:
- 文字コード/パス処理が分散していない
- 保存失敗時の診断情報が一貫している

## REFACTOR_PHASE_5_GUI_FILE_SPLIT

Status: DONE

- `src/GUIMain.cpp` を責務単位へ分割（state/view/action/preview など）
- GUI 層の入出力境界（設定I/O、実行呼び出し、プレビュー再生）を明確化
- 既存 GUI 操作導線（Play/Preview/Stop/Preset 保存）の互換を維持
- 着手済み:
  - Preview 再生処理を `src/gui/PreviewAudio.cpp` / `include/gui/PreviewAudio.h` へ分離
  - ファイルダイアログ・パスUI補助を `src/gui/GUIPlatform.cpp` / `include/gui/GUIPlatform.h` へ分離
  - 音源変換/比較/プリセットJSON補助を `src/gui/GUIConfigUtils.cpp` / `include/gui/GUIConfigUtils.h` へ分離
  - `gui_state.json` の読込/保存を `src/gui/GUIStateStorage.cpp` / `include/gui/GUIStateStorage.h` へ分離
  - 実行前バリデーション/出力WAVパス生成を `src/gui/GUIRunHelpers.cpp` / `include/gui/GUIRunHelpers.h` へ分離
  - preset差分保存/一覧収集/適用読込を `src/gui/GUIPresetIO.cpp` / `include/gui/GUIPresetIO.h` へ分離
  - `GUIState` 定義と `BuildConfig/EnsureChannel` を `src/gui/GUIStateModel.cpp` / `include/gui/GUIStateModel.h` へ分離
  - state初期化/保存値修復（`Init`/`Repair`）を `src/gui/GUIStateModel.cpp` へ分離
  - チャンネル編集UI（`DrawChannelEditor`）を `src/gui/GUIChannelEditor.cpp` / `include/gui/GUIChannelEditor.h` へ分離
  - 実行制御/プリセット適用/ログ解析を `src/gui/GUIActions.cpp` / `include/gui/GUIActions.h` へ分離
  - `gui_state.json` モデル変換と保存経路を `src/gui/GUIStatePersistence.cpp` / `include/gui/GUIStatePersistence.h` へ分離
  - `GUIMain.cpp` を描画中心に整理し、GUI責務境界（設定I/O・実行・プレビュー）を明確化
  - `scripts/gui_smoke.ps1`（13ステップ）で Play/Preview/Stop/Preset を含む互換スモークを再確認

完了条件:
- GUI 関連変更が局所化され、`GUIMain.cpp` への集中が緩和されている
- GUI の主要スモーク（起動・実行・停止・保存）が退行していない

## REFACTOR_PHASE_6_CONFIG_FILE_SPLIT

Status: DONE

- `src/config/ConfigFileIO.cpp` を読込・検証・保存整形の責務で分割
- JSON 解析補助とドメイン変換処理を分離し、保守点を局所化
- 設定キー互換（`config/*.json`）を維持
- 着手済み:
  - `src/config/ConfigFileIO.cpp` を公開I/Fラッパーへ簡素化
  - `src/config/ConfigJsonUtils.cpp` へ JSON補助/型変換/文字列表現を分離
  - `src/config/ConfigLoad.cpp` へ読込・検証・ドメイン適用を分離
  - `src/config/ConfigSave.cpp` へ保存整形/出力処理を分離
  - `src/config/ConfigFileInternal.h` で内部結合点を明確化
  - `Debug x64` をビルドし、`scripts/gui_smoke.ps1` 13ステップ（config/preset/invalid/japanese path含む）を通過

完了条件:
- Config 変更時の影響範囲が限定され、テスト対象が明確になっている
- `LoadConfigFile/SaveConfigFile` の公開I/F互換が維持される

## REFACTOR_PHASE_7_APP_RUN_FILE_SPLIT

Status: DONE

- `src/SoundGenerate.cpp` を実行フロー/統計ログ/保存制御の責務で分割
- `Run` 系 API の公開I/Fと主要ログ意味（`EventStats`/`RenderStats`）を維持
- app 層から core/midi/io 呼び出し境界を明確化
- 実施内容:
  - `src/app/RunExecution.cpp` に実行フロー本体を分離
  - `src/app/RunStats.cpp` に MIDI/Render 統計ログを分離
  - `src/app/RunSave.cpp` に WAV 保存制御を分離
  - `src/app/RunDefaults.cpp` に default config / project root 解決を分離
  - `src/SoundGenerate.cpp` は `Run` API ラッパー + `main` へ簡素化
  - `Debug x64` ビルドと `scripts/gui_smoke.ps1` 13ステップで互換確認

完了条件:
- 実行コア変更時の差分がレビューしやすい粒度に分割されている
- CLI 実行経路の互換が維持される

## REFACTOR_PHASE_8_TEST_AND_RELEASE

Status: DONE

- スモークテストと回帰テストを新構成に追従
- `README` / `Architecture` / `STATUS` を最終構成へ更新
- 旧配置から新配置への移行ノートを作成
- 実施内容:
  - `scripts/check.ps1` の実行バイナリ解決を `build/<Platform>/<Configuration>` 優先へ更新
  - `scripts/check.ps1` の Auto Snapshot 対象へ `src/app/Run*.cpp` を追加
  - `scripts/gui_smoke.ps1` のステップ表記を 13件に整合、生成した日本語パス成果物の後始末を追加
  - `docs/migration_refactor.md` を作成し、Phase1-8 の移行要点と再開手順を明文化
  - フェーズ履歴のアーカイブ方針を確定（`docs/archive/refactor/` へ移動・凍結）

完了条件:
- 既存機能退行なしでビルド/実行/スモークが通る
- 再開時に迷わないドキュメント状態になっている
