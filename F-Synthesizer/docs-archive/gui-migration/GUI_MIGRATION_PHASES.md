# GUI_MIGRATION_PHASES

詳細タスクは `GUI_MIGRATION_TASKS.md` を参照してください。

このファイルの扱い:
- GUI移行中: 更新対象
- GUI移行完了後: `docs/archive/gui-migration/GUI_MIGRATION_PHASES.md` へ移動して凍結

## GUI_MIGRATION_PHASE_1_BASELINE

Status: DONE

- Dear ImGui + GLFW + OpenGL3 の起動確認
- `--gui` でGUIウィンドウ表示
- vcpkg 連携の確認

完了条件:
- GUIウィンドウが起動し、閉じられる

## GUI_MIGRATION_PHASE_2_RUN_BRIDGE

Status: DONE

- GUI入力から `AppConfig` を構築
- `Run(config)` をGUIから呼び出し
- 成功/失敗ステータス表示

完了条件:
- GUIからWAV生成まで実行できる

## GUI_MIGRATION_PHASE_3_ASYNC_AND_LOG

Status: DONE

- バックグラウンド実行（UI非ブロック）
- 実行中UIロック + Stopボタン（キャンセル要求ログまで）
- ログパネル表示（`EventStats` / `RenderStats`）

完了条件:
- 実行中の状態がGUIで把握できる

## GUI_MIGRATION_PHASE_4_USABILITY

Status: DONE

- `config/gui_state.json` 保存/復元
- preset選択導線の改善
- 出力上書き/連番トグル

完了条件:
- 単一曲ループ制作をGUI上で反復しやすい

## GUI_MIGRATION_PHASE_5_RELEASE_READY

Status: DONE

- エラーハンドリング整理
- 最小テスト（起動/実行/失敗系）
- README / Architecture / STATUS 更新

完了条件:
- GUI実装が日常運用できる品質に達している
