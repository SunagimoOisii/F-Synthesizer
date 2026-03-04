# gui-help Phase Plan

Last Updated: 2026-03-05
Owner: `gui-help`

## Goal

- ホバー時の `Help:` 行で、誤操作が起きやすい UI を優先して説明する
- 既存 UI 契約（`GUI_REQUIREMENTS.md`）と矛盾しない文言に統一する

## Operation Rules

1. 本ファイルは `gui-help` の運用台帳として継続更新し、完了フェーズも削除しない
2. 未対応項目は必ず `STATUS_DETAIL.md` の `Backlog` と同期する
3. 実装ルールの正本は本ファイルとし、`updateHoverHelp(what, impact, caution)` を標準とする
4. `gui-help` 一巡後は要点を `GUI_REQUIREMENTS.md` に昇格し、本ファイルは `docs-archive/` へ移管する
5. 更新時は `Last Updated` と対象ファイル（実装/ドキュメント）を Progress 節に明記する

## Phase Split

1. Phase 1: 対象棚卸しと優先度確定（本ドキュメント）
2. Phase 2: 共通ヘルプ導線整理（`updateHoverHelp` 呼び出しパターン統一）
3. Phase 3: 実装A（Music の誤操作高リスク領域）
4. Phase 4: 実装B（残り主要導線）
5. Phase 5: 検証とドキュメント同期
6. Phase 6: Sound残項目とエラーダイアログ周辺の補完
7. Phase 7: `DrawChannelEditor(state)` 内部UIへのヘルプ展開
8. Phase 8: 手動ホバー受け入れ確認と残件クローズ
9. Phase 9: `GUI_REQUIREMENTS.md` への昇格と `docs-archive/` への移管

## Phase 1 Deliverable (Coverage Table)

Priority:
- `A`: 誤操作が出力/再生結果に直結
- `B`: 迷いやすいが破壊的ではない
- `C`: 補助説明

| Area | UI Item | Priority | Hover Help Draft |
|---|---|---:|---|
| Music / Path | `MIDI Path` | A | 読み込むMIDIファイルです。Play Preview/Export WAV の入力に使います。 |
| Music / Path | `Browse MIDI...` | B | MIDIファイルを選択します。無効パス時の復旧導線です。 |
| Music / Path | `Copy MIDI` | C | 現在のMIDIパスをクリップボードへコピーします。 |
| Music / Path | `Output Path` | A | 書き出し先WAVパスです。Export WAV の出力先になります。 |
| Music / Path | `Browse Output...` | B | 出力先WAVパスを選択します。 |
| Music / Path | `Copy Output` | C | 現在の出力パスをクリップボードへコピーします。 |
| Music / Reference | `Snapshot (Recommended)` | A | 書き出し時に音色設定を固定し、再現性を優先します。 |
| Music / Reference | `Link (Advanced)` | A | 最新のSound編集を参照します。後の音色変更で書き出し結果が変わります。 |
| Music / Target | `All Channels` | A | 全MIDIチャンネルを出力します。PR Channel の表示先とは独立です。 |
| Music / Target | `Single Channel` | A | 指定chのみ出力します。PR Channel と自動連動しません。 |
| Music / Target | `Target Ch` | A | Single Channel 出力対象です。プレビュー表示chの切替とは別設定です。 |
| Music / Mixer | `Set PR Assign = Same slot index` | B | 表示中PR chの割当スロットを同じ番号へそろえます。 |
| Music / Mixer | `Set Output Target = PR ch` | A | Output Target を現在のPR chに合わせます。明示操作時のみ変更されます。 |
| Music / Mixer | `Reset All Assign = Same slot index` | B | 全chの割当を ch番号=slot番号 に戻します。 |
| Music / Drum | `Enable ch10 Drum Guard` | A | ch10をドラム運用として監視し、非ドラム割当を見つけやすくします。 |
| Music / Drum | `Auto Setup ch10 Drum` | A | ch10をドラム向け初期割当に自動調整します。 |
| Music / Drum | `Focus PR ch10` | B | ピアノロール表示chをch10へ切り替えます。 |
| Music / Table | `slot` | A | MIDI chごとの Sound Slot 割当です。音色の鳴り方を直接変えます。 |
| Music / Table | `M` | A | Mute。該当chを無音化します。 |
| Music / Table | `S` | A | Solo。Solo対象以外のchを抑制します。 |
| Music / Table | `Level` | A | チャンネル音量の基本レベルです。 |
| Music / Table | `Pan` | B | 左右定位を調整します（-1.0 左 / +1.0 右）。 |
| Music / Table | `Gain` | A | 追加ゲインです。上げすぎるとクリップしやすくなります。 |
| Music / Render | `Sample Rate` | B | 出力サンプルレートです。高いほど負荷と容量が増えます。 |
| Music / Render | `Bits` | B | 出力ビット深度です。互換性と容量のバランスに影響します。 |
| Music / Render | `Extra Release (sec)` | B | ノート終端後の余韻を追加します。尻切れ防止用です。 |
| Music / Render | `Serial Save (timestamp suffix)` | C | ファイル名に時刻サフィックスを付け、上書きを避けます。 |

## Done Criteria For Phase 1

- `MainWindow.inl` の Music 主要導線に対する対象一覧と文言案が確定している
- Priority `A` 項目は全て文言が作成済み
- 文言は「何をする / 何に影響する」を 1-2 文で説明している

## Phase 2 Rule (Common Hover Help Path)

