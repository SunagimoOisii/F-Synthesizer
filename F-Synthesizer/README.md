# F-Synthesizer

MIDI ファイルを読み込み、サンプル単位で合成して WAV に書き出す C++ シンセサイザです。

## 1分で再開

1. `docs/STATUS.md` を開いて「Current Snapshot」「Next Actions」を確認
2. `config/default.json` と `config/presets/` の実行設定を確認
3. Visual Studio で `F-Synthesizer.vcxproj` を開いて `x64 Debug` でビルド・実行

## 現在の主な機能

- MIDI 解析: Note / Tempo / Control Change / Pitch Bend
- 時間変換: tick -> sample へ変換
- 合成: Voice 管理、ADSR、チャンネル設定、イベント適用
- 音源: Waveform / Noise / FM / Drum / DrumKit
- 出力: WAV 保存
- GUI v0.5: Top Bar（Status バッジ + UI Scale + 実行アクション）を導入
- GUI v0.5: 3領域レイアウト（Top/Body/Bottom）と下段固定ログ
- GUI v0.5: チャンネルUX再構築（Mix Summary + Selected Channel 詳細）
- GUI v0.5: GUI内プレビュー再生（Play Preview統合操作 / Loop / Stop）
- GUI v0.5: プリセット運用（Save Preset As / Duplicate / Reset Channel / dirty表示）
- GUI v0.5: 実行中Stopのレンダキャンセル、SoAレンダ内部実装

## 構成

- `assets/midi/`: 入力MIDIファイル置き場
- `src/SoundGenerate.cpp`: 実行入口（`Run` APIラッパー + `main`）
- `src/app/RunExecution.cpp`: Run実行フロー本体
- `src/app/RunStats.cpp`: `EventStats` / `RenderStats` ログ整形
- `src/app/RunSave.cpp`: WAV保存制御
- `src/app/RunDefaults.cpp`: default config / project root 解決
- `src/midi/MIDIParser.cpp`: MIDI パース
- `src/midi/Sequencer.cpp`: tick/sample 変換
- `src/SynthEngine/`: 合成エンジン本体
- `src/io/Writer.cpp`: WAV 書き出し
- `src/config/load/`: Config load 分割実装（top-level/channel/source/modulation）
- `src/gui/main/`: GUIメイン画面の分割実装（TopBar/MainWindow/RunLoop）
- `src/gui/pianoroll/`: ピアノロール分割実装（Tempo/Edit/Render/Input）
- `include/`: 公開ヘッダ
- `docs/Architecture.md`: 設計メモ
- `docs/PRODUCT_POLICY.md`: プロダクト方針（価値/対象ユーザー/非目標）
- `docs/GUI_REQUIREMENTS.md`: GUI導入時の最小要件と実装契約
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES.md`: GUI移行の段階定義
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS.md`: GUI移行の実装タスク
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v3.md`: GUI v0.3 段階定義（試聴/ミックス）
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v3.md`: GUI v0.3 実装タスク
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v4.md`: GUI v0.4 段階定義（GUI内Preview再生）
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v4.md`: GUI v0.4 実装タスク
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v5.md`: GUI v0.5 段階定義（UI再構築）
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v5.md`: GUI v0.5 実装タスク
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v6.md`: GUI v0.6 段階定義（Sound/Music モード分離、凍結）
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v6.md`: GUI v0.6 実装タスク（凍結）
- `docs/archive/migration/migration_v4.md`: v0.3 -> v0.4 の移行手順（凍結）
- `docs/archive/migration/migration_v5.md`: v0.4 -> v0.5 の移行手順（凍結）
- `docs/archive/migration/migration_refactor.md`: Refactor Phase 1-8 の移行ノート（凍結）
- `docs/COMMENT_GUIDELINE.md`: コメント運用規約
- `docs/archive/piano-roll-migration/PIANO_ROLL_PHASES.md`: ピアノロール導入フェーズ定義（凍結）
- `docs/archive/piano-roll-migration/PIANO_ROLL_TASKS.md`: ピアノロール導入タスク（凍結）
- `docs/archive/piano-roll-migration/PIANO_ROLL_DIRECT_INTERACTION_PHASES.md`: ピアノロール直接操作の導入フェーズ（凍結）
- `docs/archive/piano-roll-migration/PIANO_ROLL_DIRECT_INTERACTION_TASKS.md`: ピアノロール直接操作の導入タスク（凍結）
- `docs/archive/piano-roll-migration/PIANO_ROLL_ACCEPTANCE_TEST.md`: ピアノロール受け入れ手順（凍結）
- `docs/PIANO_ROLL_CONTROLS.md`: ピアノロール操作方法リファレンス
- `docs/archive/comment-migration/COMMENT_MIGRATION_PHASES.md`: コメント移行フェーズ定義（凍結）
- `docs/archive/comment-migration/COMMENT_MIGRATION_TASKS.md`: コメント移行タスクチェックリスト（凍結）
- `docs/archive/comment-migration/COMMENT_MIGRATION_BASELINE.md`: コメント移行の対象一覧/レビュー基準（凍結）
- `docs/ROADMAP.md`: 直近優先事項と将来検討
- `docs/DECISIONS.md`: 設計判断ログ
- `docs/WEEKLY_MAINTENANCE.md`: 週次運用手順
- `docs/STATUS.md`: 現在の進捗と次アクション

