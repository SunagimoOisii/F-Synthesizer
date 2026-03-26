# DECISIONS

最終更新: 2026-03-26

重要な設計判断とトレードオフを記録する。

| 日付 | 判断内容 | 選ばなかった選択肢 | 理由 |
|---|---|---|---|
| 2026-03-26 | Tier D-1（next-features）の arpeggio は waveform/analog 共通で `voice state` に `arpStep/arpElapsedSec` を持たせ、`pitchMul` 決定後〜`phaseInc` 算出前に `pow(2, semitone/12)` を乗算適用する | arpeggio を別voice再トリガ（擬似シーケンサ）として実装する / modulation matrix に専用 destination を追加して間接制御する | 既存voiceライフサイクルや ADSR を崩さず、最小変更で音程ステップのみを安定追加でき、Load/Save/GUI の設定項目も単純化できるため |
| 2026-03-26 | Tier C-2（next-features）の ring modulation は waveform/analog 共通で `mainWave *= ((1-mix) + mix * sin(2π*ringPhase))` を採用し、`ringPhase` は voice state で保持・NoteOnでリセットする | ring信号を別オシレータ音として加算する / 共通グローバル位相で全voice共有する | dry/wet 一体の制御で既存音量経路への影響を最小化でき、voice単位位相で発音ごとの再現性（再トリガ時の挙動）を維持できるため |
| 2026-03-26 | Tier C-1（next-features）の PWM は `ModDestination::PulseWidth` を加算経路（`pulseWidthAdd`）として実装し、最終適用は Renderer で `effectivePW = clamp(base + add, 0.05..0.95)` に統一する | Oscillator 側で modulation 値を直接扱う / Modulation 側で固定クランプして各sourceへ配る | 波形生成器の責務を「位相→波形」に保ちつつ、sourceごとの最終値決定を Renderer に集約でき、waveform/analog の同一ルールで運用できるため |
| 2026-03-26 | Tier D-2 の Env2 curve は `StepADSR` のシグネチャは維持し、`StepEnv2Sample` 後段で `pow(clamp(v), 1/(1+curve*4))` を適用する | ADSRコアへ段階別カーブ引数を追加して全呼び出しを変更する | 既存エンベロープ利用箇所への影響を最小化しつつ、modulation Env2 にだけ curve を導入できるため |
| 2026-03-26 | Tier D-1 の LFO delay/fade は `lfo1ElapsedSec` を runtime state へ持ち、delay中は出力0・fade期間は深さへ乗算する実装とする（`delayMs/fadeMs` は 0..2000ms） | voice 側で別エンベロープを追加して LFO 深さを制御する | 既存LFO/ModMatrix経路を維持しつつ最小変更で自然なビブラート立ち上がりを導入し、旧プリセットは `0ms` 既定で互換維持できるため |
| 2026-03-26 | Tier C-3 では `ModSource::ModWheel` を mod matrix に追加する一方、既存の `lfo1 *= (1 + modwheel)` 乗算も維持して後方互換を優先する | ModWheel追加にあわせて既存のLFO×modwheel乗算を削除する | 旧プリセットの音変化を避けつつ、新規プリセットでは `modWheel -> destination` の明示ルーティングを使える状態にするため |
| 2026-03-26 | Tier C-2 の LFO key sync は `LfoConfig.keySync`（デフォルト `false`）で導入し、`NoteOnModulation(state, cfg)` で `true` 時のみ `lfo1Phase=0` を適用する | 既存free-run挙動を全体でkey syncへ変更する | 既存プリセット互換（フィールド未指定時のfree-run維持）を保ちつつ、必要な音色だけノートオン同期を有効化できるようにするため |
| 2026-03-26 | Tier C-1 の `LfoWave::SampleAndHold` は `floor(phase)` ベースの決定論xorshift生成とし、LFO位相は `StepLfoSample` で連続加算（非wrap保持）してサイクル境界ごとに値を更新する | 既存どおり phase を毎サンプル wrap し続ける | wrap状態では `floor(phase)` が固定化し S&H が変化しないため、仕様どおりのステップ変調を成立させるため |
| 2026-03-26 | modulation destination に `filterResonance` を追加し、`resonanceMul` を common shaper 適用時に乗算する設計とする | フィルタ共振の時間変化を CC のみで扱い、mod matrix には追加しない | Env2/LFO/Pressure から共振を直接駆動できるようにして、既存の filter cutoff modulation と同一経路で一貫実装するため |
| 2026-03-26 | FM アルゴリズムは既存 0..3 の挙動を維持したまま 4..7 を追加し、GUI/Load/Schema の許容範囲も 0..7 に統一する | Renderer だけ先行拡張して GUI/Config 制約を 0..3 のまま残す | 保存・再読込・GUI編集で同じアルゴリズム範囲を扱えない不整合を避け、既存プリセット互換を保った段階拡張にするため |
| 2026-03-26 | Tone Preview スペクトラム表示は外部FFT依存を増やさず、`MainWindow.inl` 内で Hann 窓付き簡易 DFT（軽量サイズ）を実装する | kissFFT などの新規依存を導入して実装する | 既存ビルド構成を維持しつつ、Tone Preview の倍音可視化を最小変更で導入するため |
| 2026-03-26 | Tone Preview ノートは `tonePreviewNoteNumber` を導入して GUI state へ永続化し、DrumKit 以外は手動指定値を優先する | 既存の自動決定（固定 C4 相当）を全 source で継続する | 音域別の聴感確認（C2〜C6 付近）を Sound タブだけで完結させ、再起動後も同じ検証条件を再現できるようにするため |
| 2026-03-26 | Noise source も `common shaper (BiquadFilter)` を通す方針とし、`NoiseConfig` に `filterMode/filterCutoffHz/filterResonance` を追加する | Noise を従来どおりフィルタ非対応の単純発振に固定する | Waveform/Fm/Analog と同一の filter 契約に揃えることで、帯域絞りノイズ（hat/sfx）を GUI/Config/Renderer で一貫運用できるため |
| 2026-03-26 | master effect の処理順を `SampleRateReducer -> BitCrusher -> Chorus -> Flanger -> Delay -> Reverb` に固定し、契約文書を同順に同期する | エフェクト順を未規定のまま実装依存にする / プリセットごとに順序可変とする | 既存プリセットの再現性と比較可能性を保ちつつ、Run/Config/GUI/Engine 間の挙動差異を防ぐため |
| 2026-03-26 | Tier2（stereo/effects/pitchbend-rpn）は実装完了として roadmap を破棄し、設計の正本（`architecture/*`, `synth-methods/*`, `STATUS.md`）へ集約する | Tier2 roadmap を残したまま運用する | 実装済み仕様と計画メモの二重管理を避け、現在有効な契約だけを追跡できる状態を維持するため |
| 2026-03-26 | Tier1 expression foundation は実装完了として roadmap ファイルを破棄し、進捗の正本を `STATUS.md` に一本化する | 完了済みTierを roadmap に残置する | 次アクション（Tier2）を明確化し、完了済み計画と実装済み状態の二重管理を避けるため |
| 2026-03-19 | SubtractiveConfig を廃止し WaveformConfig に filterKeytrack を追加 | SubtractiveConfig を独立 source type として維持する | 減算合成は合成メソッドであり発振器種別ではない。method-boundaries.md の「発振器固有機能の再実装禁止」に違反するため |
| 2026-03-08 | Source種別判定は GUI 直書きではなく `SourceCapabilityOf(...)` を正とし、`SourceRegistry` 契約へ集約する | GUIごとに `SourceKind` / `holds_alternative` 判定を個別維持する | 種別追加時の更新漏れを減らし、Config/GUI間の判定不整合を防ぐため |
| 2026-03-08 | ParameterSchema の `displayName` / `smoothable` / `automatable` は当面導入せず、最小版（`id/type/range/default`）を契約の正とする | 先行してschema項目を拡張しGUI/自動化連携まで同時実装する | 個人開発規模では運用コスト増が先行しやすく、現時点の利用箇所（Config検証）に対して過剰なため |
| 2026-03-08 | 方式固有 destination の GUI編集導線は FM から段階導入し、`fm.index` の選択UIを追加する | 方式固有 destination の編集を当面 JSON 手編集に限定する | 実装済み受理・適用（Phase 1）と編集導線を一致させ、運用負荷を下げるため |
| 2026-03-08 | 方式固有 destination は `fm.index` を Phase 1 で採用し、`source.type=fm` 限定で受理・適用する | 方式固有 destination を当面すべて非採用のまま凍結する | 契約（`<sourceKind>.<parameterId>`）の実効性を小さく検証しつつ、Waveformとの誤混在を防ぐため |
| 2026-03-03 | スモークテストを `quick`（開発時デフォルト）と `full`（包括回帰）に分割する | 常に単一のフルスイートを実行する | 日常開発の待ち時間を抑えつつ、必要時の網羅性を維持するため |
| 2026-03-03 | 機能判断は「音の気持ちよさ」と即時試聴体験を最優先する | DAW 機能拡張を先行する | 想定ユーザー（自分+初心者）に対して価値到達までの時間を短くするため |
| 2026-03-03 | 合成方式の責務境界を維持し、重複実装を禁止する | 方式ごとに同機能を個別実装する | 保守コストと挙動不整合の増加を避けるため |
| 2026-02-23 | `config/base.json` + `presets/*.json` + `--preset` 構成を採用 | 単一巨大設定ファイル運用 | 差分管理と再利用性を優先し、GUI/CLI双方で同一運用にするため |
| 2026-02-23 | `Run(const AppConfig&)` へ実行コアを分離 | GUI/CLI で別実装を維持 | 実行経路を統一し、UI層変更時の影響範囲を縮小するため |
