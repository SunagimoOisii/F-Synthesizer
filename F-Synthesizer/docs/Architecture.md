# アーキテクチャ

このファイルを F-Synthesizer のコンパクトなアーキテクチャ正本にします。
開発方針と設計基準は `docs/ArchitectureAssessment.md` に置きます。

## レイヤー構成

```mermaid
flowchart LR
    GUI[GUI]
    APP[App]
    CORE[Core]
    ENGINE[SynthEngine]
    IO[Config / MIDI / IO]

    GUI --> APP --> CORE --> ENGINE
    APP --> IO
```

## モジュール

- `src/gui`, `include/gui`: ImGui UI、プレビュー操作、ピアノロール、ステップシーケンサー、ワークスペース状態。
- `src/app`, `include/app`: CLI/GUI 入口、実行既定値、実行処理、保存・統計フロー。
- `src/core`, `include/core`: レンダリングゲートウェイと共有 AudioBuffer 境界。
- `src/SynthEngine`, `include/SynthEngine`: voice、event、renderer、modulation、smoothing、filter、mix/effects。
- `src/config`, `include/config`: JSON load/save、preset merge、source registry、schema validation。
- `src/midi`, `include/midi`: MIDI 読み込み、解析、イベントパイプライン、シーケンス処理。
- `src/io`, `include/io`: WAV 書き出しと platform path helper。
- `src/synth`, `include/synth`: oscillator と envelope helper。

依存方向は `gui -> app -> core -> SynthEngine` を基本にします。Core のレンダリングコードは
GUI 状態に依存しません。

## 守る境界

- GUI は編集、表示、プレビュー操作を担当し、音生成アルゴリズムを直接実装しない。
- App は CLI/GUI からの実行条件を `AppConfig` と `RenderOptions` にそろえる。
- Core は app から SynthEngine への薄い実行境界に保つ。
- SynthEngine は音生成、voice、modulation、filter、mix/effects の実行時処理に集中する。
- Config は JSON load/save、preset merge、schema validation を担当し、GUI と renderer に同じ契約を重複させない。
- Source capability と schema の共通判断は `SourceRegistry` に集約する。

## 移行後のモデル境界

アーキテクチャ改善中は、既存の `AppConfig` と `GUIState` を急に廃止せず、次のモデルへ段階移行します。

- `ProjectModel`: 保存、preset、config の正本。JSON load/save は最終的にこのモデルを入出力する。
- `EditorModel`: GUI 編集用の状態。GUI 操作感を維持しながら `GUIState` から段階的に移す。
- `RenderConfig`: SynthEngine へ渡す実行用モデル。GUI や保存形式に依存しない render 入力にする。
- `AppConfig`: 当面の CLI/GUI 実行境界。移行中は `ProjectModel` から生成し、既存実行経路との互換を保つ。
- GUI Facade: 既存画面コードから `ProjectModel` / `EditorModel` へアクセスするための中間層。`GUIState` の全面置換を避け、画面ごとに移行する。
- `GUIState` は互換用の集約型として残し、内部は永続状態、画面一時状態、非同期実行状態、ログ状態の base struct に分ける。

## 肥大化リスク

- `include/SynthEngine/SynthEngine.h`: source config、channel config、effect config が集まりやすい公開境界。新規設定を足す前に分割候補を確認する。
- `include/gui/GUIState.h`: 永続状態、画面一時状態、非同期実行状態が集まりやすい。新しい状態を足す前に寿命と保存要否を確認する。
- config load/save/schema: 音源契約の変更が重複しやすい。GUI editor、preset、renderer と同時変更になる場合は設計レビュー対象にする。
- GUI `.inl` 群: 画面単位の追記で巨大化しやすい。音作りロジックや config 契約を含めない。

## 実行フロー

1. CLI または GUI が `AppConfig` を作る。
2. 設定読み込みが `base.json`、任意の preset JSON、明示 config、GUI 編集状態をマージする。
3. MIDI 入力を時間付きの音楽イベントへ解析する。
4. Sequencer が音楽時間を sample 基準の render event へ変換する。
5. `SynthEngine` が source voice をレンダリングし、modulation と common shaping を適用し、channel mix と master effects を処理する。
6. `Writer` が WAV を書き出す。GUI preview も可能な限り同じ実行経路を使う。

## 設定と音源

- プリセットは `config/presets/*.json` に置き、`config/base.json` の上に適用する。
- `config/default.json` は通常のローカル開始点。
- config / preset の保存形式は `format: "projectModel.v1"` と `project` object を持つ。
- `project.channels` は MIDI チャンネルごとの音色設定を表す。
- `source.type` は音源契約を選ぶ。
  - `waveform`: 基本 oscillator、unison、sub oscillator、filter、smoothing、PWM、ring modulation、hard sync、arpeggio。
  - `analog`: waveform 系の減算的音源。drive と drift を持つ。
  - `fm`: algorithm ベースの FM 音源。operator ratio、output level、feedback、modulation しやすい index を持つ。
  - `noise`: white / pink / brown noise と shared filter shaping。
  - `drum`: kick / snare / hat 風の単体 drum voice。
  - `drumkit`: ch10 運用向けの note number map。
- source capability と schema の共通挙動は、GUI や load code へ重複実装せず `SourceRegistry` に集約する。
- 未知の config key は警告にできるが、不正値や未対応 source field は load error にする。

## GUI 契約

- UI は短い編集→試聴サイクルを最優先する。
- 主導線は「遊ぶ / 作る / 書き出す」。
- Sound 編集では、初心者向け macro control を preview control の近くに置く。
- 詳細 source control はあってよいが、使える音へ到達する唯一の導線にしない。
- Piano-roll 編集と drum step-sequencer 編集は、preview と export の両方へ反映する。
- GUI 状態は `config/gui_state.json` に保存する。Piano-roll project は `config/piano_roll_project.json` に保存する。
- GUI code は sound synthesis を直接実装せず、app/core のレンダリング経路を呼ぶ。

## 音とレンダリング契約

- 音質判断は `docs/PRODUCT_POLICY.md` に従い、継続開発性を落とさない範囲で「気持ちよい音」と短い feedback を優先する。
- source 非依存の modulation と shared shaping は common engine path に置く。
- source 固有の挙動は、その音源方式に本当に必要な場合だけ該当 renderer に置く。
- Master effects は固定順で処理する: sample-rate reducer、bit crusher、chorus、flanger、delay、reverb。
- 既存 preset の音は、ユーザーが明示的に破壊的変更を受け入れない限り安定させる。

## ドキュメント方針

このファイルは、以前分割されていた architecture、synth-method、GUI、sound-parameter、status、
decision 系ドキュメントの要点を置き換えます。短く、現在の状態に合わせて保ちます。
履歴の詳細は新しい archive ファイルではなく Git 履歴から参照します。
