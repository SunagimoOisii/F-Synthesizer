# midi 監査ログ（Phase 4）

最終更新: 2026-03-04
準拠ポリシー: `docs/cleanup/deletion-policy.md`

## 対象と観点

- 対象: `src/midi`, `include/midi`
- 観点:
  - 未参照パーサ経路
  - 重複変換
  - 旧フォールバックの要否

## 調査サマリ

- ファイル参照を `rg` で確認し、`ref=0` は 0 件。
  - `src/midi/MIDIParser.cpp`: ref=7
  - `src/midi/MidiPipeline.cpp`: ref=8
  - `src/midi/Sequencer.cpp`: ref=8
  - `include/midi/MIDIParser.h`: ref=10
  - `include/midi/MidiPipeline.h`: ref=4
  - `include/midi/Sequencer.h`: ref=9
- `BuildSampleEvents` は `BuildMidiPipeline` から単一路で呼ばれており、midi層内で重複実装は確認なし。

## 判定結果

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| src/midi/MIDIParser.cpp | file | ref=7 | keep | `LoadMIDIBasic` は app実行経路とGUIピアノロール読込で使用 |
| src/midi/MidiPipeline.cpp | file | ref=8 | keep | Parse->tick->sample の統合経路として現役 |
| src/midi/Sequencer.cpp | file | ref=8 | keep | `BuildSampleEvents` を提供、他経路で代替なし |
| include/midi/MIDIParser.h | file | ref=10 | keep | GUI/app/midi各層が参照 |
| include/midi/MidiPipeline.h | file | ref=4 | keep | app実行コアで利用 |
| include/midi/Sequencer.h | file | ref=9 | keep | midi変換で利用 |
| src/SynthEngine/Voices.cpp:228 | branch | `noteInstanceId` 優先 + `ch+note` フォールバック | keep | `MIDIParser` 側で `PopNoteInstanceId` が `-1` を返す経路があり、異常/不整合入力への互換保護として必要 |
| src/midi/MidiPipeline.cpp:10 | function | window開始時のCC/Pitch補完 (`BuildWindowedEvents`) | keep | 部分再生で制御状態を再現するため必要 |

## 重複変換の評価

- midi層内の `tick->sample` は `Sequencer.cpp` に集約され、重複なし。
- GUI側に tempo/tick 換算ロジック（`GUIActions.cpp`, `pianoroll/PianoRollTempo.inl`）は存在するが、用途はUI表示・範囲計算であり、オーディオイベント生成の重複実装ではないため現時点は `keep`。

## 結論

- 本フェーズの `delete` 対象はなし。
- 旧フォールバック（`MarkNoteOff` の `ch+note`）は、異常入力保護として現時点では維持が妥当。

