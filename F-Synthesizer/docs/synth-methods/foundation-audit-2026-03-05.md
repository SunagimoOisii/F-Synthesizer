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
- 2.2 Parameter Schema 契約: `部分対応（Waveform/Noise/FM/Drum へ拡張）`
- 2.3 Render Contract 契約: `未評価（ConfigLoad対象外）`
- 2.4 Modulation Routing 契約: `部分対応（命名統一 + 旧命名互換。pan未対応）`
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
- 不足:
  - `LoadSource.cpp` の schema 駆動検証は Waveform 中心で、FM/Drum/DrumKit は段階移行中。
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
- 不足:
  - `pan` destination 未対応。
  - 方式固有 destination（例: `fm.index`）の拡張規約がまだ無い。

### 2.5 Voice Lifecycle 契約

- できていること:
  - `SourceRegistry` に `SourceLifecyclePolicy`（retrigger/steal/noteOff/release/one-shot終了）を追加。
  - `ConfigLoad` で `source.lifecycle` の任意宣言を受理し、`SourceKind` 固定値との整合を検証できる。
- 不足:
  - 実レンダ側の挙動（voice steal優先順位やretrigger挙動）の完全一致監査は未実施。

### 2.6 Test Harness 契約

- 現状:
  - 個人運用方針として、重い自動Harnessは導入しない。
  - `check.ps1` + 代表MIDI手動確認で運用する方針へ切替。
- できていること:
  - `OPERATIONS.md` に軽量運用手順を反映済み。

## 3. 未定義項目リスト（実装順）

1. `LoadSource.cpp` の検証を schema 駆動へ段階移行する（FM/Drum/DrumKit）。
2. `pan` destination 対応可否を明記する（採用/非採用の理由を文書化）。
3. 方式固有 destination（例: `fm.index`）の拡張規約を定義する。
4. Voice Lifecycle の実レンダ挙動を契約項目（retrigger/steal/one-shot終了）に照らして監査する。

## 4. 優先実施順（最小）

1. 3章の 1 を完了する（schema 検証移行）
2. 3章の 2〜3 を完了する（modulation 拡張方針）
3. 3章の 4 を完了する（lifecycle挙動監査）

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
