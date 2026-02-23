# GUI_MODE_SPLIT_PHASES

このドキュメントは、GUIを `Sound / Music` 2モードへ分割するためのフェーズ定義です。  
対象は `UI分割のみ` とし、合成機能やレンダ機能の追加は行わない。

## 決定事項（2026-02-21）

- タブ名: `Sound / Music`
- 初期表示: `Sound`
- `Sound` タブの目的: `音源作成 + 音源試聴`
- `Music` タブの目的: 楽曲側確認と書き出し
- `Music` タブの `Export WAV`: `置く`
- `Music` タブの再生: `常に再生成`
- タブ切替時（再生中）: `自動停止`
- ログ表示: `タブ別ログ`
- UIヘルプ: `ホバー中UIの説明を下部1行に常時表示`
- スコープ: `UI分割のみ（機能現状維持）`

## 運用前提

- プロダクト方針は `docs/PRODUCT_POLICY.md` を優先する
- 既存I/F（`Run` / `AppConfig`）互換を維持する
- 既存の動作互換を崩さない範囲でUI配置を変更する

## GUI_MODE_SPLIT_PHASE_1_LAYOUT_AND_TAB_STATE

Status: DONE

- `Sound / Music` タブを導入
- 初期タブを `Sound` に固定
- タブ選択状態を `gui_state` に保存/復元

完了条件:
- 起動時に `Sound` タブが表示される
- タブ切替が安定して動作する

## GUI_MODE_SPLIT_PHASE_2_ACTION_ROUTING

Status: DONE

- `Sound` タブへ音源作成/試聴系UIを配置
- `Music` タブへ `Export WAV` と楽曲確認系UIを配置
- `Music` タブの再生は常に再生成に固定
- タブ切替時に再生中なら自動停止

完了条件:
- 指定タブの目的に沿った操作だけが表示される
- タブ切替時の自動停止が機能する

## GUI_MODE_SPLIT_PHASE_3_LOG_AND_HINT

Status: DONE

- タブ別ログ表示を導入（`Sound` / `Music`）
- ホバー中UIの説明を下部に1行表示するヘルプを導入
- 既存ログの診断情報粒度は維持

完了条件:
- ログがタブ別で確認できる
- ホバー対象に応じてヘルプ文がリアルタイムで切り替わる

## GUI_MODE_SPLIT_PHASE_4_COMPAT_AND_POLISH

Status: DONE

- `scripts/gui_smoke.ps1` 互換を維持
- 既存操作（Preview/Stop/Export）の回帰確認
- 文言を目的語中心に調整
- 固定ヒントを廃止し、ホバーUIヘルプ（下部1行）へ統一

完了条件:
- 既存スモークが通る
- UI分割後も既存機能が壊れていない

検証結果:
- `scripts/gui_smoke.ps1` 13/13 通過
