# core 監査ログ（Phase 5）

最終更新: 2026-03-04
準拠ポリシー: `docs/cleanup/deletion-policy.md`

## 対象と観点

- 対象: `src/core`, `include/core`（依存境界を含む）
- 観点:
  - 未使用処理
  - 過去互換コード
  - 性能対策の残骸

## 調査サマリ

- 対象ファイル:
  - `src/core/AudioBuffer.cpp`
  - `src/core/RenderGateway.cpp`
  - `include/core/AudioBuffer.h`
  - `include/core/RenderGateway.h`
- ファイル参照は全件 `ref>0`（未参照ファイルなし）。
- `F-Synthesizer.vcxproj` へのビルド接続も全件確認済み。
- 依存境界は `app -> core(RenderGateway) -> SynthEngine` の単一呼び出しで維持。

## 判定結果

| Path | Kind | Evidence (ref=0) | Decision | Note |
|---|---|---|---|---|
| src/core/AudioBuffer.cpp | file | ref=2 | keep | `SoundData` 実装として app/gui/io/SynthEngine から利用 |
| src/core/RenderGateway.cpp | file | ref=5 | keep | app層の単一レンダ入口として利用 |
| include/core/AudioBuffer.h | file | ref=7 | keep | 広範囲で型境界として使用 |
| include/core/RenderGateway.h | file | ref=4 | keep | `RunExecution.cpp` から利用 |
| src/core/AudioBuffer.cpp:5 | symbol | `kPi` は参照0件 | delete | 未使用定数を削除（同時に `<cmath>` 依存を除去） |
| src/core/AudioBuffer.cpp:17-22 相当 | block | `data` の手動0初期化ループ | delete | `std::vector<double>(length)` の値初期化と重複するため削除 |

## 実施変更

- `src/core/AudioBuffer.cpp` から未使用定数 `kPi` を削除。
- 同ファイルから冗長な `for` 初期化ループを削除。

## 検証

- 必須検証: `Debug x64` ビルド成功（2026-03-04、warning=0 / error=0）。

## 結論

- ファイル削除対象はなし。
- コード断片レベルでは、未使用・冗長処理の削除を実施済み。

