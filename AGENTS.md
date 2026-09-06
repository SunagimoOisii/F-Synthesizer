# AGENTS.md

## 目的と優先順位

手持ちの MIDI に好みの音色を手軽に割り当て、自分版の曲を作る。
個人開発で、現在の Windows PC と身内程度の利用を想定する。
実装は AI に任せたい。学習のための自作や商用展開は目的にしない。

- メガドライブ、X68000、コナミ系の FM サウンドが好み。
- プリセット選択と微調整を中心に、初心者でも操作しやすくする。
- 再生中の音色変更を速く反映する。ドラムはセット単位で扱う。
- 簡単な音符編集を維持する。機能の多さより操作の快適さを優先する。
- 付属プリセットは読み取り専用の出発点。保存は曲か自分用のコピーへ。
- 無料で MIT / BSD / zlib などの依存を優先する。
- 旧データ互換、汎用化、大規模テスト基盤の維持を目的にしない。

## 作業の始め方

1. `git status --short --branch` と [README.md](README.md) を読む。
2. 依頼に関係するコードを読み、ユーザーの未コミット変更を保つ。
3. 変更に合った短い検証を実行し、未確認事項を報告する。

## コードの見取り図

実装本体は `F-Synthesizer/`。

| 場所 | 担当 |
|---|---|
| `src/gui/` | 画面、音色・音符編集、保存、音声デバイス |
| `src/project/`, `include/project/` | Instrument と曲のモデル |
| `src/config/` | 共通 JSON の読込・保存・検証 |
| `src/midi/` | midifile とアプリの演奏イベントの接続 |
| `src/app/`, `src/core/` | 曲の演奏・書き出しと音生成の接続 |
| `src/SynthEngine/`, `src/synth/` | ボイス、ymfm 接続、その他の音源とエフェクト |
| `third_party/` | バージョンを固定した ymfm / midifile の原文ソース |

- 音色と metadata は `GUIState.instruments` が一緒に保持する。
- `GUIProjectFacade` が編集状態と `ProjectModel` を変換し、チャンネル割当を解決する。
- JSON は `config::ProjectToJSON / ProjectFromJSON / WriteJSONFile` を共通利用する。
- `GUIStatePersistence` は workspace と名前付き曲を保存する。`.fsynth` は MIDI 本体も含む。
- `GUIPresetIO` は付属音色をコピーし、自分用音色は `config/user_presets/` に新規保存する。
- 実行経路は `ProjectModel -> RenderConfig -> SynthEngine`。別の音生成・保存経路を増やさない。
- 再生中の設定は `LiveRenderMailbox` へ immutable なスナップショットを公開し、生成側が64サンプル以下の区切りで受け取る。
- `PreviewAudio` は約20msのリングを使う。音声コールバックへ可変GUI参照やファイル処理を渡さない。
- ループは都度生成する。古い PCM を使い回して編集反映を失わないようにする。
- `YmfmVoice` はボイスごとにチップを保持し、miniaudio で出力サンプルレートへ変換する。

## ビルド

Visual Studio 2022 の C++ ビルドツールを使う。IDE を開く必要はない。
利用者の入口はルートの `start.cmd`。Release をビルドして GUI を起動する。
`start.cmd --build-only` はビルドのみ。通常の開発用チェックは Debug。

この PC の既存依存は Dear ImGui、GLFW、miniaudio、nlohmann/json。
ymfm（BSD-3-Clause）と midifile（BSD-2-Clause）は同梱済みで、ビルド時のダウンロードは不要。
固定リビジョン・著作権・配布時の原文は [サードパーティ通知](F-Synthesizer/licenses/THIRD_PARTY_NOTICE.md) を参照。

環境を作り直す場合の GUI 依存:

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

通常はビルドのみ。音声・MIDI変更では短尺 runtime smoke、付属音色変更では preset check を追加する。
保存・再生反映の変更では、ビルド後に persistence check を使う。
同スクリプトは既存オブジェクトを再利用し、隔離した場所で曲保存・原本保護・MIDI時刻・FM音程・再生中の反映も確認する。
GUI変更では必要に応じて短く表示を確認する。大規模な自動化基盤は追加しない。
操作感と音の好みはユーザーの試用を受けて調整する。

## 今後の扱い

合意した音源接続・即時反映・画面整理・曲保存・プリセット刷新は実装済み。
次はユーザーの実際の MIDI と操作感のフィードバックを扱う。
録音素材が必要になった場合は TinySoundFont（MIT）を検討する。素材自体のライセンスは別に確認する。

## 文書運用

利用者向けは README、AI 向けはこのファイル。ライセンス通知・原文は別に維持する。
機能一覧、方針書、設計評価書、長期計画、完了ログを別文書に増やさない。
具体的な未完了作業が生じたらここへ短く記載し、完了したら消す。履歴は Git を参照する。
