# GUI_MIGRATION_PHASES_v2

v0.2 は「チャンネルごとの音設定をGUIから編集・保存・再利用できる状態」を目標にする。

このファイルの扱い:
- v0.2 開発中: 更新対象
- v0.2 完了後: `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v2.md` へ移動して凍結

## GUI_V2_PHASE_A_CONFIG_SCHEMA

Status: DONE

- `config` のチャンネル定義フォーマットを策定
- 既存 `base/preset` との後方互換方針を決定
- `AppConfig` で表現できる構造へ整備

完了条件:
- CLI/GUIで共通利用できるJSONスキーマが確定している

## GUI_V2_PHASE_B_CONFIG_IO

Status: DONE

- チャンネル定義のロード実装
- チャンネル定義の保存実装
- 値範囲バリデーション（ch, amp, ADSR, wave type 等）を追加

完了条件:
- JSON編集のみでチャンネル音設定を変更し、実行に反映できる

## GUI_V2_PHASE_C_CHANNEL_EDITOR_UI

Status: DONE

- ch0-15 切替UI（タブ/リスト）を追加
- Source切替UI（Waveform/Noise/FM/Drum/DrumKit）を追加
- ADSR/amp/波形等の編集UIを追加

完了条件:
- GUIだけでチャンネル個別設定を編集して実行できる

## GUI_V2_PHASE_D_PRESET_WORKFLOW

Status: DONE

- `Save preset as` / `Duplicate` / `Reset channel` を追加
- baseとの差分管理方針を実装
- 変更有無の可視化（dirty表示）を追加

完了条件:
- 単一曲ループ制作でプリセット運用がGUI中心で回せる

## GUI_V2_PHASE_E_STABILIZE_AND_RELEASE

Status: DONE

- 最小テスト（読込/保存/実行/失敗系）を整備
- README / Architecture / STATUS をv0.2仕様へ更新
- 旧設定からの移行手順を明記

完了条件:
- 日常運用できる品質でv0.2をリリース可能
