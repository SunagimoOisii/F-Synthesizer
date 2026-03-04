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

- `docs/GUI_V8_ACCEPTANCE_TEST.md`

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
