# PIANO_ROLL_DIRECT_INTERACTION_TASKS

このドキュメントは `docs/PIANO_ROLL_DIRECT_INTERACTION_PHASES.md` の実装チェックリスト。

## Global Rules

- [ ] `gui -> app -> core` の依存方向を維持する
- [ ] `Run` / `AppConfig` の公開I/F互換を維持する
- [ ] 既存の Preview / Export の結果互換を維持する
- [ ] コメントは `docs/COMMENT_GUIDELINE.md` に準拠する

## Phase 1: Core Edit Direct

- [x] クリック選択
- [x] ドラッグ移動
- [x] 端ドラッグで長さ変更
- [x] 空白ドラッグ作成
- [x] Shift+空白ドラッグで範囲選択

## Phase 2: Navigation Direct

- [x] ホイール縦スクロール
- [x] Shift+ホイール横スクロール
- [x] Ctrl+ホイール時間ズーム
- [x] マウス位置中心ズーム

## Phase 3: Playback Direct

- [x] ルーラクリックで再生開始位置変更
- [x] Space再生/停止
- [x] Musicタブの再生成ポリシー維持

## Phase 4: Snap and Shortcut Polish

- [x] Snap常設トグル
- [x] QでSnap ON/OFF
- [x] 1/2/3/4で分解能切替
- [x] 現在値は常設トグル表示で確認（オーバーレイなし）

## Phase 5: Legacy UI Minimize

- [x] 低頻度スライダー/詳細設定UIを廃止
- [x] 重複UI削減
- [x] 説明文更新
- [x] 表示行数を空き領域に追従（ログ展開時含む）
- [x] ルーラ列番号の最小間隔を適用して数字つぶれ防止

## Phase 6: User Guide

- [x] `docs/USER_PIANO_ROLL_GUIDE.md` を作成
- [x] 「最初の5分」「基本編集」「ショートカット」「よくあるつまずき」を記載
- [x] `docs/PIANO_ROLL_CONTROLS.md` と操作仕様の整合を確認(ズレがある場合、実装プログラムを優先する)
