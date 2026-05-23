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

## アーキテクチャ改善ロードマップ

このロードマップは、単一 Visual Studio プロジェクトを維持したまま内部構造を再設計するための実行順序です。
新機能追加は行わず、GUI 操作感を守りながら、音源機能追加時の変更範囲を縮めます。
この節で、改善方針、モデル分離、プッシュ順序、非目標を固定します。

### 目標構造

- C++ domain 型を config、GUI、renderer の正本にします。
- `ProjectModel` を保存、preset、config の正本モデルにします。
- `EditorModel` を GUI 編集用の状態にします。
- `RenderConfig` を SynthEngine へ渡す実行用モデルにします。
- `AppConfig` は当面の CLI/GUI 実行境界として維持し、新モデルから生成します。
- `GUIState` は即時置換せず、Facade 経由で `ProjectModel` / `EditorModel` へ段階移行します。

### 非目標

- 新しい音源機能、GUI 機能、preset 制作はこの改善に含めません。
- 複数 `.vcxproj` や静的ライブラリ化は行いません。
- C++ テスト用プロジェクトは追加しません。
- 既存 JSON 形式の読み込み互換は必須にしません。

### プッシュ順序

1. 完了: 設計文書に改善方針、モデル分離、プッシュ順序、非目標を固定する。
2. `SynthEngine.h` から source config、channel config、effects config を別ヘッダーへ移す。
3. waveform、analog、fm、noise、drum、drumkit、psg の source config を音源単位に整理する。
4. 保存、preset、config の正本となる `ProjectModel` を導入し、既存 `AppConfig` への変換を用意する。
5. SynthEngine へ渡す `RenderConfig` を導入し、render 境界を `AppConfig` から段階的に分離する。
6. config load/save を `ProjectModel` 中心へ移す。
7. `config/base.json`、`config/default.json`、preset を新しい JSON 形式へ一括変換する。
8. 既存 `GUIState` の背後に GUI Facade を導入する。
9. `GUIState` を永続状態、画面一時状態、非同期実行状態、ログ状態へ分ける。
10. GUI editor と `SourceRegistry` の capability、default、表示可否、保存対象判断の重複を整理する。
11. PowerShell 検証を拡張し、model 変換、config 読み込み、preset 一括検証、短尺レンダー、無音検出を確認する。
12. GUI から SynthEngine 詳細型への直接依存を減らし、主要導線の手動確認手順を文書化する。

### 受け入れ基準

- 各プッシュはビルド可能な状態で終える。
- 各プッシュで `.\scripts\check.ps1` を実行する。
- config、renderer、preset、音源型を触るプッシュでは `.\scripts\check.ps1 -RunRuntimeSmoke` を実行する。
- preset/config 移行プッシュでは `.\scripts\check_presets.ps1` を必須にする。
- GUI Facade 以降のプッシュでは、「遊ぶ / 作る / 書き出す」、Sound Preview、ピアノロール、ステップシーケンサーを手動確認する。

### プッシュ 1 完了記録

- 改善方針として、単一 Visual Studio プロジェクトを維持し、C++ domain 型を正本にすることを明記済み。
- モデル分離として、`ProjectModel`、`EditorModel`、`RenderConfig`、`AppConfig`、GUI Facade の役割を明記済み。
- プッシュ順序として、設計文書更新から依存整理までの作業順を明記済み。
- 非目標として、新機能追加、複数プロジェクト化、C++ テスト用プロジェクト追加、旧 JSON 互換必須化を除外済み。

## ドキュメント更新先

- 方針や設計判断基準を変える場合は、このファイルを更新します。
- レイヤー構造、データフロー、設定、GUI、レンダリング、音源契約が変わる場合は `docs/Architecture.md` を更新します。
- プロダクト価値、利用者、非目標、機能判断ルールが変わる場合は `docs/PRODUCT_POLICY.md` を更新します。
- コマンド、依存関係、検証方針、実行時出力が変わる場合は `docs/OPERATIONS.md` を更新します。
- AI エージェントの作業手順や参照順が変わる場合は `AGENTS.md` を更新します。
- 完了済み作業、古い判断、詳細な履歴は新しいログ文書に残さず Git 履歴を参照します。
