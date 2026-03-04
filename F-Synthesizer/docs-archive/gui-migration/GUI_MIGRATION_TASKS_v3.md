# GUI_MIGRATION_TASKS_v3

このファイルは `GUI_MIGRATION_PHASES_v3.md` を実装タスクへ落とし込むチェックリストです。

このファイルの扱い:
- v0.3 開発中: 更新対象
- v0.3 完了後: `docs/archive/gui-migration/GUI_MIGRATION_TASKS_v3.md` へ移動して凍結

## Phase A: Mix Engine Base

- [x] `ChannelMixState`（mute/solo/level/pan/gain）構造を追加
- [x] レンダリング時にチャンネル別ゲイン/パンを適用
- [x] solo有効時はsolo対象のみ出力するルールを実装
- [x] 既存config未指定時のデフォルト値を定義
- [x] CLI実行時の後方互換を確認

完了条件:
- [x] GUIとCLIで同じミックスロジックが使われる

## Phase B: Channel Monitor UI

- [x] ch0-15 の `Mute`/`Solo` ボタンを追加
- [x] ch0-15 の `Level` スライダを追加
- [x] ch0-15 の `Pan` スライダを追加
- [x] ch0-15 の `Gain` スライダを追加
- [x] peakメータ表示を追加
- [x] クリップ警告表示を追加

完了条件:
- [x] ミックス調整をGUIだけで完結できる

## Phase C: Audition Workflow

- [x] `Solo Preview` ボタンを追加（選択chのみ一時solo）
- [x] 再生/停止のUIを追加
- [x] 実行中UIロック範囲を定義
- [x] 単体試聴解除時の状態復帰を実装
- [x] 連続試聴時のログ/状態遷移を安定化

完了条件:
- [x] チャンネル単体の試聴を短時間で繰り返せる

## Phase D: State Persistence

- [x] `gui_state.json` にミックス状態を保存
- [x] プリセット保存時にチャンネルミックス情報を反映
- [x] GUIプリセット切替時に `channels` をGUI状態へ適用
- [x] GUIプリセット切替時に `midiPath/wavPath` との同時反映を保証
- [x] GUI/CLI でプリセット適用結果（channels含む）が一致することを確認
- [x] 旧プリセット読込時の自動補完を実装
- [x] 保存/読込時のバリデーションを追加
- [x] 不正値読み込み時のフォールバック表示を追加

完了条件:
- [x] 再起動後もミックス状態が再現される
- [x] GUIだけでプリセット変更後の音色差分を確認できる

## Phase E: Stabilize & Release

- [x] `scripts/gui_smoke.ps1` をv0.3仕様へ拡張
- [x] mute/solo/level/pan/gain の最小テストを追加
- [x] 単体試聴フローの回帰テストを追加
- [x] README更新（新UI操作手順）
- [x] Architecture更新（ミックス処理/状態保存）
- [x] STATUS更新（Current Snapshot/Next Actions）
- [x] v0.2 -> v0.3 移行手順を `docs/migration_v3.md` に記載

完了条件:
- [x] v0.3を日常運用できる品質でリリース可能

## 先行着手（推奨順）

1. Phase A の solo/mute ロジック確定
2. Phase B の Mute/Solo + Level 先行実装
3. Phase C の Solo Preview 導線を追加
