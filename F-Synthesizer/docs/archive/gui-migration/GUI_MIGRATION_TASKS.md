# GUI段階移行タスク

このファイルは `GUI_MIGRATION_PLAN.md` を実装に落とし込むためのチェックリストです。
段階定義は `GUI_MIGRATION_PHASES.md` を参照してください。

このファイルの扱い:
- GUI移行中: 更新対象
- GUI移行完了後: `docs/archive/gui-migration/GUI_MIGRATION_TASKS.md` へ移動して凍結

## 0. 事前準備

- [x] `STATUS.md` に本移行作業を Next Actions として記載
- [x] 影響範囲を確認（`src/SoundGenerate.cpp`, `include/SynthEngine/SynthEngine.h` など）
- [x] 最低限の受け入れ条件を定義（CLIで同等出力が得られる）

影響範囲（Step 1-3想定）:
- `src/SoundGenerate.cpp`（`main()` の責務分離、`Run(config)` 呼び出し）
- `include/SynthEngine/SynthEngine.h`（必要時のみ。原則、既存APIは維持）
- `src/*` の設定ロード追加先（`Config` 系ファイルを新設する場合あり）
- `README.md` / `STATUS.md` / `Architecture.md`（運用・設計ドキュメント更新）

最低限の受け入れ条件:
- 既存相当の設定でCLI実行し、WAV生成が成功する
- `--config` で入力MIDIと出力先を切り替えられる
- `Run(const AppConfig&)` 経由で実行可能
- 既存の合成コア（Parser/Sequencer/SynthEngine/Writer）の責務を維持する

## 1. 設定の構造化（AppConfig導入）

- [x] `AppConfig` 構造体を定義
- [x] MIDI入力パスを `AppConfig` に移行
- [x] WAV出力パスを `AppConfig` に移行
- [x] 16ch `ChannelConfig` を `AppConfig` に集約
- [x] 既存ハードコード値を `DefaultConfig()` に移動
- [x] `SoundGenerate.cpp` が `DefaultConfig()` で動作することを確認

完了条件:
- [x] `main()` の設定値直書きが減り、`AppConfig` 経由で実行できる

## 2. 設定ファイル化（CLI継続）

- [x] `config/default.json` を追加
- [x] JSON読み込み処理を追加（`AppConfig` へ反映）
- [x] `--config <path>` 引数を実装
- [x] 設定ファイル未指定時は `config/default.json` を読む
- [x] 設定ロード失敗時のエラーメッセージを明確化

完了条件:
- [ ] コード編集なしで MIDI / 出力先 / チャンネル設定を変更できる

## 3. 実行コア分離（Run(config)）

- [x] `Run(const AppConfig&)` を実装
- [x] `main()` から合成処理の詳細を分離
- [x] `main()` を「引数解釈 + 設定ロード + Run呼び出し」に整理
- [x] 既存と同等のWAVが生成されることを確認

完了条件:
- [ ] GUI候補から `Run(config)` を直接呼べる構造になっている

## 4. プリセット運用

- [x] `config/base.json` を追加
- [x] `config/presets/frog.json` を追加
- [x] `config/presets/solstice.json` を追加
- [x] 差分上書きルールを実装（base + preset）
- [x] `--preset <name>` 引数を追加

完了条件:
- [x] 楽曲切替が preset 指定だけで可能

## 5. ドキュメント更新

- [x] `README.md` に新しい実行方法を追記
- [x] `STATUS.md` の Current Snapshot / Next Actions を更新
- [x] `Architecture.md` に設定読み込みフローを追記（必要時）
- [x] 例コマンドを記載（`--config`）
- [x] 例コマンドを記載（`--preset`）

完了条件:
- [x] 再開時にドキュメントだけで操作方法が分かる

## 6. 将来GUI導入準備

- [x] GUI要件（最低限）を明文化
- [x] 必須操作（MIDI選択、プリセット選択、出力先、実行）を定義
- [x] GUIから `Run(config)` 呼び出しのインターフェースを固定（`GUI_REQUIREMENTS.md` に契約を記載）

完了条件:
- [x] GUI実装が「表示層追加」で進められる

## 検証コマンド

```powershell
.\scripts\check.ps1 -SkipBuild -SkipRun
.\scripts\check.ps1 -SkipRun
```
