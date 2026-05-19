# F-Synthesizer

F-Synthesizer は、MIDI を WAV にレンダリングする C++ シンセサイザです。GUI では
音色作成、試聴、ピアノロール編集、ステップシーケンサー、書き出しを扱えます。

## 1分で再開

```powershell
git status --short --branch
.\scripts\check.ps1
```

対話的に作業する場合は、Visual Studio 2022 で `..\F-Synthesizer.sln` を開き、
`Debug | x64` を選んでビルド・実行します。

## 現在の状態

- GUI: 遊ぶ / 作る / 書き出す導線、Sound Preview、Waveform / Spectrum / VU 表示、ピアノロール編集、ドラムステップシーケンサー。
- レンダリング: waveform、analog 風 waveform、FM、noise、drum、drumkit、stereo pan、pitch bend、pressure、modulation、master effects。
- 設定: `config/base.json`、`config/default.json`、`config/presets/*.json`、GUI ワークスペース状態。
- 方針: 短い編集→試聴サイクル、気持ちよいレトロゲーム風サウンド、初心者が触りやすい操作。

## 次にやること

1. この整理後の検証導線を実行する。
2. GUI を起動し、遊ぶ / 作る / 書き出すの主要導線が開けることを確認する。
3. 機能作業は、音の気持ちよさまたは試聴速度の改善に効くものから再開する。

## ビルドと実行

```powershell
# ビルド / 標準チェック
.\scripts\check.ps1

# レンダリング・音声・CLI・config 読み込みを触った後だけ使う任意 smoke
.\scripts\check.ps1 -RunRuntimeSmoke

# GUI 実行
.\build\x64\Debug\F-Synthesizer.exe

# CLI 実行
.\build\x64\Debug\F-Synthesizer.exe --cli
.\build\x64\Debug\F-Synthesizer.exe --cli --preset wave_snes_lead_vibrato
.\build\x64\Debug\F-Synthesizer.exe --cli --config .\config\default.json
```

## ドキュメント

- `AGENTS.md`: AI エージェント向けルールと更新方針。
- `docs/Architecture.md`: アーキテクチャ、データフロー、設定、GUI、音源契約。
- `docs/PRODUCT_POLICY.md`: プロダクト価値と機能判断ルール。
- `docs/OPERATIONS.md`: ビルド、依存関係、実行、検証の詳細。
- `docs/PRESETS.md`: プリセット一覧。
- `licenses/` と `docs/THIRD_PARTY_LICENSES.md`: ライセンス関連。

ドキュメントは短く保ちます。過去の判断や完了履歴は、長いステータスログではなく
Git 履歴から参照します。
