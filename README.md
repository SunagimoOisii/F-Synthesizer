# F-Synthesizer Workspace

このリポジトリは、`F-Synthesizer/` 配下に実装本体を持つワークスペースです。  
現在の実装状況は GUI（Sound/Music モード分離）とピアノロール直接操作を含む構成です。

## 主要ディレクトリ

- `F-Synthesizer/`: プロジェクト本体（`F-Synthesizer.vcxproj`、`src/`, `include/`, `docs/`）
- `F-Synthesizer.sln`: Visual Studio ソリューション

## 最初に見るドキュメント

- `F-Synthesizer/docs/STATUS.md`: 現在の進捗と次アクション
- `F-Synthesizer/docs/Architecture.md`: 実装アーキテクチャ
- `F-Synthesizer/README.md`: 実行手順、GUI運用、ドキュメント運用

## 現在の実装スナップショット

- MIDI解析（Note/Tempo/CC/Pitch Bend）
- tick -> sample 変換
- 音源（Waveform / Noise / FM / Drum / DrumKit）
- GUI（Sound/Music タブ、Preview再生、Export、折りたたみログ）
- ピアノロール直接操作（選択/移動/伸縮/作成、ホイール操作、ルーラクリック開始、Space再生停止、Snapショートカット）

## ビルド

1. Visual Studio で `F-Synthesizer.sln` を開く
2. `Debug | x64` を選択
3. ビルドして実行

CLI 実行例や補助スクリプトは `F-Synthesizer/README.md` を参照してください。
