# migration_refactor

最終更新: 2026-02-21

## 目的

本ドキュメントは、Refactor Phase 1-8 で行った再配置を、再開者が最短で把握するための移行ノートです。
外部I/F（CLI引数、設定キー、主要ログ）は互換維持されています。

## 互換維持事項

- CLI: `--gui` / `--cli` / `--config` / `--preset`
- 設定キー: `config/*.json`
- 主要ログ意味: `EventStats` / `RenderStats`

## 主要な再配置

- 実行入口
  - 旧: `src/SoundGenerate.cpp` に実行ロジック集中
  - 新: `src/SoundGenerate.cpp` は `Run` APIラッパー + `main`
  - 新規分割: `src/app/RunExecution.cpp` / `src/app/RunStats.cpp` / `src/app/RunSave.cpp` / `src/app/RunDefaults.cpp`

- Config I/O
  - 旧: `src/config/ConfigFileIO.cpp` に読込/検証/保存が集中
  - 新: `src/config/ConfigFileIO.cpp` は公開I/Fラッパー
  - 新規分割: `src/config/ConfigLoad.cpp` / `src/config/ConfigJsonUtils.cpp` / `src/config/ConfigSave.cpp`

- GUI
  - 旧: `src/GUIMain.cpp` に状態/操作/再生/保存が集中
  - 新: GUI責務を `src/gui/*` へ分割（Actions/StateModel/StatePersistence/PreviewAudio/PresetIO 等）

- IO/Platform
  - 新規: `src/io/PlatformPaths.cpp` で UTF/Path/診断整形を共通化
  - 拡張: `Writer` に `WavWriteError` を導入

## 実行確認コマンド

```powershell
# ビルド
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ..\F-Synthesizer.sln /t:Build /p:Configuration=Debug /p:Platform=x64

# 統合チェック（doc rules + auto architecture）
.\scripts\check.ps1 -SkipBuild -SkipRun

# GUI/CLIスモーク
.\scripts\gui_smoke.ps1
```

## アーカイブ方針（確定）

Refactor完了後の履歴管理は以下に統一します。

1. `REFACTOR_PHASES.md` と `REFACTOR_TASKS.md` を `docs/archive/refactor/` へ移動して凍結
2. ルートには最小限（`README.md`）のみ残し、運用中ドキュメントは `docs/` に集約する
3. `README.md` にアーカイブ先リンクを残す
4. 新規リファクタ企画は別名ドキュメントで開始し、旧版は更新しない

## 再開時の最短手順

1. `docs/STATUS.md` の `Current Snapshot` と `Next Actions` を確認
2. `docs/Architecture.md` の `実装構成(現状)` を確認
3. `scripts/check.ps1 -SkipBuild -SkipRun` を実行して差分ルールを確認
4. 変更前に `scripts/gui_smoke.ps1` を実行して基準結果を確保
