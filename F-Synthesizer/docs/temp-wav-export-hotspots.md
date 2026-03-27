# WAV書き出し時の高負荷ポイント（破棄前提メモ）

作成日: 2026-03-27  
用途: 一時分析メモ（不要になったら削除）

## 結論（優先度順）
1. 最重: レンダリング本体（サンプル単位ループ）
2. 重: ボイス合成（`RenderVoices`）
3. 中〜重: マスターエフェクト
4. 中: WAVエンコード・書き込み（保存処理）

`WAV`書き出し時の体感遅延は、保存I/Oよりも「保存前レンダ」のCPU負荷が支配的。

## 1. サンプル単位のメインループ（最重）
- 対象:
  - `src/SynthEngine/Engine.cpp:462`
  - `src/SynthEngine/Engine.cpp:474`
  - `src/SynthEngine/Engine.cpp:481`
  - `src/SynthEngine/Engine.cpp:482`
- 内容:
  - `for (int i = 0; i < sound.length; i++)` で全サンプル処理。
  - 各サンプルで `ProcessEventsAtSample` → `RenderVoices` → `ApplyMasterEffects` を実行。
- 負荷理由:
  - 44.1kHzでは1秒あたり44,100回ループ。
  - 曲長に比例して確実に増加するホットパス。
- 現状の最適化（実施済み）:
  - `cleanupInterval=256` による間引きクリーンアップ。
  - `channelRenderable[]` の事前計算でホットループ内の分岐を削減。
  - キャンセル確認を 256 サンプル単位に間引き。
- 対応策:
  - **ブロック処理化（N サンプル単位）**: `RenderVoices` / `ApplyMasterEffects` を 64〜256 サンプルのブロック単位に変更すると SIMD 化・キャッシュ効率が大幅に改善できる。
  - **実施可否**: ❌ 現状では困難。全下流関数のシグネチャ変更が必要で、侵襲度が高く保守性・拡張性へのリスクが大きい。大規模な設計変更として別途計画が必要。

## 2. ボイス合成 `RenderVoices`（最重クラス）
- 対象:
  - `src/SynthEngine/Renderer.cpp:626`
- 内容:
  - アクティブボイスを毎サンプル走査し、音源・変調・フィルタ・パン・ゲインを合成。
- 負荷理由:
  - 実効計算量は概ね `O(サンプル数 × 同時発音ボイス数)`。
  - 内部で高コスト関数を多数使用。
- 関連ホットスポット:
  - モジュレーション評価: `src/SynthEngine/Renderer.cpp:195`, `src/SynthEngine/Modulation.cpp:137`
  - ユニゾン/デチューン: `src/SynthEngine/Renderer.cpp:241`
  - フィルタ適用: `src/SynthEngine/Renderer.cpp:574`, `src/SynthEngine/Filter.cpp:151`
  - 数学関数の多用（`pow/sin/tanh`）:
    - `src/SynthEngine/Renderer.cpp:206`
    - `src/SynthEngine/Renderer.cpp:230`
    - `src/SynthEngine/Renderer.cpp:246`
    - `src/SynthEngine/Renderer.cpp:275`
    - `src/SynthEngine/Modulation.cpp:206`
    - `src/synth/Oscillator.cpp:43`
