# GUI Architecture

最終更新: 2026-02-25

## Structure

```mermaid
flowchart TD
    Main[GUIMain.cpp]
    TopBar[main/TopBar.inl]
    MainWin[main/MainWindow.inl]
    Loop[main/RunLoop.inl]
    Piano[GUIPianoRoll.cpp]
    Tempo[pianoroll/PianoRollTempo.inl]
    Edit[pianoroll/PianoRollEdit.inl]
    Render[pianoroll/PianoRollRender.inl]
    Input[pianoroll/PianoRollInput.inl]

    Main --> TopBar
    Main --> MainWin
    Main --> Loop
    MainWin --> Piano
    Piano --> Tempo
    Piano --> Edit
    Piano --> Render
    Piano --> Input
```

## UI Interaction Map

```mermaid
flowchart LR
    User[User Input]
    Hit[Hit Test]
    EditOp[Edit Operation]
    State[GUIState]
    Draw[Render]

    User --> Hit --> EditOp --> State --> Draw
```

## Main Split

| File | Role |
|---|---|
| `src/gui/main/TopBar.inl` | フォント初期化、UI倍率、Status表示 |
| `src/gui/main/MainWindow.inl` | メインUI描画、保存導線、エラー導線、ログ表示 |
| `src/gui/main/RunLoop.inl` | ウィンドウ初期化、ImGuiフレームループ、終了処理 |

## Piano Roll Split

| File | Role |
|---|---|
| `src/gui/GUIPianoRoll.cpp` | `DrawPianoRollPanel` のエントリ |
| `src/gui/pianoroll/PianoRollTempo.inl` | tick/sec変換、Snap補助 |
| `src/gui/pianoroll/PianoRollEdit.inl` | ノート編集、Undo/Redo、選択、モデル同期 |
| `src/gui/pianoroll/PianoRollRender.inl` | グリッド/ノート描画 |
| `src/gui/pianoroll/PianoRollInput.inl` | ヒットテスト、ドラッグ更新 |

## GUI State

| 区分 | File |
|---|---|
| 定義 | `include/gui/GUIState.h` |
| 永続化 | `src/gui/GUIStateStorage.cpp`, `src/gui/GUIStatePersistence.cpp` |
| 補助 | `src/gui/GUIActions.cpp`, `src/gui/GUIRunHelpers.cpp`, `src/gui/GUIConfigUtils.cpp` |

## GUI実装確認ポイント

| 観点 | 確認点 | 参照 |
|---|---|---|
| 画面責務 | メイン画面は `main/*.inl`、ピアノロールは `pianoroll/*.inl` に分割される | `src/gui/main/`, `src/gui/pianoroll/` |
| 状態保持 | 状態定義は `GUIState.h`、永続化は `GUIStateStorage/Persistence` が担当する | `include/gui/GUIState.h`, `src/gui/GUIStateStorage.cpp`, `src/gui/GUIStatePersistence.cpp` |
| 編集履歴 | Undo/Redoは `PianoRollEdit.inl` で管理し、上限は `maxUndoCommands` で制御する | `src/gui/pianoroll/PianoRollEdit.inl`, `include/gui/GUIPianoRoll.h` |

変更影響の確認先は `docs/architecture/README.md` の `Impact Map（変更時の影響先）` を参照。

## Special Notes

### GUI操作・状態管理

#### 2026-02-25: Preview再生コールバックはロックなしで進行し、PCM差し替え時のみ排他
- カテゴリ: GUI操作・状態管理
- 背景: オーディオコールバック内でロック待ちが発生すると、再生途切れの原因になる。
- 判断: コールバックでは `frameCursor`/`playing` をatomicで扱い、`PlayPreviewAudio` 側でPCM書換時のみmutexを使用。
- 代替案: 常時mutex保護して整合性を優先する案。
- 影響範囲: 低遅延再生時の安定性向上。実装はatomic状態管理を前提に複雑化。
- 関連ファイル: `src/gui/PreviewAudio.cpp`, `include/gui/PreviewAudio.h`

