# 運用

F-Synthesizer のビルド、実行、検証メモです。

## Visual Studio

1. `..\F-Synthesizer.sln` を開く。
2. `Debug | x64` を選ぶ。
3. ビルドして実行する。

## 依存関係

Visual Studio 2022 の MSBuild と、vcpkg で導入した GUI 実行時依存を前提にします。

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
& C:\vcpkg\bootstrap-vcpkg.bat
& C:\vcpkg\vcpkg.exe install glfw3:x64-windows imgui[glfw-binding,opengl3-binding,docking-experimental]:x64-windows
& C:\vcpkg\vcpkg.exe integrate install
```

## コマンド

```powershell
# 標準検証
.\scripts\check.ps1

# CLI / render / config 変更時の任意 runtime smoke
.\scripts\check.ps1 -RunRuntimeSmoke

# GUI
.\build\x64\Debug\F-Synthesizer.exe
.\build\x64\Debug\F-Synthesizer.exe --gui

# CLI
.\build\x64\Debug\F-Synthesizer.exe --cli
.\build\x64\Debug\F-Synthesizer.exe --cli --config .\config\default.json
.\build\x64\Debug\F-Synthesizer.exe --cli --preset wave_snes_lead_vibrato
.\build\x64\Debug\F-Synthesizer.exe --help
```

## 検証方針

- 通常導線は `.\scripts\check.ps1`。
- `-RunRuntimeSmoke` は、render、audio、config、CLI、実行ファイル起動に影響する変更時だけ使う。
- UX や音の気持ちよさに関わる変更では、GUI と音声の手動確認を行う。
- 現在の作業で明示的に必要な場合を除き、長時間の回帰スイートは追加しない。

## 実行時出力

- ビルド出力: `build/`
- 生成音声・検証出力: `output/`
- ローカル GUI 状態: `config/gui_state.json`
- ローカル piano-roll project 状態: `config/piano_roll_project.json`

生成された build / audio 出力は Git 管理対象外です。

## 保守

- `README.md` を再開入口として正確に保つ。
- モジュール境界、実行フロー、設定契約、GUI 契約、レンダリング契約が変わった場合は `docs/Architecture.md` を更新する。
- コマンド、依存関係、検証方針が変わった場合はこのファイルを更新する。
- 履歴ログはリポジトリに増やさない。古い文脈が必要な場合は Git 履歴を使う。
