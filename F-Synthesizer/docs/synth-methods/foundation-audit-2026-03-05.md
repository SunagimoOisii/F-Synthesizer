# 基盤契約 棚卸し（SourceRegistry / ConfigLoad）

作成日: 2026-03-05
対象:
- `include/config/SourceRegistry.h`
- `src/config/SourceRegistry.cpp`
- `src/config/ConfigLoad.cpp`
- `src/config/load/*.cpp`
- 参照契約: `docs/synth-methods/foundation-contract.md`

## 1. 判定サマリ

- 2.1 Source Capability 契約: `未対応`
- 2.2 Parameter Schema 契約: `部分対応`
- 2.3 Render Contract 契約: `未評価（ConfigLoad対象外）`
- 2.4 Modulation Routing 契約: `部分対応`
- 2.5 Voice Lifecycle 契約: `未対応（ConfigLoad対象外）`
- 2.6 Test Harness 契約: `未対応（ConfigLoad対象外）`

## 2. 観測結果（契約別）

### 2.1 Source Capability 契約

- できていること:
  - source種別の列挙と typeName 解決は `SourceKind` で一元化されている。
  - `DefaultSourceConfig` により種別ごとの既定値を返せる。
- 不足:
  - `hasPitch` / `hasFilterIn` / `supportsPolyphony` / `isOneShot` など capability 宣言が無い。
  - GUI表示制御・Load時の無効項目拒否を capability ベースで行う仕組みが無い。

### 2.2 Parameter Schema 契約

- できていること:
  - `LoadSource.cpp` / `LoadModulation.cpp` に個別パラメータの parse・validate がある。
  - Waveform は範囲検証が比較的明確（unison/filter/mod/smoothing）。
- 不足:
  - schema の単一定義（`id/type/range/default/smoothable`）が存在しない。
  - 検証ロジックが方式別に分散し、定義重複の温床になっている。
  - FM/Drum は「必須項目チェック中心」で、数値レンジ検証が薄い。

### 2.3 Render Contract 契約

- 備考:
  - Render順序契約は Renderer/Voices 側で評価すべきで、SourceRegistry/ConfigLoadのみでは判定できない。
  - 本棚卸しでは対象外とする。

### 2.4 Modulation Routing 契約

- できていること:
  - destination は `Pitch/Amp/FilterCutoff` に限定され、未知値は parse error になる。
  - route数は固定（8）で、index 範囲外を検出できる。
- 不足:
  - 契約が求める命名/単位（`pitchMul`, `filterCutoffHz`, `pan`）との差分がある。
  - `pan` destination 未対応。
  - 方式固有 destination（例: `fm.index`）の拡張規約がまだ無い。

### 2.5 Voice Lifecycle 契約

- 不足:
  - SourceRegistry/ConfigLoad では noteOn/noteOff/retrigger/steal/one-shot終了条件を定義していない。
  - lifecycle 契約の受け皿（設定キー・宣言）も未整備。

### 2.6 Test Harness 契約

- 不足:
  - ConfigLoad 追加変更時に共通で走る契約テスト（再現性/clip率/無音）が紐づいていない。
  - source type 追加時の必須テストチェックリストがコード近傍に無い。

## 3. 未定義項目リスト（実装順）

1. `SourceCapability` 構造体を追加し、`SourceKind` ごとの capability を定義する。
2. `SourceKind -> ParameterSchema[]` の単一テーブルを追加する。
3. `LoadSource.cpp` の検証を schema 駆動へ段階移行する（少なくとも range/default の一元化）。
4. modulation destination を契約命名へ寄せる方針を決める（互換 alias を許可するか含む）。
5. `pan` destination 対応可否を明記する（採用/非採用の理由を文書化）。
6. FM/Drum の数値レンジ検証ポリシーを追加する（必須キーのみ運用を終了）。
7. source type 追加時の「契約2.1〜2.6チェック」テンプレートを PR チェックリスト化する。
8. 共通 Test Harness 最小セット（無音/再現性/clip率/CPU基準）を導入する。

## 4. 優先実施順（最小）

1. 3章の 1〜3 を完了する（capability + schema 土台）
2. 3章の 4〜5 を完了する（modulation 命名/互換方針）
3. 3章の 6〜8 を完了する（検証強化 + 運用チェック + テスト基盤）

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
