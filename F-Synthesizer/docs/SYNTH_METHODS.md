# SYNTH_METHODS

最終更新: 2026-02-23
担当: Synth/Audio
状態: Draft（分割運用）

## 1. 目的

合成方式の実装判断を一貫させ、方式ごとの現状・改善案を継続運用する。

## 2. 運用方針

- 実装済み方式は個別 `md` で管理する。
- 未実装方式は本ファイルで方針のみ管理する。
- 方式の追加実装時に、対応する個別 `md` を新規作成する。
- 実装接続の標準手順は `docs/synth-methods/integration-playbook.md` を参照する。
- 方式間の重複防止は `docs/synth-methods/method-boundaries.md` を正として運用する。

## 3. 実装済み方式（個別ファイル）

- 基本波形: `docs/synth-methods/waveform.md`
- ノイズ: `docs/synth-methods/noise.md`
- FM（2オペレータ）: `docs/synth-methods/fm.md`
- Drum / DrumKit: `docs/synth-methods/drum-drumkit.md`

## 4. 未実装方式（現時点）

- 減算合成
  - 高優先候補。フィルタ/変調の共通基盤を作れる。
- 加算合成
  - 設計自由度は高いが、CPU とパラメータ複雑性に注意。
- PSG
  - 低コストで音色バリエーションを増やせる候補。
- アナログ模倣
  - 減算基盤（フィルタ、非線形、変調）の上に段階導入。
- PCM（サンプル再生）
  - ユーザー価値は高いが、資産管理とメモリ/I/O設計の影響が大きい。

## 5. 共通評価軸

- ユーザー価値
- 実装コスト
- 保守コスト
- 拡張性
- テスト容易性
- 性能（CPU / メモリ）

## 6. 更新ルール

- source type を追加/変更したら次を更新する。
  - 本ファイル（`SYNTH_METHODS.md`）
  - 責務境界ガイド（`docs/synth-methods/method-boundaries.md`）
  - 実装済み方式の個別ファイル
  - `docs/STATUS.md` の Current Snapshot
  - 責務境界が変わる場合は `docs/Architecture.md`
- 方式マイルストーン完了時は、個別ファイルに以下を追記する。
  - Done 条件
  - テスト結果
  - 残課題
