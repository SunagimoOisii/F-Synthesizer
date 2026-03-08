# gui 監査ログ（Phase 3）

最終更新: 2026-03-04
準拠ポリシー: `docs/cleanup/deletion-policy.md`

## 対象と観点

- 対象: `src/gui`, `include/gui`
- 観点:
  - 未使用UI部品
  - 到達不能分岐
  - 旧UI互換コード

## 調査サマリ

- `src/gui/*.cpp` 12件、`include/gui/*.h` 12件の参照を `rg` で確認。
- 参照0件（`ref=0`）のファイルはなし。
- `src/gui/main/*.inl`, `src/gui/pianoroll/*.inl` も確認し、参照0件なし。
- 明確な到達不能分岐（`#if 0`, `if (false)` など）は検出なし。

## 判定結果

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| src/gui/*.cpp (12 files) | file | すべて ref>=2 | keep | ビルド対象として `F-Synthesizer.vcxproj` に接続済み |
| include/gui/*.h (12 files) | file | すべて ref>=4 | keep | GUI実装・呼び出し側から参照あり |
| src/gui/main/*.inl | file | すべて ref>=5 | keep | GUIMain 経路で使用 |
| src/gui/pianoroll/*.inl | file | すべて ref>=5 | keep | GUIPianoRoll 経路で使用 |
| src/gui/GUIChannelEditor.cpp:117 | branch | legacy drum 変換分岐 | keep | `DrumConfig` は現行 `SourceConfig` の一部で、config/engine 側も運用中 |
| src/gui/main/MainWindow.inl:720 | branch | DrumConfig/DrumKitConfig 判定 | keep | ch10 ドラム運用ガードとして到達可能 |
| src/gui/main/MainWindow.inl:828 | branch | DrumConfig/DrumKitConfig 判定 | keep | ミキサー注記表示で到達可能 |

## 補足

- `DrumConfig` は GUI だけの旧互換ではなく、`include/SynthEngine/SynthEngine.h` の `SourceConfig`（variant）に含まれており、`src/config/*` と `src/SynthEngine/*` でも使用中。
- 現フェーズでは `delete` 対象なし。