- 対応策:

  ### 2-A. ユニゾン detune 比率のキャッシュ（Renderer.cpp:246）
  - 現状: `pow(2.0, cents / 1200.0)` を毎サンプル × ユニゾン数だけ実行。
  - 改善: `cents` は `uv`・`unisonVoices`・`unisonDetuneCents` のみで決まりレンダリング中に変化しない。ノートオン時に `VoiceState` へキャッシュすることで、ユニゾン数分の `pow` をゼロにできる。
  - **実施可否**: ✅ すぐ可能。可読性・保守性への影響は最小。
  - **実施状況**: ✅ 実施済み（`WaveformVoiceState` / `AnalogVoiceState` に `unisonDetuneRatio[8]` を追加し、NoteOn初期化で計算）。

  ### 2-B. filterKeytrack 比率のキャッシュ（Renderer.cpp:275）
  - 現状: `pow(2.0, filterKeytrack * (noteNumber - 60) / 12.0)` を毎サンプル実行。
  - 改善: `filterKeytrack` と `noteNumber` はレンダリング中に変化しない（ポルタメントは phaseInc/pitchFactor 経由で反映される）。ノートオン時に1回だけ計算してキャッシュできる。
  - **実施可否**: ✅ すぐ可能。副作用なし。
  - **実施状況**: ✅ 実施済み（`filterKeytrackRatio` をNoteOn時に計算し、レンダ中の `pow` を除去）。

  ### 2-C. arpeggio semitone → ratio の LUT 化（Renderer.cpp:230）
  - 現状: `pow(2.0, semitoneOffset / 12.0)` をアルペジオステップが進むたびに実行。
  - 改善: semitones 配列は整数で範囲も限定的（−24〜+24）なので、49 エントリの静的 LUT に置き換えると `pow` をゼロにできる。
  - **実施可否**: ✅ すぐ可能。コードも明瞭になる。
  - **実施状況**: ✅ 実施済み（`-24..24` の49要素 LUT を `Renderer.cpp` に追加して参照）。

  ### 2-D. `tanh(k)` の定数キャッシュ（Renderer.cpp:579-588）
  - 現状: `tanhK = tanh(shaperDrive * 20.0)` を毎サンプル実行。
  - 改善: `shaperDrive` は config 値なのでノートオン時または設定変更時にキャッシュできる。`tanh(k * sample)` は毎サンプル必要だが、`tanhK` の除算用定数だけでも定数化できる。
  - **実施可否**: ✅ すぐ可能。影響小。
  - **実施状況**: ✅ 実施済み（`driveNorm=1/tanh(k)` を VoiceState に保持し、レンダ中は乗算のみ）。

  ### 2-E. `pow(2.0, x)` → `exp2(x)` への置き換え（Modulation.cpp:206 など）
  - 現状: ピッチモジュレーション適用で `std::pow(2.0, value)` を使用。
  - 改善: `std::exp2(x)` は C++11 標準で、多くの実装で `pow(2.0, x)` より高速。意味も明確になる。
  - **実施可否**: ✅ すぐ可能。ドロップイン置換で副作用なし。可読性も向上。
  - **実施状況**: ✅ 実施済み（`Modulation.cpp` に加え `Renderer.cpp` / `Voices.cpp` の base-2 累乗を `exp2` 化）。

  ### 2-F. モジュレーション評価のレート間引き（Renderer.cpp:195, Modulation.cpp:137）
  - 現状: `EvaluateModulation`（LFO/Env2 含む）を毎サンプル × ボイス数だけ実行。
  - 改善: LFO/Env2 は数Hz〜数十Hzで動作するため、1/4〜1/8 レートで評価して線形補間することでボイス数×モジュレーション評価コストを大幅削減できる（control-rate / audio-rate 分離）。知覚上の劣化はほぼない。
  - **実施可否**: ⚠️ 設計変更が伴う。`EvaluateModulation` のインターフェース変更と補間バッファの追加が必要。効果は大きいが、別途設計を検討すること。

## 3. マスターエフェクト（中〜重）
- 対象:
  - `src/SynthEngine/Engine.cpp:368`
- 内容:
  - `ApplyMasterEffects` 内で `retro -> chorus -> flanger -> delay -> reverb` を逐次適用。
- 負荷理由:
  - 各サンプルで複数バッファ参照・更新を実施。
  - エフェクト有効時は `sin` 計算や遅延線処理が積み上がる。
- 対応策:

  ### 3-A. 無効エフェクトのスキップ確認
  - 現状: `ApplyMasterEffects` 内の各エフェクト関数が無効時の early return を持っているか未確認。
  - 改善: 各 `ApplyXxx` 関数でエフェクト無効時にバッファコピーのみ行う early return を確保する。
  - **実施可否**: ✅ 確認後にすぐ対応可能。保守性への影響なし。
  - **実施状況**: ✅ 実施済み（各 `ApplyXxx` の early return を確認済み。加えて `ApplyMasterEffects` 冒頭で「全エフェクト無効時は入力をそのまま返す」早期リターンを追加）。

  ### 3-B. Chorus/Flanger の sin を位相累積に統一
  - 現状: モジュレーション LFO の sin を毎サンプル計算している可能性がある。
  - 改善: sin ではなく位相累積（phase accumulator）を使い、低レートで sin テーブルまたは多項式近似に切り替えると削減できる。
  - **実施可否**: ⚠️ 各エフェクト実装の詳細確認が必要。音質への影響度に応じて判断。

