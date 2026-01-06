# Notes

## 将来拡張
- Program Change対応: チャンネルプリセットを上書きする
- FM拡張: アルゴリズム追加、feedback、複数オペ、LFO/EGでmodIndex制御
- ChannelConfig で関係ないパラメータも入力する必要があるのが面倒(NoiseではWave関係はいらない)
- Voice の channel, channelIndex を統合するか

## 優先順位(音質/表現力優先・大きめ変更OK)
### 高
- LFO/フィルタ/デチューン導入(ストリングス等の表現力底上げ)
- MidiParser: メタイベント/ SysEx後のrunning statusクリア
- SynthEngine: 同一ノート重なり時のNoteOffずれ
### 中
- FM拡張(アルゴリズム/複数オペ/feedback)
- ChannelConfig記述量の削減(型別の最小記述など)
### 低
- Program Change対応(後回しでOK)
- Voiceのchannel/channelIndex統合

## LFO/フィルタ/デチューンの設計方針
### LFO
- 目的: ピッチ/アンプ/フィルタへの緩い周期変調で質感を付与
- 設計: VoiceにLFO位相と速度を持ち、Sampleごとに更新
- 適用先: pitchFactor/amp/filterCutoffへ倍率として反映
- 拡張: 波形選択、Delay/Depth/RateをChannelConfigで指定
### フィルタ
- 目的: 倍音の整理と時間変化(attack時に明るく、sustainで落ち着く)
- 設計: 1次 or 2次のローパスをVoice単位で保持
- 適用先: RenderVoicesで生成した波形に対して通す
- 拡張: カットオフ/レゾナンス/EGをChannelConfigに追加
### デチューン
- 目的: ユニゾン感と厚みを作る
- 設計: Voice生成時に複製(2-4本)し、phaseIncに微小係数を掛ける
- 適用先: 同一ノートの複数Voiceを合成
- 拡張: Detune幅/本数をプリセットで指定
## ドラム合成(メガドライブ風)の設計方針
### 目的
- ノイズ一択から脱却し、ドラムと認識できる質感を合成で出す
### 設計
- SourceConfigにDrumSynthを追加し、Kick/Snare/Hatを種別で切替
- Kick: サイン波 + 短いピッチエンベロープ(100-150Hz→50Hz)
- Snare: ノイズ + 低域トーンをミックス
- Hat: ハイパスしたノイズ + 超短ADSR
### 必要な拡張
- パーカッション向けの極短ADSR
- 簡易フィルタ(1次HP/LP)の導入

## 修正すべき不具合
- MidiParser: メタイベント/ SysEx後にランニングステータスをクリアしていないため、後続データが誤解釈される可能性
- SynthEngine: 同一ノートが重なった場合、NoteOffが最初に一致したVoiceに当たり、解放対象がずれる可能性
- SaveWavFile: sound.bitsを16以外にするとヘッダと実データの不整合が起きる(実装は16bit固定)

## 現状の設計改善点
- SourceConfigのvariant運用は安全だが、SoundGenerate側の記述量が増える
- Noise生成がthread_local状態のため、Voice単位の再現性や制御がしづらい
- FMのパラメータがVoice内部に散在し、将来の複数オペ化で肥大化する
- チャンネルプリセットが直接配列初期化で、Program Changeやプリセット切替に弱い
- SynthEngineが描画と状態更新を一体で持ち、責務の境界が曖昧

## SoA導入案（RenderVoices中心）
- 対象: Voice配列（AoS）をSoA化し、音源タイプ（Wave/Noise/FM）ごとに別配列管理
- 目的: RenderVoicesの分岐/メモリアクセスを単純化し、キャッシュ効率とベクトル化の余地を確保
### メリット
- アクティブボイス数が多い場合にキャッシュ効率が上がり、RenderVoicesのCPU負荷が下がる
- std::visitの分岐や型チェックを排除でき、ホットループが単純化する
- 将来的にSIMD化や並列化（音源タイプ別バッチ処理）がやりやすい
### デメリット
- データ構造が複雑になり、実装量と保守コストが増える
- 音源タイプの追加時に配列の追加・同期が必要で拡張時の変更箇所が増える
- デバッグ時に「1ボイスの状態」が追いにくくなる
### 効果目安
- SoA化のみ: 1.1〜1.3x（キャッシュ効率改善）
- SoA + 音源タイプ別配列: 1.3〜1.8x（分岐削減）

## 設計改善案の評価
- SourceConfig記述量増: メリット=無効組み合わせ防止/保守性向上, デメリット=設定が冗長, 妥当性=variant運用を優先するなら妥当
- Noise状態の分離: メリット=再現性/制御性/拡張性向上, デメリット=実装量増/状態管理増, 妥当性=Pink/Brown本格化なら妥当
- FMパラメータ整理: メリット=拡張しやすい/責務が明確, デメリット=構造変更コスト, 妥当性=複数オペやアルゴリズム計画があるなら妥当
- プリセット管理導入: メリット=Program Change対応が容易/可読性向上, デメリット=管理コード増, 妥当性=プリセット運用を行うなら妥当
- SynthEngine責務分離: メリット=可読性/保守性向上, デメリット=把握箇所が増える, 妥当性=拡張が続くなら妥当

## クラス化案（今後の拡張前提）
- FmEngine: 複数オペ/アルゴリズム/feedback対応のためFM責務を独立化
- NoiseGenerator: ノイズ種別ごとの状態と正規化をVoice単位で管理
- SequencerState: テンポ/CC/ProgramChangeの状態管理をクラスに集約
- MidiParser: running status等の状態を内部化し、拡張イベント対応を容易化
- PresetManager: Program Change対応のためプリセット管理を独立化
## クラス化案の評価
- FmEngine: メリット=拡張性/責務分離が大きい, デメリット=実装量増, 妥当性=複数オペ/アルゴリズム実装を見据えるなら妥当
- NoiseGenerator: メリット=状態管理と音色安定性向上, デメリット=構造増, 妥当性=ノイズ種別拡張を進めるなら妥当
- SequencerState: メリット=テンポ/CC/ProgramChangeが整理される, デメリット=初期化/テストコスト増, 妥当性=時間系拡張を予定するなら妥当
- MidiParser: メリット=拡張イベント対応が容易, デメリット=現状規模では過剰, 妥当性=SMF拡張を進めるなら妥当
- PresetManager: メリット=プリセット切替の一元管理, デメリット=管理対象と依存が増える, 妥当性=Program Change運用が前提なら妥当