#### 2026-02-25: Piano Roll Undo履歴を64件に上限化
- カテゴリ: GUI操作・状態管理
- 背景: ノート編集履歴を無制限に保持すると、長時間編集でメモリ使用量が増え続ける。
- 判断: `maxUndoCommands = 64` を既定値として、超過時は最古履歴を破棄。
- 代替案: 無制限保持または差分圧縮方式の履歴管理。
- 影響範囲: メモリ増加を抑制。極端に長い操作列では古いUndoが失われる。
- 関連ファイル: `include/gui/GUIPianoRoll.h`, `src/gui/pianoroll/PianoRollEdit.inl`

ADR記法は `docs/architecture/README.md` の `ADR Card Template` を使用。

#### 2026-02-26: 音源別編集UIを `GUIChannelEditor` に集約
- カテゴリ: GUI操作・状態管理
- 背景: 音源方式ごとの編集項目が複数箇所に分散すると、機能追加時の反映漏れとUX不整合が起きやすい。
- 判断: Source切替に応じた編集UIは `GUIChannelEditor` に集約し、Sound編集の入口を一本化する。
- 代替案: Sourceごとに別画面・別編集関数へ分散する案。
- 影響範囲: 実装変更時の追跡性が向上し、外部デモ時に「編集導線が一貫している」と説明しやすくなる。
- 関連ファイル: src/gui/GUIChannelEditor.cpp


#### 2026-03-03: 描画と副作用処理の責務を分離
- カテゴリ: GUI操作・状態管理
- 背景: 描画コードに実行/保存/復旧ロジックを混在させると、画面変更時に挙動回帰を招きやすい。
- 判断: 描画は `MainWindow.inl`、操作副作用は `GUIActions.cpp`、パラメータ編集は `GUIChannelEditor.cpp` へ分離する。
- 代替案: 1ファイルで描画と操作を一体管理する案。
- 影響範囲: レビュー時に見た目変更と挙動変更を分離でき、回帰原因の切り分けが容易になる。
- 関連ファイル: src/gui/GUIActions.cpp, src/gui/GUIChannelEditor.cpp, src/gui/main/MainWindow.inl


#### 2026-03-04: GUI保存・復旧の契約をヘッダ境界で固定
- カテゴリ: GUI操作・状態管理
- 背景: 実行中断・保存失敗・復旧導線の契約が曖昧だと、呼び出し側ごとにエラー処理が分岐して保守負荷が増える。
- 判断: `GUIActions/GUIRunHelpers/GUIStatePersistence/GUIStateStorage` の公開関数契約を明示し、状態遷移を共通化する。
- 代替案: 呼び出し側で都度ローカル処理を追加する案。
- 影響範囲: 運用時の復旧導線（Recover操作）を統一し、将来の機能追加でも例外処理の再利用性が高まる。
- 関連ファイル: include/gui/GUIActions.h, include/gui/GUIRunHelpers.h, include/gui/GUIStatePersistence.h, include/gui/GUIStateStorage.h, src/gui/GUIActions.cpp


#### 2026-03-05: ホバー説明の表示契約を `影響/注意` 優先に統一
- カテゴリ: GUI操作・状態管理
- 背景: `Help` 文言がラベル説明と重複すると情報密度が下がり、ユーザーが操作差分を把握しづらい。
- 判断: `composeHoverHelp/updateHoverHelp` で `影響` と `注意` を主表示にし、Top Controls 全体で同一フォーマットを適用する。
- 代替案: 各UI要素で自由形式のヘルプ文を維持する案。
- 影響範囲: 文言設計の一貫性が向上し、操作ガイドを外部ドキュメントへ転用しやすくなる。
- 関連ファイル: src/gui/main/MainWindow.inl