- `MainWindow.inl` では `composeHoverHelp(what, impact, caution)` + `updateHoverHelp(what, impact, caution)` を共通導線として使う
- UI項目追加時は必ず次の順序で実装する
  1. UI項目を描画する
  2. 直後に `updateHoverHelp(what, impact, caution)` を呼ぶ
- `caution` は必要時のみ指定し、省略時は「何をする / 影響」の2要素で統一する
- 文言順序は常に `何をする -> 影響 -> 注意` とする

## Phase 3 Progress (2026-03-05)

- `src/gui/main/MainWindow.inl` の Music セクションで Priority `A` 項目のヘルプを追加
- 対象: `MIDI/Output Path`, `Snapshot/Link`, `All/Single/Target Ch`,
  `PR Channel summary`, `Set Output Target = PR ch`, `Enable/Auto Setup ch10`,
  Mixerテーブルの `slot/M/S/Level/Pan/Gain`

## Phase 4 Progress (2026-03-05)

- `MIDI/Output Path` の入力/Browse/Copyに補助説明と注意点を追加
- `Render Settings` の主要項目に意図説明を追加
  - `Sample Rate`
  - `Initial Seconds`
  - `Bits`
  - `Extra Release (sec)`
  - `Serial Save (timestamp suffix)`
- `Play Preview` / `Play Tone` / `Export WAV` / `Stop` の違いを注意文言で明確化

## Phase 5 Verification (2026-03-05)

- 自動確認:
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（回帰なし）
- 手動確認:
  - Sound/Music のホバー文言確認は GUI 対話操作が必要（この実行環境では未実施）
- 未対応項目:
  - Soundタブ詳細編集領域・エラーダイアログ周辺のホバー文言は次タスクとして Backlog 化済み

## Phase 6 Plan (Next)

- 対象:
  - Soundタブ詳細編集領域（Preset操作、Sound Slot編集、主要トグル/スライダ）
  - エラーダイアログ周辺（Recover系ボタン、`Clear Error`、`OK`/`Dismiss`）
  - 未付与の補助ボタン（Music/Sound 共通の残項目）
- 実装方針:
  - Phase 2 Rule に従い、描画直後に `updateHoverHelp(what, impact, caution)` を付与
  - 高リスク操作（保存、上書き、状態遷移）を Priority `A` として先行実装
- Done Criteria:
  - Sound/Music の主要クリック導線でホバー文言欠落がない
  - `STATUS_DETAIL.md` の `Backlog` と本ファイルの未対応項目が一致している
  - `scripts/gui_smoke.ps1 -Profile quick` が通過している

## Phase 6 Progress (2026-03-05)

- 追加実装:
  - Soundタブの Preset 操作（Preset選択/適用/初期化/保存/複製/Sound Slot初期化）
  - Error表示と Errorダイアログ（Recover系ボタン、`Clear Error`、`OK`/`Dismiss`）
  - Unsaved Changes ダイアログ（`保存して続行` / `保存せず続行` / `キャンセル`）
  - Music補助ボタンの未付与分（`Set PR Assign` / `Reset All Assign` / `Focus PR ch10`）
- 検証:
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-05、Phase 6反映後）
- 残項目:
  - `DrawChannelEditor(state)` 内部UIのホバー文言は別ファイル側の実装確認が必要

## Phase 7 Plan (Next)

- 対象:
  - `src/gui/GUIChannelEditor.cpp` の主要操作UI（トグル、スライダ、選択UI）
- 実装方針:
  - Phase 2 Rule に従い、各項目描画直後に `updateHoverHelp(what, impact, caution)` を接続
  - 誤操作時の音色変化が大きい項目を Priority `A` として先行
- Done Criteria:
  - `DrawChannelEditor(state)` 内で主要導線のホバー欠落がない
  - `STATUS_DETAIL.md` の `gui-help` 残タスク記述と整合している

## Phase 7 Progress (2026-03-05)

- 追加実装:
  - `DrawChannelEditor` にホバー更新コールバックを受け渡すインターフェースを追加
  - `MainWindow.inl` から共通 `updateHoverHelp` を `DrawChannelEditor` へ接続
  - `GUIChannelEditor.cpp` の主要UIへホバー文言を追加
    - Sound Slot選択/PR割当編集ボタン
    - Envelope/Amp/ADSR
    - Source Type、Waveform/Filter/Smoothing/Modulation主要項目
    - Noise/FM/DrumKit主要項目
- 検証:
  - `scripts/gui_smoke.ps1 -Profile quick` 通過（2026-03-05、Phase 7反映後）

## Phase 8 Plan

- 対象:
  - Sound/Music 両モードの主要クリック導線全体
- 実施内容:
  - 実機GUIで手動ホバー確認（文言の誤記、長さ、意図不一致を確認）
  - 必要な文言微修正と Backlog の更新
- Done Criteria:
  - 手動ホバー確認結果をドキュメントへ反映済み
  - `gui-help` の残件がゼロ、または非対象理由つきで明示されている

## Phase 9 Plan

- 対象:
  - `docs/GUI_REQUIREMENTS.md`
  - `docs/gui-help-phase-plan.md`
  - `docs/STATUS.md`, `docs/STATUS_DETAIL.md`
- 実施内容:
  - `gui-help` の運用要点・文言規則・実装手順を `GUI_REQUIREMENTS.md` へ昇格
  - 本ファイルを `docs-archive/` 配下へ移管
  - `STATUS` / `STATUS_DETAIL` を `gui-help` 完了状態へ更新
- Done Criteria:
  - 昇格内容と移管先がドキュメント上で追跡可能
  - `gui-help` が Current/Backlog 上でクローズされている
