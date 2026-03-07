# 基盤契約 棚卸し（SourceRegistry / ConfigLoad）

作成日: 2026-03-05
最終更新: 2026-03-08
状態: Draft（更新継続）
対象:
- `include/config/SourceRegistry.h`
- `src/config/SourceRegistry.cpp`
- `src/config/ConfigLoad.cpp`
- `src/config/load/*.cpp`
- 参照契約: `docs/synth-methods/foundation-contract.md`

## 1. 判定サマリ

- 2.1 Source Capability 契約: `対応済み`
- 2.2 Parameter Schema 契約: `部分対応（LoadSource の schema 検証を FM/Drum/DrumKit へ移行）`
- 2.3 Render Contract 契約: `未評価（ConfigLoad対象外）`
- 2.4 Modulation Routing 契約: `部分対応（命名統一 + 旧命名互換。pan非採用方針確定）`
- 2.5 Voice Lifecycle 契約: `部分対応（最小受け皿を定義）`
- 2.6 Test Harness 契約: `運用代替（重い自動Harnessは未導入）`

## 2. 観測結果（契約別）

### 2.1 Source Capability 契約

- できていること:
  - source種別の列挙と typeName 解決は `SourceKind` で一元化されている。
  - `DefaultSourceConfig` により種別ごとの既定値を返せる。
  - `SourceCapability` が導入され、GUIのドラム判定に capability 利用が入っている。
- 不足:
  - capability ベース適用は一部のみで、全方式の分岐置換は継続課題。

### 2.2 Parameter Schema 契約

- できていること:
  - `SourceParameterSchemaEntry` が導入され、`SourceKind -> ParameterSchema[]` は Waveform/Noise/FM/Drum まで定義済み。
  - DrumKit は可変 `note -> DrumConfig` 構造のため、`ParameterSchema[]` は空定義（構造上の特例）とした。
  - `LoadSource.cpp` の schema 駆動検証は Waveform/FM/Drum/DrumKit（各noteのDrumConfig）まで移行済み。
  - Noise enum は「parse で文字列解決 + schema で値ドメイン検証」の分離方針で統合済み。
- 不足:
  - `displayName` / `smoothable` / `automatable` は未導入。

### 2.3 Render Contract 契約

- 備考:
  - Render順序契約は Renderer/Voices 側で評価すべきで、SourceRegistry/ConfigLoadのみでは判定できない。
  - 本棚卸しでは対象外とする。

### 2.4 Modulation Routing 契約

- できていること:
  - 保存時の destination 命名は `pitchMul` / `amp` / `filterCutoffHz` に統一済み。
  - 旧命名 `pitch` / `filterCutoff` は読込互換を維持。
  - route数は固定（8）で、index 範囲外を検出できる。
  - `pan` は現時点で非採用とし、ConfigLoad 非受理にする方針を文書化済み。
  - 方式固有 destination の命名規約（`<sourceKind>.<parameterId>`、例: `fm.index`）を定義済み。
- 不足:
  - 拡張規約は定義済みだが、`fm.index` などの受理・適用実装は未着手。

### 2.5 Voice Lifecycle 契約

- できていること:
  - `SourceRegistry` に `SourceLifecyclePolicy`（retrigger/steal/noteOff/release/one-shot終了）を追加。
  - `ConfigLoad` で `source.lifecycle` の任意宣言を受理し、`SourceKind` 固定値との整合を検証できる。
  - 実レンダ実装を監査し、契約との差分を特定した。
- 監査結果（2026-03-08）:
  - retrigger:
    - 実装: `SourceLifecyclePolicy` が restart の方式（`Waveform/Noise/FM`）は同一 note の既存voiceを再初期化し、積み増ししない。
    - 判定: `Waveform/Noise/FM` は契約（restart）と一致。`Drum/DrumKit` は契約（stack）と一致。
  - noteOff -> release:
    - 実装: 非Drumは `MarkNoteOff` で `NoteOff` 実行、Drumは `PrepareDrumRelease` で自動 `NoteOff`。
    - 判定: 契約意図と概ね一致。
  - one-shot 終了条件:
    - 実装: Drumは `drumTime >= attack+decay` で自動release、`ADSRStage::Off` 後に `pendingRemove` で削除。
    - 判定: 契約意図と一致。
  - voice steal:
    - 実装: voice上限/steal処理が無く、`Oldest/RejectNew` の優先順位は未適用。
    - 判定: 契約の「steal優先順位」は未実装（評価不能）。
