# GUI_MIGRATION_PHASES_v4

v0.4 は「Preview をファイル生成からGUI内再生へ移し、試行錯誤速度を上げつつ、SoAへ直接移行する」ことを目標にする。

このファイルの扱い:
- v0.4 開発中: 更新対象
- v0.4 完了後: `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v4.md` へ移動して凍結

## GUI_V4_PHASE_A_PREVIEW_EXPORT_SPLIT

Status: DONE

- 実行モードを `Preview` と `Export` に分離
- `RenderOptions`（`mode/startSec/durationSec/writeWav/allowCancel`）を導入
- Preview時は短尺・保存なしを既定に設定

完了条件:
- PreviewとExportが明確に分離され、用途に応じた挙動になる

## GUI_V4_PHASE_B_IN_APP_PLAYBACK

Status: DONE

- GUI内再生基盤（`miniaudio`）を導入
- `Play Preview` / `Stop` / `Loop` UIを追加
- Preview結果をWAV保存せずに直接再生

完了条件:
- Preview実行後、GUI内で即再生できる

## GUI_V4_PHASE_C_RUNTIME_CONTROL

Status: DONE

- Render loopにキャンセルチェックを追加
- `Stop` を実キャンセルに接続
- 連続Preview時の状態遷移/ログを安定化

完了条件:
- 長尺MIDIでもPreviewを途中停止できる

## GUI_V4_PHASE_D_DIRECT_SOA_MIGRATION

Status: DONE

- 現行レンダラ実装を SoA データ構造へ直接移行
- Voice/Event のホットパスを SoA 前提で再配置
- GUI/CLI の `Run` I/F は維持し、呼び出し側変更を最小化

完了条件:
- SoA 化後も現行プリセットで破綻なく再生できる
- Preview 応答速度が改善する

## GUI_V4_PHASE_E_STABILIZE_AND_RELEASE

Status: DONE

- v0.4向けスモークテストを追加
- README / Architecture / STATUS をv0.4仕様へ更新（SoA直接移行版）
- v0.3 -> v0.4 移行手順を追加

完了条件:
- 日常運用できる品質でv0.4をリリース可能