## ビルドと実行

### Visual Studio (推奨)

1. `F-Synthesizer.vcxproj` を開く
2. 構成を `Debug | x64` に設定
3. ビルドして実行

### 依存ライブラリ（vcpkg）

GUI移行で利用する `Dear ImGui` / `GLFW` は `vcpkg` で導入します。

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
& C:\vcpkg\bootstrap-vcpkg.bat
& C:\vcpkg\vcpkg.exe install glfw3:x64-windows imgui[glfw-binding,opengl3-binding,docking-experimental]:x64-windows
& C:\vcpkg\vcpkg.exe integrate install
```

補足:
- `integrate install` 済みなら、MSBuild (`.vcxproj`) から include/link が自動連携されます
- `VCPKG_ROOT` が別値を指している場合は `C:\vcpkg` に合わせて更新してください

### CLI（設定ファイル運用）

#### 使い方

1. `Debug|x64` をビルド
2. 実行（既定はGUI起動）
3. 必要なら `--config` または `--preset` を指定

```powershell
# build
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ..\F-Synthesizer.sln /t:Build /p:Configuration=Debug /p:Platform=x64

# run (default: GUI)
.\build\x64\Debug\F-Synthesizer.exe

# run (CLI mode, auto: config/default.json)
.\build\x64\Debug\F-Synthesizer.exe --cli

# run (GUI explicit)
.\build\x64\Debug\F-Synthesizer.exe --gui

# run (explicit config)
.\build\x64\Debug\F-Synthesizer.exe --config .\config\default.json

# run (preset: base + presets/<name>.json)
.\build\x64\Debug\F-Synthesizer.exe --preset basic_wave
.\build\x64\Debug\F-Synthesizer.exe --preset fm_default

