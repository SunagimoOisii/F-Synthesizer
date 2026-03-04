# GUI段階移行プラン

## 目的

- 現在の `SoundGenerate.cpp` の手動編集運用から脱却する
- いきなりGUI化せず、段階的に移行してリスクを下げる
- 将来GUIを載せるときに、既存合成ロジックをほぼ再利用できる状態にする

## 基本方針

- 合成エンジン（MIDI解析/Sequencer/SynthEngine/Writer）は極力そのまま
- 操作面（入力切替、チャンネル設定、出力先指定）を外出し
- 先にCLI + 設定ファイルで運用改善し、最後にGUIを追加する

## 段階移行プラン

### 1. 設定の構造化

- `AppConfig` を導入
  - MIDI入力パス
  - WAV出力パス
  - 16ch の `ChannelConfig`
  - 追加オプション（対象チャンネル、デフォルト波形など）
- 既存ハードコード値は `DefaultConfig()` に集約

### 2. 設定ファイル化（中間段階の中心）

- `config/default.json` を追加
- `--config <path>` で読み込み
- 楽曲別に `config/presets/*.json` を分離
- まずはCLIのまま使い、コード編集なしで切替可能にする

### 3. 実行コア分離

- `Run(const AppConfig&)` を作成
- `main()` は以下だけに限定
  - 引数解釈
  - 設定読み込み
  - `Run(config)` 呼び出し
- GUI化時は `Run(config)` をそのまま再利用

### 4. プリセット運用の整備

- 共通設定 + 差分上書き方式
- 例:
  - `config/base.json`
  - `config/presets/frog.json`
  - `config/presets/solstice.json`
- チャンネル設定の重複を減らし保守しやすくする

### 5. GUI導入

- GUIは `AppConfig` を編集して `Run(config)` を呼ぶだけにする
- 先に最小UIから開始
  - MIDI選択
  - プリセット選択
  - 出力先選択
  - 実行ボタン

## マイルストーン（最小）

1. `AppConfig` 導入
2. `config/default.json` + `--config` 対応
3. `Run(config)` 分離
4. プリセット追加
5. GUI着手

## 完了条件（各段階）

- 段階1: `SoundGenerate.cpp` 直編集なしで基本設定が管理できる
- 段階2: MIDI/プリセット切替が設定ファイルのみで可能
- 段階3: 実行ロジックが `Run(config)` 経由に統一
- 段階4: 主要楽曲のプリセットがファイル化済み
- 段階5: GUIから `Run(config)` 実行で同等音声が得られる

## 期待効果

- 手動編集ミスの削減
- 再開時の認知負荷軽減
- GUI実装の工数とリスクを分散
- 既存エンジン資産の最大活用

