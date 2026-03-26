# 合成基盤契約（方式横断）

最終更新: 2026-03-26
状態: Frozen（2026-03-08 契約凍結）

## 1. 目的

- 合成方式の追加前に、方式横断で守る契約を固定する。
- 実装順序に依存した設計崩れ（場当たり `if (type == ...)`）を防ぐ。
- Config / GUI / Renderer / Test の更新漏れを減らす。

関連:
- `docs/synth-methods/method-boundaries.md`
- `docs/STATUS.md`

## 2. 基盤契約（必須）

### 2.1 Source Capability 契約

- 各方式は capability を宣言する。
- 最小項目:
  - `hasPitch`
  - `hasAmpEnv`
  - `hasFilterIn`
  - `hasModTargets`
  - `supportsPolyphony`
  - `isOneShot`
- 用途:
  - GUIの表示制御
  - 無効パラメータのロード抑止
  - Rendererでの適用分岐整理

### 2.2 Parameter Schema 契約

- 方式パラメータは schema で定義する（単一の正とする）。
- 必須項目（最小版）:
  - `id`
  - `type`（int/float）
  - `range`（min/max）
  - `default`
- 拡張項目（任意）:
  - `displayName`
  - `type` の追加種別（bool/enum）
  - `smoothable`（スムージング対象可否）
  - `automatable`（将来拡張用）
- 個人運用の現行判断（2026-03-08）:
  - `displayName` / `smoothable` / `automatable` は当面 `非導入`（必要時に追加）。
- 用途:
  - Config load/save の一貫性確保
  - GUI項目の自動整合
  - バリデーションの共通化
- enum 検証の統合方針:
  - 文字列 -> enum の解決は parse 層で行う（不正文字列は即エラー）。
  - 解決後の enum 値ドメイン（許容範囲）は schema で検証する。
  - これにより、`LoadSource` は「解決」と「契約範囲検証」を分離して扱う。

### 2.3 Render Contract 契約

- 1サンプル処理順を固定する。
- 現行順序（2026-03-26）:
  1. voice state 更新
  2. source render（方式固有）
  3. common shaper（filter / drive など共通処理）
  4. modulation 適用（後段乗算）
  5. mix（stereo pan / level/gain）
  6. master effect layer（chorus -> delay -> reverb） / output
- source render 内で「発振に直結する modulation サンプリング（pitch など）」を行うことは許可する。
- waveform/fm の現行マッピング:
  - waveform: `pitchMul` は 2 で参照、`filterCutoff` は 3、`amp` は 4
  - fm: `pitchMul/fm.index` は 2、`amp` は 4

### 2.4 Modulation Routing 契約

- destination 名と単位を固定する。
- 現行 destination（実装済み）:
  - `pitchMul`
  - `amp`
  - `filterCutoffHz`
- `pan` の扱い（2026-03-26 更新）:
  - 現行は `mod destinationとしては非採用`（ConfigLoad で受理しない）。
  - 理由:
    - ステレオレンダは `channelMix.pan`（equal-power）を mix 層で適用する契約に固定したため。
    - voice単位 pan modulation を導入すると、mix層の責務と競合しやすい。
  - 再検討条件:
    - voice pan を導入する場合、`source/render層` と `mix層` の合成規約（優先順位・合成式）を先に明文化する。
- 方式固有 destination の拡張規約（2026-03-08 確定）:
  - 命名: `<sourceKind>.<parameterId>`（例: `fm.index`）。
  - `sourceKind` は `source.type` と同じ小文字名を使う（`fm`, `noise`, `drum` など）。
  - `parameterId` は対象方式の `ParameterSchema.id` と一致させる。
  - 単位は `ParameterSchema` と同じ意味を使う（無次元倍率/Hzなど）。
- 現行実装:
  - `fm.index` を ConfigLoad で受理し、FMレンダで適用する。
  - 受理範囲は `source.type=fm` の `modulation` に限定し、Waveform等では非受理にする。
- 将来拡張の検討メモは `docs-archive/` 側へ保持し、本書では扱わない。

### 2.5 Voice Lifecycle 契約

- すべての方式で次の状態遷移を定義する。
  - `noteOn`
  - `active`
  - `noteOff`
  - `release`
  - `ended`
