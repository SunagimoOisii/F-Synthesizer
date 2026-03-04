# GUI_MIGRATION_PHASES_v5

v0.5 は「使いにくさの解消」を主目的に、UI構成を再設計するフェーズ。
音声コアの追加最適化ではなく、操作導線と視認性の改善を優先する。

このファイルの扱い:
- v0.5 開発中: 更新対象
- v0.5 完了後: `docs/archive/gui-migration/GUI_MIGRATION_PHASES_v5.md` へ移動して凍結

## GUI_V5_PHASE_A_READABILITY_AND_STATUS

Status: DONE

- フォントサイズとUIスケール設定を導入
- Top Barに実行状態バッジ（Idle/Running/Preview/Canceled/Failed）を表示
- 開発用文言（例: `Phase 5: Release Ready`）をUIから除去

完了条件:
- 既定表示で現行より文字が読みやすい
- 現在状態がTop Barで一目で分かる

## GUI_V5_PHASE_B_LAYOUT_REBUILD

Status: DONE

- 画面を `Top/Body/Bottom` の3領域へ再構成
- Bodyを左右分割（左: Project/Channel Editor, 右: Mix/Monitor）
- Logsを下段固定（高さ可変）

完了条件:
- ウィンドウサイズ変更時に主要UIが追従する
- 現行よりスクロール量が減る

## GUI_V5_PHASE_C_FLOW_AND_PATH_UX

Status: DONE

- `Play` / `Play Preview` / `Replay` / `Stop` の導線を整理
- `Browse MIDI...` / `Browse Output...` を追加（手入力依存を低減）
- パス表示を省略表示 + ホバー全文表示 + コピーしやすい形へ改善

完了条件:
- 初見で主要操作の意図が分かる
- パス手入力が必須でなくなる

## GUI_V5_PHASE_D_CHANNEL_UX

Status: DONE

- 16chを常時全展開しないUIへ変更
- 選択チャンネル中心の詳細編集に集約
- Mix一覧は簡易表示（Mute/Solo/Level中心）へ圧縮

完了条件:
- 16ch編集時の視線移動とスクロールが削減される

## GUI_V5_PHASE_E_STABILIZE_AND_RELEASE

Status: DONE

- v0.5向けスモークテストを追加
- README / Architecture / STATUS をv0.5仕様へ更新
- v0.4 -> v0.5 移行手順を `docs/migration_v5.md` に記載

完了条件:
- 日常運用できる品質でv0.5をリリース可能
