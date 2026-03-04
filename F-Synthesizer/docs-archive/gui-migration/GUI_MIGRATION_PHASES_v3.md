# GUI_MIGRATION_PHASES_v3

v0.3 は「各チャンネル設定をGUI上で素早く試聴し、ミックス調整まで完結できる状態」を目標にする。

このファイルの扱い:
- v0.3 開発中: 更新対象
- v0.3 完了後: `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v3.md` へ移動して凍結

## GUI_V3_PHASE_A_MIX_ENGINE_BASE

Status: DONE

- チャンネルごとの `mute/solo/level/pan/gain` 状態を実行コアへ追加
- 既存レンダリング経路にミックス係数を統合
- 既存 `config` との互換方針（未指定時は従来値）を定義

完了条件:
- GUIから設定したミックス状態が音声生成に正しく反映される

## GUI_V3_PHASE_B_CHANNEL_MONITOR_UI

Status: DONE

- ch0-15 の `mute/solo` トグルUIを追加
- ch0-15 の `level/pan/gain` 調整UIを追加
- 最小メータ（peak）とクリップ警告を表示

完了条件:
- 各チャンネルをGUIで視認しながら調整できる

## GUI_V3_PHASE_C_AUDITION_WORKFLOW

Status: DONE

- 「チャンネル単体試聴」導線を追加（solo連動）
- 再生/停止と即時反映フローを追加
- 単体試聴時の状態復帰ルール（解除時に元状態へ戻す）を定義

完了条件:
- 1チャンネルずつ短時間で試聴・比較できる

## GUI_V3_PHASE_D_STATE_PERSISTENCE

Status: DONE

- ミックス状態（mute/solo/level/pan/gain）を保存対象へ追加
- 既存プリセット保存/読込との整合を確保
- GUIの `Preset` 切替時に `midiPath/wavPath` だけでなく `channels` も反映
- GUI と CLI（`--preset`）のプリセット適用挙動を一致
- 旧プリセット読込時のデフォルト補完を実装

完了条件:
- GUI終了後も前回のチャンネル状態を再現できる
- GUIプリセット切替だけで音色/音源変更まで反映される

## GUI_V3_PHASE_E_STABILIZE_AND_RELEASE

Status: DONE

- 最小テスト（正常系/異常系）を追加
- README / Architecture / STATUS をv0.3仕様へ更新
- v0.2 -> v0.3 の移行手順を追加

完了条件:
- 日常運用できる品質でv0.3をリリース可能
