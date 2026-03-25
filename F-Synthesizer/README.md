# F-Synthesizer

MIDI ファイルを読み込み、サンプル単位で合成して WAV に書き出す C++ シンセサイザです。

## 1分で再開

1. `docs/STATUS.md` を読む
2. `config/default.json` と `config/presets/` を確認
3. `F-Synthesizer.vcxproj` を `x64 Debug` でビルド

## 主要ドキュメント

- 実行・運用詳細: `docs/OPERATIONS.md`
- 現在状態: `docs/STATUS.md`
- 設計: `docs/Architecture.md`
- プロダクト方針: `docs/PRODUCT_POLICY.md`
- GUI契約: `docs/GUI_REQUIREMENTS.md`
- コメント規約（必須版）: `agents/standards/COMMENT_GUIDELINE.md`
- コメント規約（詳細版）: `agents/standards/COMMENT_GUIDELINE_FULL.md`
- 音色パラメータ: `docs/SOUND_PARAMETERS.md`
- プリセット一覧: `docs/PRESETS.md`
- 合成方式/契約: `docs/synth-methods/foundation-contract.md`
- 設計判断: `docs/DECISIONS.md`
- 直近計画: `docs/STATUS.md` の `Next 3`
- アーカイブ: `docs-archive/`

## クイックコマンド

```powershell
git status --short --branch
.\scripts\check.ps1
.\scripts\check.ps1 -GuiSmokeProfile full -RunMIDIRegression
```

## リポジトリ構成（最小）

- `src/`: 実装
- `include/`: 公開ヘッダ
- `config/`: 実行設定とプリセット
- `scripts/`: ビルド/検証スクリプト
- `docs/`: 現行ドキュメント
- `docs-archive/`: 凍結済み履歴
