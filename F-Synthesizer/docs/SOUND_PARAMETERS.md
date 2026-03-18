# Sound パラメータ参照ガイド

最終更新: 2026-03-19

この文書は Sound パラメータの**概要と参照先**のみを示す。
パラメータ仕様の正本は `docs/Architecture.md` と `docs/architecture/` 配下に統一する。

## 参照先（正本）

| 観点 | 正本 |
|---|---|
| 全体方針（どこを正本にするか） | `docs/Architecture.md` |
| Sound 編集UIの責務・導線 | `docs/architecture/gui.md` |
| 依存境界・音響アルゴリズム制約 | `docs/architecture/module-map.md` |
| 実行経路（Run→MIDI→Render） | `docs/architecture/runtime-flow.md` |
| source 契約（capability / lifecycle / schema） | `docs/synth-methods/foundation-contract.md` |

## Sound 方式別の確認先

- Waveform / FM / Noise / DrumKit の方式境界: `docs/architecture/module-map.md`
- Sound タブ編集挙動（補助説明・表示契約）: `docs/architecture/gui.md`
- Drum 未指定値（`0` / 負値）時の扱い: `docs/architecture/module-map.md` の `Special Notes`

## 運用ルール

- パラメータ仕様を更新する場合は、このファイルではなく正本側を更新する。
- このファイルは参照導線の維持のみ行う（新仕様の詳細追記はしない）。
