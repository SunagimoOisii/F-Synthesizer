# 開発方針と設計基準

このファイルを、F-Synthesizer の開発方針と設計判断基準の正本にします。
`docs/Architecture.md` は現在の構造、`docs/PRODUCT_POLICY.md` はプロダクト判断、`docs/OPERATIONS.md` は検証と運用、`AGENTS.md` は AI エージェント向け作業ルールを扱います。

## 基本方針

- 最優先は、個人開発と AI 支援開発を数か月後も安全に続けられる継続開発性です。
- 音の気持ちよさ、初心者向け導線、短い編集から試聴までの流れは主要価値です。
- 主要価値を守る場合でも、保守不能な構造、検証不能な変更、責務が曖昧な実装は避けます。
- 個人開発の速度は重視しますが、巨大型や巨大ファイルへの直足しで速度を作りません。
- 過剰なプロセス、大規模テスト基盤、長い判断ログは増やしません。

## 設計境界

- GUI は編集、表示、プレビュー操作を担当し、音生成アルゴリズムを直接実装しません。
- App は CLI/GUI からの実行条件を `AppConfig` と `RenderOptions` にそろえます。
- Core は App から SynthEngine への薄い実行境界に保ちます。
- SynthEngine は音生成、voice、modulation、filter、mix/effects の実行時処理に集中します。
- Config は JSON load/save、preset merge、schema validation を担当します。
- MIDI は読み込み、解析、イベントパイプライン、シーケンス処理を担当します。
- IO は WAV 書き出しと platform path helper を担当します。
- Source capability と schema の共通判断は `SourceRegistry` に集約します。

## 肥大化を防ぐ基準

- `include/SynthEngine/SynthEngine.h` は source config、channel config、effect config が集まる公開境界です。新規設定を足す前に、別ヘッダーや helper へ分けられないか確認します。
- `include/gui/GUIState.h` は永続状態、画面一時状態、非同期実行状態が集まりやすい境界です。新しい状態を足す前に、寿命、保存要否、非同期実行との関係を確認します。
- config load/save/schema、GUI editor、renderer、preset を同時に変える作業は大きめの変更として扱います。
- GUI `.inl` 群には画面描画と入力処理だけを置き、音作りロジックや config 契約を重複定義しません。
- 共通 engine behavior は source ごとに重複実装せず、一度だけ追加します。
- source 固有 behavior は、その音源方式に本当に必要な場合だけ残します。

## 大きな変更の扱い

大きめの変更では、コード編集前に次を短く確認します。

- 変更の主な責務が `GUI / App / Core / SynthEngine / Config / MIDI / IO` のどこに属するか。
- 公開型、保存形式、preset、schema、GUI 状態へ影響するか。
- `SynthEngine.h` や `GUIState` へ新しい状態を足す必要があるか。
- 標準 check、runtime smoke、preset check、手動 GUI/音声確認のどれが必要か。
- 既存 preset の音、主要 GUI 導線、CLI 実行に破壊的な影響があるか。

大規模な再設計を行う場合は、先に現在の主要導線とプリセット音の受け入れ基準を固定します。
そのうえで、既存動作を保ちながら公開型、GUI 状態、config 契約、renderer 境界を順に小さくします。

## ドキュメント更新先

- 方針や設計判断基準を変える場合は、このファイルを更新します。
- レイヤー構造、データフロー、設定、GUI、レンダリング、音源契約が変わる場合は `docs/Architecture.md` を更新します。
- プロダクト価値、利用者、対象外、機能判断ルールが変わる場合は `docs/PRODUCT_POLICY.md` を更新します。
- コマンド、依存関係、検証方針、実行時出力が変わる場合は `docs/OPERATIONS.md` を更新します。
- AI エージェントの作業手順や参照順が変わる場合は `AGENTS.md` を更新します。
- 完了済み作業、古い判断、詳細な履歴は新しいログ文書に残さず Git 履歴を参照します。