# help
.\build\x64\Debug\F-Synthesizer.exe --help
```

### GUI運用メモ

- 既定起動は GUI（`F-Synthesizer.exe`）
- `Run` 前に入力バリデーションを実施:
  - MIDI path 空欄/ファイル不在
  - Output path 空欄
  - `sampleRate <= 0`
  - `initialSeconds <= 0`
  - `bits != 16`
- バリデーション失敗時はログパネルに理由を表示し、実行は行わない
- GUI状態は `config/gui_state.json` に保存/復元される

### GUI v0.5 操作（Top Bar）

1. `Status` で現在状態（Idle/Running/Preview/Canceled/Failed）を確認
2. `UI Scale` で表示倍率（100/125/150%）を切替
3. `Play` / `Play Preview (Selected ch)` / `Stop` を上段から実行
4. `Play Preview (Selected ch)` は、プレビュー済みバッファがあれば再生、`Ctrl+クリック` で再レンダ
5. `Loop Preview` でプレビュー再生のループを切替

### GUI v0.5 操作（チャンネル編集 / ミックス）

1. `Mix Summary (M/S/L)` で 16ch の Mute/Solo/Level を俯瞰
2. `Edit` で編集対象チャンネルを選択
3. `Mix Details` で `Mute/Solo/Level/Pan/Gain` を調整
4. `Envelope / Gain` で `amp/attack/decay/sustain/release` を調整
5. `Source Details` で `waveform/noise/fm/drum/drumkit` と固有パラメータを調整
6. 選択チャンネルを `Play Preview (Selected ch)` で即試聴

### GUI v0.5 操作（プリセット運用）

1. `Preset Name` に保存名を入力
2. `Save Preset As` で `config/presets/<name>.json` へ保存
3. `Duplicate Preset` で `*_copy` を作成
4. `Reset Channel` で現在チャンネルを既定値へ戻す
5. 変更未保存時は `Preset: modified (unsaved)` を表示

#### 設定ファイル（`config/default.json`）

主なキー:

- `midiPath`: 入力MIDIパス（相対パスは設定ファイル基準）
- `wavPath`: 出力WAVパス（相対パスは設定ファイル基準）
- `targetChannel`: 対象チャンネル（`-1` は全チャンネル）
- `defaultWave`: `sine` / `square` / `saw` / `triangle`
- `initialSeconds`: 初期バッファ秒数
- `bits`: 出力ビット深度（現状は 16bit 運用）
- `sampleRate`: サンプルレート
- `extraReleaseSec`: 末尾の余長（秒）

#### プリセット運用

- `config/base.json`: 共通設定
- `config/presets/basic_wave.json`: 基本波形プリセット（推奨デフォルト）
- `config/presets/fm_default.json`: FMプリセット
- 適用順は `base.json` -> `presets/<name>.json`（後勝ち）
- `--config` 指定時は `--preset` より `--config` を優先

## Unified Check Command

PowerShell から以下を実行すると、差分確認・md更新ルール確認・ビルド・実行をまとめて行えます。

```powershell
.\scripts\check.ps1
```

よく使うオプション:

- `.\scripts\check.ps1 -SkipRun` : ビルドまで
- `.\scripts\check.ps1 -SkipBuild -SkipRun` : md更新ルール確認のみ
- `.\scripts\check.ps1 -AllowDocMismatch` : md不足を警告のみで継続

## Git Hook (自動ドキュメント更新)

初回のみ:

```powershell
.\scripts\install_git_hooks.ps1
```

有効化後は `git commit` 時に以下が自動更新され、コミットへ自動追加されます。

- `docs/Architecture.md`（Auto-Generated ブロック）
- `docs/synth-methods/integration-playbook.md`（Auto-Generated ブロック）
- `docs/architecture/*.md` の `Special Notes`（関連カテゴリの雛形）

対象カテゴリ（変更ファイルに応じて雛形追記）:
- GUI操作・状態管理
- 実行フロー/キャンセル
- MIDI時間変換
- Config互換性
- 音響アルゴリズム上の制約
- 依存方向・責務境界

## GUI Minimal Test

```powershell
.\scripts\gui_smoke.ps1
```

このスクリプトは以下を順に確認します。
- `--help` 表示
- `--cli --config config/default.json` 実行
- `--cli --config config/samples/channel_minimal.json` 実行
- `--cli --config config/samples/channel_full.json` 実行
- `--cli --config config/samples/mix_all_mute.json` 実行（`nonZero=0` 確認）
- `--cli --preset basic_wave` 実行（`nonZero>0` 確認）
- `--cli --preset fm_default` 実行（`nonZero>0` 確認）
- `--cli --config config/samples/channel_mix_invalid.json` 失敗確認
- `--cli --config config/__missing__.json` 失敗確認
- `--cli --config config/samples/channel_invalid.json` 失敗確認
- 既定起動（GUI）スモークテスト
- `--gui` 明示起動スモークテスト

## GUI v8 Acceptance (Manual)

手動受け入れ手順:
- `docs/GUI_V8_ACCEPTANCE_TEST.md`

## ドキュメント運用ルール

### 常設ドキュメント（継続更新）

- `README.md`: 実行手順と導入手順
- `docs/STATUS.md`: 現在状態と次アクション
- `docs/Architecture.md`: 実装アーキテクチャ
- `docs/PRODUCT_POLICY.md`: プロダクト方針（価値/対象ユーザー/非目標）
- `docs/GUI_REQUIREMENTS.md`: GUI実装契約（機能追加時に更新）
- `docs/COMMENT_GUIDELINE.md`: コメント運用規約（`src+include`）
- `docs/archive/comment-migration/COMMENT_MIGRATION_PHASES.md`: コメント移行のフェーズ管理（凍結）
- `docs/archive/comment-migration/COMMENT_MIGRATION_TASKS.md`: コメント移行の進捗チェック（凍結）
- `docs/archive/comment-migration/COMMENT_MIGRATION_BASELINE.md`: コメント移行の優先順位/判定基準（凍結）

### 移行ドキュメント（移行期間のみアクティブ）

- `docs/archive/gui-migration/GUI_MIGRATION_PHASES.md`
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS.md`
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v3.md`
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v3.md`
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v4.md`
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v4.md`
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v5.md`
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v5.md`
- `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v6.md`
- `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v6.md`
- `docs/archive/piano-roll-migration/PIANO_ROLL_PHASES.md`
- `docs/archive/piano-roll-migration/PIANO_ROLL_TASKS.md`
- `docs/archive/piano-roll-migration/PIANO_ROLL_DIRECT_INTERACTION_PHASES.md`
- `docs/archive/piano-roll-migration/PIANO_ROLL_DIRECT_INTERACTION_TASKS.md`
- `docs/archive/piano-roll-migration/PIANO_ROLL_ACCEPTANCE_TEST.md`

扱い:
- GUI移行中は更新対象
- GUI移行完了後は `docs/archive/gui-migration/` へ移動してアーカイブ
- `README.md` には「移行履歴へのリンク」のみ残す

## 再開時チェックリスト

- `git status --short --branch` で作業ツリーを確認
- `git log --oneline --decorate -n 10` で直近変更を確認
- `docs/STATUS.md` の日付・優先タスクを更新
