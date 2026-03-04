# OPERATIONS

ビルド、実行、チェック、GUIスモーク、運用ルールの詳細手順。
日常の入口は `../README.md` を参照。

## Visual Studio Build

1. `F-Synthesizer.vcxproj` を開く
2. 構成を `Debug | x64` に設定
3. ビルドして実行

## Dependency Setup (vcpkg)

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
& C:\vcpkg\bootstrap-vcpkg.bat
& C:\vcpkg\vcpkg.exe install glfw3:x64-windows imgui[glfw-binding,opengl3-binding,docking-experimental]:x64-windows
& C:\vcpkg\vcpkg.exe integrate install
```

## CLI / GUI Run Examples

```powershell
# build
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ..\F-Synthesizer.sln /t:Build /p:Configuration=Debug /p:Platform=x64

# run (default: GUI)
.\build\x64\Debug\F-Synthesizer.exe

# run (CLI mode)
.\build\x64\Debug\F-Synthesizer.exe --cli

# run (GUI explicit)
.\build\x64\Debug\F-Synthesizer.exe --gui

# run (explicit config)
.\build\x64\Debug\F-Synthesizer.exe --config .\config\default.json

# run (preset)
.\build\x64\Debug\F-Synthesizer.exe --preset basic_wave
.\build\x64\Debug\F-Synthesizer.exe --preset fm_default

# help
.\build\x64\Debug\F-Synthesizer.exe --help
```

## Unified Check

```powershell
.\scripts\check.ps1
.\scripts\check.ps1 -SkipRun
.\scripts\check.ps1 -SkipBuild -SkipRun
.\scripts\check.ps1 -AllowDocMismatch
```

## Git Hook (Auto Doc Update)

```powershell
.\scripts\install_git_hooks.ps1
```

自動更新対象:
- `docs/Architecture.md`（Auto-Generated ブロック）
- `docs/synth-methods/integration-playbook.md`（Auto-Generated ブロック）
- `docs/architecture/*.md` の `Special Notes`

## GUI Smoke

```powershell
.\scripts\gui_smoke.ps1
```

## GUI Acceptance (Manual)

- `docs-archive/gui-migration/GUI_V8_ACCEPTANCE_TEST.md`

## GUI Validation Contract

`Run` 前に以下を検証する。
- MIDI path 空欄/ファイル不在
- Output path 空欄
- `sampleRate <= 0`
- `initialSeconds <= 0`
- `bits != 16`

## Config Notes

主なキー:
- `midiPath`, `wavPath`, `targetChannel`, `defaultWave`
- `initialSeconds`, `bits`, `sampleRate`, `extraReleaseSec`

プリセット運用:
- `config/base.json` -> `config/presets/<name>.json`（後勝ち）
- `--config` 指定時は `--preset` より `--config` を優先

## Weekly Maintenance

週 1 回（10〜20分）で実施する。

手順:
1. `docs/STATUS.md` の残タスク・既知の問題を棚卸しする
2. 構造変更があれば `docs/Architecture.md` を更新する
3. 優先順位の変化を `docs/STATUS.md` の `Next 3` に反映する
4. 新しい設計判断を `docs/DECISIONS.md` に追記する
5. `docs/architecture/*.md` の `Special Notes` で `TODO (auto-generated)` を確認し、対象があれば ADR を記入する（`背景/判断/代替案/影響範囲/関連ファイル`）

完了チェック:
- 解消済み課題を `STATUS.md` から削除した
- 優先事項が `STATUS.md` の `Next 3` に反映されている
- 新しい判断があれば `DECISIONS.md` に残した
- `docs/architecture/*.md` の `Special Notes` に未記入の `TODO (auto-generated)` が残っていない
