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

# CLI / render / audio / config 変更時だけ使う任意 runtime smoke
.\scripts\check.ps1 -RunRuntimeSmoke

# config / preset 変更時の短尺全件検証
.\scripts\check_presets.ps1

# GUI
.\build\x64\Debug\F-Synthesizer.exe
.\build\x64\Debug\F-Synthesizer.exe --gui

# CLI
.\build\x64\Debug\F-Synthesizer.exe --cli
.\build\x64\Debug\F-Synthesizer.exe --cli --config .\config\default.json
.\build\x64\Debug\F-Synthesizer.exe --cli --preset retro_heavy_fm_brass_ensemble
.\build\x64\Debug\F-Synthesizer.exe --help
```

## 検証方針

- 通常導線は `.\scripts\check.ps1`。
- `-RunRuntimeSmoke` は毎回不要。render、audio、config、CLI、実行ファイル起動に影響する変更時だけ使う。
- runtime smoke は `projectModel.v1` の最小 config と最小 MIDI を一時生成し、ProjectModel 読み込み、RenderConfig 変換、短い WAV、missing config の失敗を確認する。
- config / preset を追加・変更・削除した場合は `.\scripts\check_presets.ps1` を使う。`config/base.json`、`config/default.json`、全 preset の `projectModel.v1` 構造、読み込み、短尺レンダー、無音でないことをまとめて見る。
- UX や音の気持ちよさに関わる変更では、GUI と音声の手動確認を行う。
- 現在の作業で明示的に必要な場合を除き、長時間の回帰スイートは追加しない。

## 実装前確認

大きめの変更では、コード編集前に次を確認します。

- 主な責務が `GUI / App / Core / SynthEngine / Config / MIDI / IO` のどこに属するか。
- 公開型、保存形式、preset、schema、GUI 状態へ影響するか。
- 必要な検証が標準 check、runtime smoke、preset check、手動 GUI/音声確認のどれか。
- `SynthEngine.h`、`GUIState`、config load/save/schema、GUI `.inl` 群へ変更が集中していないか。

## 実行時出力

- ビルド出力: `build/`
- 生成音声・検証出力: `output/`
- ローカル GUI 状態: `config/gui_state.json`
- ローカル piano-roll project 状態: `config/piano_roll_project.json`

生成された build / audio 出力は Git 管理対象外です。

## 保守

- `README.md` を再開入口として正確に保つ。
- モジュール境界、実行フロー、設定契約、GUI 契約、レンダリング契約が変わった場合は `docs/Architecture.md` を更新する。
- 開発方針や設計基準が変わった場合は `docs/ArchitectureAssessment.md` を更新する。
- コマンド、依存関係、検証方針が変わった場合はこのファイルを更新する。
- 履歴ログはリポジトリに増やさない。古い文脈が必要な場合は Git 履歴を使う。
