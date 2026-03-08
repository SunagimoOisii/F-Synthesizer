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
.\scripts\check.ps1 -GuiSmokeProfile full
.\scripts\check.ps1 -GuiSmokeProfile full -RunMIDIRegression
.\scripts\check.ps1 -AllowDocMismatch
```

## Personal Verification Flow (Lightweight)

個人運用前提として、重い自動ハーネス（大規模回帰自動化）は導入しない。
日常運用は以下を標準とする。

1. 通常系は `.\scripts\check.ps1` のみを実行する（Build + GUI smoke quick + Doc rules）
2. 代表MIDI（最低1つ）の手動確認は、品質確認が必要な変更時のみ追加で実施する
3. 音色・レンダ品質に影響する場合のみ、`.\scripts\check.ps1 -GuiSmokeProfile full -RunMIDIRegression` を実行する

手動確認の観点:
- 異常なノイズ/破綻音がない
- クリップ増加がない（必要時は比較ログを残す）
- 変更対象パラメータが意図どおり反映される

## Git Hook (Lightweight)

```powershell
.\scripts\install_git_hooks.ps1
```

運用方針:
- pre-commit では md 自動更新を行わない（時刻のみ差分のノイズを避けるため）
- `docs/Architecture.md` の Auto-Generated 更新は必要時のみ手動実行（`check.ps1`）

## GUI Smoke

```powershell
.\scripts\gui_smoke.ps1
```

`gui_smoke.ps1` はデバッグ用途の直接実行向け。通常運用では `check.ps1` 経由を正とする。

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
- `midiPath`, `wavPath`, `targetChannel`
- `initialSeconds`, `bits`, `sampleRate`, `extraReleaseSec`

プリセット運用:
- `config/base.json` -> `config/presets/<name>.json`（後勝ち）
- `--config` 指定時は `--preset` より `--config` を優先

## Weekly Maintenance

週 1 回（10〜20分）で実施する。

情報保持ポリシー:
- 短期入力（次回週次まで）: `docs/STATUS.md` の `Current`
- 長期判断（恒久）: `docs/DECISIONS.md`
- 長期契約（恒久）: `docs/synth-methods/foundation-contract.md`
- ADR詳細（恒久）: `docs/architecture/*.md` の `Special Notes`
- 履歴退避（恒久・参照専用）: `docs-archive/`

手順:
1. `docs/STATUS.md` の残タスク・既知の問題を棚卸しし、更新要否を判定する
2. 判定結果に応じて、必要なファイルだけ更新する
   - `STATUS.md`: 進捗/優先順位が変わった時
   - `DECISIONS.md`: 新しい設計判断が発生した時
   - `docs/synth-methods/foundation-contract.md`: 契約（capability/lifecycle/schema）が変わった時
   - `docs/architecture/*.md`: 設計判断の背景詳細を残す必要がある時のみ `Special Notes` に ADR を追記
   - それ以外の文書: 差分がある時のみ更新（なければ更新しない）
3. `docs/architecture/*.md` の `Special Notes` へ追記する場合は、`背景/判断/代替案/影響範囲/関連ファイル` を記入する
4. 変更がなかった文書は記録しない（`No update` の記載は不要）

週次AI更新の入出力:
- 入力: `STATUS.md`（短期情報）+ 直近コード差分（git）
- 出力: `STATUS.md` / `DECISIONS.md` / `foundation-contract.md` / 必要時 `docs/architecture/*.md`

完了チェック:
- 変更が必要だった文書のみ更新されている
- 優先事項が変わった場合のみ `STATUS.md` の `Next 3` を更新した
- 新しい判断がある場合のみ `DECISIONS.md` に追記した
- 契約変更がある場合のみ `foundation-contract.md` を更新した
- `docs/architecture/*.md` へ追記した場合、ADR 5項目が埋まっている
