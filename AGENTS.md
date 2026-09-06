# AGENTS.md

## このアプリの目的

手持ちの MIDI に自分好みの音色を手軽に割り当て、自分版の曲を作る。
本人は初心者で、実装はできるだけ AI に任せる。現在の Windows PC で快適に使えればよい。

- 好みはメガドライブ、X68000、コナミ系の FM サウンド。
- プリセットからの微調整を中心にし、付属プリセットを編集・保存操作で上書きしない。
- 音色変更を再生中の曲へすぐ反映することを重視する。
- 簡単な音符編集は残す。機能の網羅性より操作の快適さを優先する。
- ドラムはセット単位で選ぶ。録音素材も必要なら使ってよい。
- 自作へのこだわりはない。無料で MIT / BSD / zlib などの依存を優先する。
- 旧データ互換、大規模テスト基盤、汎用フレームワークの維持を目的にしない。

## 作業の始め方

1. `git status --short --branch` と [README.md](README.md) を読む。
2. 依頼に必要なコードを読む。ユーザーの未コミット変更を保つ。
3. 変更後は下記の短いチェックを実行し、未確認事項を報告する。

## コードの見取り図

実装本体は `F-Synthesizer/`。フォルダを増やすより、既存の責務を明確にする。

| 場所 | 担当 |
|---|---|
| `src/gui/` | 画面、入力、音色編集、ピアノロール、音声再生 |
| `src/project/`, `include/project/` | Instrument と曲の保存モデル |
| `src/config/` | JSON の読込・保存、音源設定の検証 |
| `src/midi/` | MIDI 読込と演奏時刻への変換 |
| `src/app/`, `src/core/` | 実行入力の準備、試聴・書き出しから音生成への接続 |
| `src/SynthEngine/`, `src/synth/` | 音源、ボイス、変調、エフェクト |
| `src/io/` | パスと WAV 出力 |

- GUI の音色は `GUIState.instruments` が保持する。metadata と音色本体を別々に保存しない。
- `GUIProjectFacade` が編集状態と `ProjectModel` を変換する。共有スロットの割当もここで扱う。
- 音色と曲の JSON は `config::ProjectToJSON / ProjectFromJSON` を共通利用する。
- `GUIStatePersistence` は同じ project JSON に画面・音符の状態を加え、一つの `workspace.json` に保存する。
- `GUIPresetIO` は付属プリセットを読み取り、自分用音色だけを `config/user_presets/` に新規保存する。
- 実行経路は `ProjectModel -> RenderConfig -> SynthEngine`。GUI の音生成処理や別の保存実装を増やさない。
- 再生スレッドへ編集データの可変参照を渡さない。必要なスナップショットだけを渡す。

## ビルド環境

Visual Studio 2022 の C++ ビルドツールを使う。IDE を開く必要はない。
既存の依存は Dear ImGui、GLFW、miniaudio、nlohmann/json。
ライセンスの正本は `F-Synthesizer/licenses/THIRD_PARTY_NOTICE.md`。

初回だけ、必要なら以下で GUI の依存を導入する。

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
& C:\vcpkg\bootstrap-vcpkg.bat
& C:\vcpkg\vcpkg.exe install glfw3:x64-windows imgui[glfw-binding,opengl3-binding,docking-experimental]:x64-windows
& C:\vcpkg\vcpkg.exe integrate install
```

## 検証

リポジトリ直下から実行する。

```powershell
.\F-Synthesizer\scripts\check.ps1
.\F-Synthesizer\scripts\check.ps1 -RunRuntimeSmoke
.\F-Synthesizer\scripts\check_presets.ps1
.\F-Synthesizer\scripts\check_persistence.ps1
```

通常はビルドのみ。音声・MIDI・設定変更では短尺 runtime smoke、付属プリセット変更では preset check を追加する。
保存処理変更では、ビルド後に `check_persistence.ps1` で保存→再読込の一致と付属プリセットの保護を確認する。
大規模なテスト基盤や長時間の全件確認を毎回要求しない。操作感と音の好みは必要な場面で短く手動確認する。

## 残っている作業

- 再生中の編集を即時反映する。現在は最大8秒の先読み PCM を再生するため、待ち時間の数字だけを縮めて済ませない。小さなブロック単位の音生成と編集反映を設計・計測する。
- チャンネル一覧、プリセット選択、微調整、試聴を近くに配置し、簡単なノート編集を残して GUI を整理する。
- 曲ごとの名前付き保存と、目的の FM サウンドに寄せた音源・プリセットを整える。ymfm 等への置換は、必要な音・編集速度・接続コード量を小さな試作で比較して決める。

## 文書運用

利用者向けは README、AI 向けはこのファイル。ライセンス通知・原文は別に維持する。
機能一覧、方針書、設計評価書、長期計画、完了ログを別文書に増やさない。
このファイルの残作業は短く保ち、完了したら消す。履歴は Git を参照する。