- 必須仕様:
  - retrigger 挙動
  - voice steal 時の優先順位
  - one-shot の終了条件
- 最小受け皿（Config/Registry）:
  - `SourceRegistry` に `SourceLifecyclePolicy` を定義する。
  - `ConfigLoad` は `source.lifecycle` を任意で受理し、`SourceKind` ごとの固定値と照合する。
  - 最小キー:
    - `retrigger`（`restart` / `stack`）
    - `steal`（`oldest` / `rejectNew`）
    - `noteOffEntersRelease`（bool）
    - `oneShotEndsAutomatically`（bool）

### 2.6 Test Harness 契約

- 個人運用の通常確認（常時）:
  - `check.ps1` が通る
  - 代表MIDI 1件以上で破綻音がない
  - 重大クリップ増加がない
- 追加確認（変更トリガー時のみ）:
  - レンダ挙動を変えた時: AB比較（耳確認 + 必要時 peak/rms/clip）
  - 乱数系を変えた時: 再現性確認（seed固定）
  - 高負荷機能を触った時: CPU負荷の簡易比較

### 2.7 Parameter Smoothing 契約

- 方式横断の適用方針:
  - `waveform`: `適用済み`（amp/pitch/filterCutoff を source 内 smoothing で適用）
  - `fm`: `未適用`（読み込み/保存/GUIで smoothing 項目を持たない）
  - `noise`: `未適用`（読み込み/保存/GUIで smoothing 項目を持たない）
  - `drum` / `drumkit`: `未適用`（one-shot アタック保護のため非適用）
- Config 契約:
  - `source.smoothing` は `source.type=waveform` のみ受理する。
  - `source.type` が `fm/noise/drum/drumkit` の場合、`source.smoothing` はロード時エラーにする（黙殺しない）。
- 安全制約:
  - one-shot 系（drum/drumkit）はアタック破壊を避けるため smoothing 非適用を既定とする。
- 再検討条件:
  - 方式別に「適用対象パラメータ」「適用帯域（attack/release）」「ABでの peak/rms/clip + 聴感」を定義できた場合のみ導入検討する。

## 2.8 監査結果サマリ（2026-03-19）

- 2.1 Source Capability: 対応済み
- 2.2 Parameter Schema: 対応済み（最小版維持）
- 2.3 Render Contract: 変更トリガー時のみ確認
- 2.4 Modulation Routing: 対応済み（`fm.index` phase 1）
- 2.5 Voice Lifecycle: 対応済み（retrigger/steal/one-shot 一致）
- 2.6 Test Harness: 個人運用の軽量手順へ圧縮
- 2.7 Parameter Smoothing: 対応済み（waveform限定 + 非対応方式はロード拒否）

履歴監査（アーカイブ）:
- `docs-archive/synth-methods/foundation-audit-2026-03-05.md`

### 2.9 監査トリガー（個人開発用）

- `SourceRegistry` / `ConfigLoad` / `LoadSource` を変更した時:
  - 2.1 / 2.2 / 2.4 / 2.5 を確認する
- `Events` / `Voices` / `Renderer` を変更した時:
  - 2.3 / 2.5 / 2.6 を確認する
- GUI の source 編集・変調導線を変更した時:
  - 2.1 / 2.4 の利用整合のみ確認する
- 上記に該当しない変更:
  - foundation監査は省略可

## 3. 方式追加時チェック

1. `method-boundaries.md` で責務境界を先に確定する。
2. 本契約の 2.1〜2.7 を埋める（未定義を残さない）。
3. 方式別 `md` を更新する。
4. `STATUS.md` と必要時 `DECISIONS.md` を更新する。

## 4. 非目標

- 方式ごとの詳細アルゴリズム設計を本書で定義しない。
- UI文言やプリセット方針の全体設計を本書で代替しない。

## 5. 凍結後の更新条件

- 本書の更新は次の場合のみ許可する:
  - 新方式追加
  - 既存契約違反の修正
  - 監査トリガーと運用ルールの明確化
- 更新時は `DECISIONS.md` に判断理由を残す。
- 詳細監査ログと検討メモは `docs-archive/` へ保管する。
