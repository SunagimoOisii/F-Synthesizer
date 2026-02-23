# PARAMETER_SMOOTHING_PHASES

最終更新: 2026-02-23  
対象: 共通基盤
関連: `docs/synth-methods/integration-playbook.md`

## 0. 目的

- パラメータ急変時のクリック/ジッパーノイズを低減する。
- 方式ごとの個別実装を避け、共通Smoothing基盤として提供する。
- 今後の音源方式追加（減算/加算/PSG/PCM）でも同一接続手順を使える状態にする。

## 1. 適用スコープ（初期）

- 対象パラメータ（優先）:
  - `amp`
  - `pitch`（係数系）
  - `filterCutoff`
- 次段候補:
  - `resonance`
  - `fm index`
  - `pan`

## 2. Phase構成

## Phase A: 基盤仕様固定

目的:
- 方式/式/時定数の扱いを固定し、後続実装の揺れをなくす。

実施:
- Smoothing方式を確定（初期は `one-pole`）。
- 時間指定を `timeMs` に統一。
- `timeMs=0` をバイパス扱いに定義。
- 値域クランプと異常値扱い（NaN/inf）方針を定義。

Phase A 決定仕様:
- 方式:
  - 1サンプル更新式を `current += alpha * (target - current)` に固定する。
- `alpha` 計算:
  - `timeMs <= 0` の場合はバイパス（`alpha=1`）。
  - それ以外は `tau = timeMs / 1000`、`alpha = 1 - exp(-dt / tau)`、`dt = 1 / sampleRate`。
- 時定数の意味:
  - `timeMs` は「目標値へ滑らかに追従する速度」を表す共通単位。
- 無効値方針:
  - `sampleRate <= 0` は無効設定としてバイパス（`alpha=1`）。
  - `timeMs` が NaN/inf の場合は `0ms` と同等に扱う。
  - `target/current` が NaN/inf の場合は `0` にフォールバックする。
- 値域クランプ（初期）:
  - `amp`: `0..2`
  - `pitchMul`: `0.25..4`
  - `filterCutoffHz`: `10..20000`
- 推奨初期 `timeMs`:
  - `amp`: `4ms`
  - `pitch`: `2ms`
  - `filterCutoff`: `8ms`

完了条件:
- 実装仕様が1ファイルで参照可能（このPHASES + TASKS）。
- 対象パラメータごとの初期推奨 `timeMs` が定義済み。

## Phase B: 共通モジュール実装

目的:
- レンダラ依存なしで再利用できるSmoothing部品を実装する。

実施:
- `SmoothedParam`（`current/target/coeff`）を実装。
- API実装: `Reset`, `SetTarget`, `Step`, `SetTimeMs`。
- `sampleRate` 変更時の係数再計算を実装。
- 無効設定時の低コストバイパスを実装。

完了条件:
- 単体利用で `target` 追従が確認できる。
- `timeMs=0` と通常時で分岐動作が安定している。

## Phase C: Voice/Renderer統合（第1段）

目的:
- 実際の音声経路に接続し、ノイズ低減効果を出す。

実施:
- `VoicesSoA` に smoothing state を追加。
- voice lifecycle（追加/再利用/解放）で初期化ルールを統一。
- まず `filterCutoff` と `amp` に接続。
- `pitch` はフラグ付きで段階接続。

完了条件:
- 同一MIDIで ON/OFF 比較時にクリック低減を確認。
- レンダ負荷が許容範囲（大幅悪化なし）。

## Phase D: Config I/O / GUI接続

目的:
- 基盤を運用可能な設定項目として公開する。

実施:
- Configモデルへ `smoothing` を追加。
- `ConfigLoad` 読込 + バリデーション実装。
- `ConfigJsonUtils` 保存実装。
- GUI編集と preset diff 比較更新。

完了条件:
- GUI編集 -> 保存 -> 再読込で値が一致。
- 無効値入力で安全に失敗/補正できる。

## Phase E: 横展開と方式別接続

目的:
- Waveform以外の既存方式へ段階適用し、共通基盤として成立させる。

実施:
- Noise / FM / Drum(必要箇所のみ)へ接続検討・実装。
- 方式ごとに「接続する/しない」の理由を明文化。
- 将来方式（減算/加算/PSG/PCM）向け接続テンプレート作成。

完了条件:
- `integration-playbook.md` の接続状態が更新済み。
- 各方式での適用可否が明示されている。

## Phase F: AB検証と運用固定

目的:
- 品質確認フローを定型化し、継続開発で再利用できる状態にする。

実施:
- 同一MIDIの ON/OFF 比較手順を固定。
- peak/rms/clip + 聴感評価の記録テンプレートを作成。
- 極端設定（0ms/長時定数/高速連打）テストを定例化。

完了条件:
- AB確認手順がドキュメント化され、再実行可能。
- `STATUS.md` に結果が反映済み。

## 3. 非目標（この計画でやらない）

- 高度な補間方式（Spline/高次IIR）の先行導入。
- 全パラメータ一括自動適用。
- GUI演出改善のみを目的とした実装。