## 4. WAV保存処理（中）
- 対象:
  - `src/io/Writer.cpp:75`
  - `src/io/Writer.cpp:76`
  - `src/io/Writer.cpp:122`
- 内容:
  - `SoundData` を `int16` へ全サンプル変換し、`wdata` に展開後に一括書き込み。
- 負荷理由:
  - 変換＋メモリコピー＋ファイルI/Oが発生。
  - 計算量はおおむね `O(サンプル数)` のため、通常はレンダより軽い。
- 補足:
  - 低速ディスク・ウイルススキャン・OneDrive同期等でI/O待ちが増えると体感遅延は増える。
- 対応策:

  ### 4-A. toPcm16 のクランプを `std::clamp` に統一（Writer.cpp:80-84）
  - 現状: 三項演算子によるクランプ処理のため、コンパイラの自動ベクトル化が阻害される場合がある。
  - 改善: `std::clamp` に置き換えることで SIMD 化のヒントになる。コードも簡潔になる。
  - **実施可否**: ✅ すぐ可能。効果は限定的だが、コードの明瞭化という副次効果あり。
  - **実施状況**: ✅ 実施済み（`toPcm16` のクランプを `std::clamp` に置換し、`<algorithm>` を追加）。

  ### 4-B. 非同期書き込み
  - 現状: レンダ完了後に同一スレッドで fwrite を実行。
  - 改善: レンダ完了後に別スレッドでファイル書き込みを行うと、UI がブロックされる時間を短縮できる（OneDrive 環境での遅延対策にもなる）。
  - **実施可否**: ⚠️ スレッド管理とエラー通知の仕組みが必要。効果は環境依存（低速ディスク/OneDrive 時に顕著）。

## まとめ
- 主因は `WAV` 書き込みそのものではなく、書き込み前のレンダリング（特に `RenderVoices`）。
- 改善優先度は「レンダ内ホットパス最適化」＞「WAV保存最適化」。

### 即時実施可能な最適化（✅）
| 対応策 | 対象箇所 | 想定効果 |
|---|---|---|
| 2-A: ユニゾン detune 比率キャッシュ | Renderer.cpp:246 | ユニゾン数分の `pow` を削減 |
| 2-B: filterKeytrack 比率キャッシュ | Renderer.cpp:275 | 毎サンプルの `pow` を削減 |
| 2-C: arpeggio semitone LUT 化 | Renderer.cpp:230 | `pow` をゼロ化 |
| 2-D: `tanh(k)` 定数キャッシュ | Renderer.cpp:579 | 除算定数の毎サンプル計算を削減 |
| 2-E: `exp2` 置き換え | Modulation.cpp:206 他 | `pow` より高速な関数へ置換 |
| 3-A: 無効エフェクトの early return 確認 | Engine.cpp:368 | 無効エフェクトのコストをゼロ化 |
| 4-A: toPcm16 クランプ統一 | Writer.cpp:80 | SIMD 化ヒント・コード明瞭化 |

### 設計変更が必要な最適化（⚠️）
| 対応策 | 内容 | 備考 |
|---|---|---|
| 2-F: モジュレーション レート間引き | control-rate / audio-rate 分離 + 補間 | 効果大、インターフェース変更が必要 |
| 3-B: sin を位相累積に統一 | Chorus/Flanger の sin 計算削減 | 実装確認後に判断 |
| 4-B: 非同期書き込み | レンダ後の書き込みをスレッド化 | 低速ディスク環境で効果あり |

### 現状では困難な最適化（❌）
| 対応策 | 理由 |
|---|---|
| 1: メインループのブロック処理化 | 全下流関数のシグネチャ変更が必要な大規模改修 |