- 不足:
  - voice上限と steal 優先順位（`Oldest` / `RejectNew`）の実装が未着手。

### 2.6 Test Harness 契約

- 現状:
  - 個人運用方針として、重い自動Harnessは導入しない。
  - `check.ps1` + 代表MIDI手動確認で運用する方針へ切替。
- できていること:
  - `OPERATIONS.md` に軽量運用手順を反映済み。

## 3. 未定義項目リスト（実装順）

1. 方式固有 destination（例: `fm.index`）の受理・適用実装を行うか判断し、採用時は段階導入する。
2. voice上限と steal 優先順位（`Oldest/RejectNew`）を `SourceLifecyclePolicy` に沿って実装する。

## 4. 優先実施順（最小）

1. 3章の 1 を完了する（modulation destination 拡張の実装判断）
2. 3章の 2 を完了する（lifecycle 実装一致）

## 5. タスク完了後の凍結手順

1. 再監査:
   - 本書の 2.1〜2.6 を再評価し、`未対応/部分対応` が残らないことを確認する。
2. 最終判定:
   - 判定サマリを最終版へ更新し、完了日を追記する。
3. 状態固定:
   - ファイル名を維持したまま冒頭へ `状態: Frozen` を追記する。
4. 参照固定:
   - `foundation-contract.md` 側の関連リンクから本書を「完了監査」として参照する。
5. 変更制限:
   - 凍結後は誤記修正のみ許可し、内容変更が必要な場合は新しい監査ファイルを日付付きで追加する。

## 6. Foundationタスク対象プログラムファイル

本監査で対象とする実装ファイルは `foundation-contract.md` の「6. Foundationタスク対象プログラムファイル」を正とする。
監査観点ごとの主要対象は以下。

- 契約定義:
  - `include/config/SourceRegistry.h`
  - `src/config/SourceRegistry.cpp`
- Config整合:
  - `src/config/ConfigLoad.cpp`
  - `src/config/load/Internal.h`
  - `src/config/load/LoadSource.cpp`
  - `src/config/load/LoadModulation.cpp`
  - `src/config/ConfigJSONUtils.cpp`
- 実挙動監査:
  - `src/SynthEngine/Events.cpp`
  - `src/SynthEngine/Voices.cpp`
  - `src/SynthEngine/Renderer.cpp`

## 7. Foundationタスク一覧（棚卸し）

### 7.1 契約2.1〜2.6タスク

1. `2.1 Source Capability`
   - 状態: 対応済み（部分適用課題あり）
   - 残: capability ベース分岐の全面適用
2. `2.2 Parameter Schema`
   - 状態: 部分対応
   - 完了: Waveform/Noise/FM/Drum schema、DrumKit特例、LoadSource schema検証移行、Noise enum統合方針
   - 残: `displayName` / `smoothable` / `automatable` 導入
3. `2.3 Render Contract`
   - 状態: 未評価（本監査対象外）
   - 残: Renderer/Voices 側で契約監査
4. `2.4 Modulation Routing`
   - 状態: 部分対応
   - 完了: 命名統一、旧名互換、`pan` 非採用方針、`<sourceKind>.<parameterId>` 規約定義
   - 残: 方式固有 destination の受理/適用実装判断
5. `2.5 Voice Lifecycle`
   - 状態: 部分対応
   - 完了: `SourceLifecyclePolicy` + `source.lifecycle` 整合検証、実レンダ監査
   - 残: steal の実装一致
6. `2.6 Test Harness`
   - 状態: 運用代替で整理済み
   - 完了: 重い自動Harness非採用、`check.ps1` + 代表MIDI手動確認運用

### 7.2 現行の残タスク（実装/判断）

1. 方式固有 destination（例: `fm.index`）の受理・適用実装を行うか判断し、採用時は段階導入する。
2. voice上限と steal 優先順位（`Oldest/RejectNew`）を `SourceLifecyclePolicy` に沿って実装する。

### 7.3 凍結時タスク（最終クローズ）

1. 2.1〜2.6 を再監査し、`未対応/部分対応` を解消する。
2. 判定サマリを最終版へ更新し、完了日を追記する。
3. 冒頭の状態を `Frozen` に更新する。
4. `foundation-contract.md` から本書を「完了監査」として参照固定する。
5. 凍結後の変更は誤記修正のみとし、内容変更は新しい監査ファイルを日付付きで追加する。
