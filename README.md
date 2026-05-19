# F-Synthesizer Workspace

F-Synthesizer は C++ 製デスクトップシンセサイザのワークスペースです。実装本体は
`F-Synthesizer/` 配下にあり、GUI ツールとしても、MIDI を WAV に変換する CLI
レンダラとしても実行できます。

## 再開手順

1. Visual Studio 2022 で `F-Synthesizer.sln` を開く。
2. `Debug | x64` を選ぶ。
3. ソリューションをビルドする。
4. Visual Studio から実行するか、`F-Synthesizer/scripts/check.ps1` でコマンドラインビルドを確認する。

## 現在の状態

- MIDI 解析は Note / Tempo / CC / Pitch Bend / Pressure / 基本的なチャンネル処理に対応。
- レンダリングは waveform、PSG 風プリセット、noise、FM、drum、drumkit を扱う。
- GUI は「遊ぶ / 作る / 書き出す」導線を中心に、ピアノロール直接編集とステップシーケンサーを持つ。
- 音作りは、短いプレビュー導線、初心者向け操作、レトロゲーム風の音色を優先する。

## 次にやること

- ドキュメントと検証導線の整理後に、アプリを手動で再確認する。
- 通常検証は小さく保つ。まずビルドし、音声・レンダリング変更時のみ runtime smoke を使う。
- 機能追加は「音の気持ちよさ」または「編集から試聴までの短さ」に効くものを優先する。

## 主要パス

- `F-Synthesizer/`: プロジェクト本体、設定、スクリプト、ドキュメント
- `F-Synthesizer.sln`: Visual Studio ソリューション
- `F-Synthesizer/README.md`: プロジェクト単位の再開・ビルド・実行・ドキュメント案内
- `F-Synthesizer/AGENTS.md`: AI エージェント向け作業ルール
