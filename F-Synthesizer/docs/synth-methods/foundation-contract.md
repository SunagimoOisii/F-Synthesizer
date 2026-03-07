# 合成基盤契約（方式横断）

最終更新: 2026-03-08
状態: Draft（運用開始）

## 1. 目的

- 合成方式の追加前に、方式横断で守る契約を固定する。
- 実装順序に依存した設計崩れ（場当たり `if (type == ...)`）を防ぐ。
- Config / GUI / Renderer / Test の更新漏れを減らす。

関連:
- `docs/SYNTH_METHODS.md`
- `docs/synth-methods/method-boundaries.md`
- `docs/synth-methods/integration-playbook.md`
- `docs/synth-methods/foundation-audit-2026-03-05.md`
- `docs/STATUS.md`
- `docs/STATUS_DETAIL.md`

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
- 含める項目:
  - `id` / `displayName`
  - `type`（bool/int/float/enum）
  - `range`（min/max）
  - `default`
  - `smoothable`（スムージング対象可否）
  - `automatable`（将来拡張用）
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
- 標準順序:
  1. voice state 更新
  2. source render（方式固有）
  3. shaper（filter / drive など共通処理）
  4. modulation 適用
  5. mix / output
- 例外は方式ドキュメントに明記し、理由を残す。

### 2.4 Modulation Routing 契約

- destination 名と単位を固定する。
- 現行 destination（実装済み）:
  - `pitchMul`
  - `amp`
  - `filterCutoffHz`
- `pan` の扱い（2026-03-08 確定）:
  - 現行は `非採用`（ConfigLoad で受理しない）。
  - 理由:
    - 現レンダがモノラル前提で、voice 単位の pan 変調を適用しても出力意味が不明確。
    - `channelMix.pan` と競合しやすく、契約境界を曖昧にする。
  - 再検討条件:
    - ステレオレンダ経路と voice pan 合成規約（pre/post mix）を先に確定した場合のみ採用検討する。
- 方式固有 destination の拡張規約（2026-03-08 確定）:
  - 命名: `<sourceKind>.<parameterId>`（例: `fm.index`）。
  - `sourceKind` は `source.type` と同じ小文字名を使う（`fm`, `noise`, `drum` など）。
  - `parameterId` は対象方式の `ParameterSchema.id` と一致させる。
  - 単位は `ParameterSchema` と同じ意味を使う（無次元倍率/Hzなど）。
- 段階導入方針（2026-03-08 決定）:
  - Phase 1（実装済み）:
    - `fm.index` を ConfigLoad で受理し、FMレンダで適用する。
    - 受理範囲は `source.type=fm` の `modulation` に限定し、Waveform等では非受理にする。
  - Phase 2（将来）:
    - 方式固有 destination を複数追加する場合は、`<sourceKind>.<parameterId>` と `ParameterSchema` の対応表を追加する。
  - Phase 3（将来）:
    - GUI編集導線（方式固有 destination の選択UI）を追加し、手動JSON編集依存を解消する。
    - 2026-03-08 時点では FM の destination 選択UIに `fm.index` を導入済み（Waveform には未表示）。

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

- 新方式追加時の最低確認を固定する。
- 必須項目:
  - 無音入力で無音出力
  - クリップ率上限（閾値は方式別に記録）
  - 同一入力の再現性（乱数利用時はseed固定）
  - CPU負荷の基準（poly数を明記）
- AB比較は耳確認に加え、peak/rms/clip のログを残す。
- 個人運用では、重い自動ハーネスの代わりに `check.ps1` + 代表MIDI手動確認を許可する。

## 3. 方式追加時チェック

1. `method-boundaries.md` で責務境界を先に確定する。
2. 本契約の 2.1〜2.6 を埋める（未定義を残さない）。
3. `integration-playbook.md` の手順に沿って接続する。
4. `SYNTH_METHODS.md` と方式別 `md` を更新する。

## 4. 非目標

- 方式ごとの詳細アルゴリズム設計を本書で定義しない。
- UI文言やプリセット方針の全体設計を本書で代替しない。

## 5. タスク完了後の凍結手順

1. 判定条件:
   - 2.1〜2.6 の必須項目が実装・文書の両方で満たされている。
2. 反映:
   - `SYNTH_METHODS.md` / `integration-playbook.md` / `STATUS.md` / `STATUS_DETAIL.md` / 本書の関連リンクと手順を最終状態へ更新する。
3. 状態更新:
   - 本書の `状態` を `Frozen` へ変更し、`最終更新` を更新する。
4. 履歴化:
   - 未解決論点がある場合は `docs-archive/` へ移送する論点メモを作成してから凍結する。
5. 運用ルール:
   - 凍結後の変更は「新方式追加」または「契約違反修正」のみ許可し、差分理由を冒頭に明記する。

## 6. Foundationタスク対象プログラムファイル

foundation 系タスクでは、原則として次の実装ファイルを操作対象にする。

- Source 契約/定義
  - `include/config/SourceRegistry.h`
  - `src/config/SourceRegistry.cpp`
- Config load/save
  - `src/config/ConfigLoad.cpp`
  - `src/config/load/Internal.h`
  - `src/config/load/LoadSource.cpp`
  - `src/config/load/LoadModulation.cpp`
  - `src/config/ConfigJSONUtils.cpp`
  - `src/config/ConfigFileInternal.h`
- データモデル
  - `include/SynthEngine/SynthEngine.h`
- レンダ/ライフサイクル実装監査対象
  - `src/SynthEngine/Events.cpp`
  - `src/SynthEngine/Voices.cpp`
  - `src/SynthEngine/Renderer.cpp`

新規追加が必要な場合は、次の配置を優先する。

- Config load 拡張:
  - `src/config/load/Load*.cpp`
- Source 契約拡張:
  - `include/config/*.h`
  - `src/config/*.cpp`
- SynthEngine 側契約受け皿:
  - `include/SynthEngine/*.h`
  - `src/SynthEngine/*.cpp`
