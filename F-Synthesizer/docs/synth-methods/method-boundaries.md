# 合成方式の責務境界（重複防止ガイド）

最終更新: 2026-03-08  
状態: Draft（運用開始）

## 1. 目的

- 合成方式ごとの責務重複を防ぐ。
- 新方式追加時に「どこへ実装するか」を先に固定する。

## 2. レイヤー分離（実装先の原則）

- `Oscillator Layer`:
  - 音の発振そのもの（波形生成、位相、チューニング、ユニゾン）
- `Shaper Layer`:
  - 発振後の成形（フィルタ、サチュレーション等）
- `Modulation Layer`:
  - 時間変化制御（LFO、追加Env、ルーティング）
- `Mixer/Output Layer`:
  - mute/solo/level/pan/gain、最終出力

## 3. 方式ごとの責務範囲（In / Out）

### Waveform

- In:
  - `sine/square/saw/triangle` 生成
  - band-limited 品質改善
  - `unison`, `sub-osc`, 位相挙動
- Out:
  - 減算フィルタ本体
  - 汎用LFOマトリクス
  - 空間系/マスター系エフェクト

### Noise

- In:
  - ノイズ種別生成（white/pink/brown/blue）
  - ノイズ固有の基本トーン整形
- Out:
  - 汎用フィルタ体系（減算合成側へ実装）

### FM

- In:
  - オペレータ構成と変調計算
  - FM固有パラメータ（ratio/index/algorithm）
- Out:
  - 減算用フィルタ設計
  - アナログ非線形モデル

### 減算合成（未実装）

- In:
  - 共通フィルタ（LP/HP/BP + resonance）
  - filter envelope / filter keytrack
- Out:
  - 発振器固有機能の再実装（waveform/fm/noise側を再利用）

### 加算合成（未実装）

- In:
  - partial管理（倍音ごとの振幅/位相/包絡）
- Out:
  - waveformの`unison/sub-osc`相当を加算側へ二重実装

### PSG（未実装）

- In:
  - チップ制約を再現する発音仕様（離散パラメータ、制限付き波形）
- Out:
  - 汎用高機能シンセ機能（方式の個性を失うため）

### アナログ模倣（未実装）

- In:
  - 減算基盤 + 非線形（drive/saturation） + drift
- Out:
  - 別方式の発振器ロジックの再定義

### PCM（未実装）

- In:
  - サンプル再生、ループ、ピッチ追従、補間
- Out:
  - 波形発振器ロジックのコピー実装

## 4. 実装ルール（変更前チェック）

- 追加パラメータがどのレイヤー責務かを明記する。
- `source.type` 追加時は `SourceRegistry` を唯一の起点にする。
- 方式固有パラメータは他方式へ横展開しない。
- 共通機能化する場合は先にレイヤー（共通モジュール）へ切り出す。

## 5. 命名ルール（衝突回避）

- 方式接頭辞を付ける:
  - 例: `wave.unisonVoices`, `fm.index`, `sub.filterCutoff`, `pcm.loopStart`
- 汎用語単独（`index`, `drive`, `shape` など）は禁止。

## 6. 更新トリガー

- 方式追加/方式拡張時に本ファイルを更新する。
- 責務変更が発生したら `docs/SYNTH_METHODS.md` と同時更新する。
