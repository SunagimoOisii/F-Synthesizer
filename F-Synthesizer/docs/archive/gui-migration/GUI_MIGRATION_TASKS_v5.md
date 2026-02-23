# GUI_MIGRATION_TASKS_v5

このファイルは `GUI_MIGRATION_PHASES_v5.md` を実装タスクへ落とし込むチェックリストです。

このファイルの扱い:
- v0.5 開発中: 更新対象
- v0.5 完了後: `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v5.md` へ移動して凍結

## Phase A: Readability and Status

- [x] GUIスケール設定（100/125/150%）を追加
- [x] 既定フォントサイズを引き上げる
- [x] Top Bar に状態バッジ（Idle/Running/Preview/Canceled/Failed）を追加
- [x] 開発用文言をUIから除去

完了条件:
- [x] 既定表示で可読性が改善する
- [x] 状態遷移がTop Barだけで把握できる

## Phase B: Layout Rebuild

- [x] 3領域レイアウト（Top/Body/Bottom）へ再構成
- [x] Bodyを左右分割（Project+Editor / Mix+Monitor）
- [x] Logs を下段固定かつ可変高さへ変更
- [x] リサイズ時の比率・最小サイズ制約を定義

完了条件:
- [x] リサイズ時に主要UIが崩れない
- [x] 現行よりスクロール量が減る

## Phase C: Flow and Path UX

- [x] 主要アクション配置を最上段へ集約（Play/Preview/Replay/Stop/Loop）
- [x] `Browse MIDI...` / `Browse Output...` を追加
- [x] パス欄の省略表示 + ホバー全文表示を追加
- [x] 操作補助文（Play と Preview の違い）を追加

完了条件:
- [x] 初見で主要操作の意図が分かる
- [x] パス選択がGUI内で完結できる

## Phase D: Channel UX

- [x] チャンネル一覧を簡易表示に変更（Mute/Solo/Level中心）
- [x] 選択チャンネルの詳細編集パネルを導入
- [x] 詳細項目（Pan/Gain/Source詳細）を折りたたみ化
- [x] 選択chの試聴導線を明確化

完了条件:
- [x] 16ch編集時の操作ステップが削減される
- [x] 視線移動とスクロール量が現行より減る

## Phase E: Stabilize and Release

- [x] `scripts/gui_smoke.ps1` をv0.5仕様へ拡張
- [x] README更新（v0.5操作手順）
- [x] Architecture更新（v0.5画面構成）
- [x] STATUS更新（Current Snapshot/Next Actions）
- [x] `docs/migration_v5.md` を作成

完了条件:
- [x] v0.5を日常運用できる品質でリリース可能
