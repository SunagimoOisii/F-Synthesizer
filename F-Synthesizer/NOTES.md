# Notes

## 将来拡張
- Program Change対応: チャンネルプリセットを上書きする
- FM拡張: アルゴリズム追加、feedback、複数オペ、LFO/EGでmodIndex制御
- FMがアホほど使いにくい: プリセット追加？

## 修正すべき不具合
- MidiParser: メタイベント/ SysEx後にランニングステータスをクリアしていないため、後続データが誤解釈される可能性
- SynthEngine: 同一ノートが重なった場合、NoteOffが最初に一致したVoiceに当たり、解放対象がずれる可能性
- Writer: SaveCsvFileがファイルオープン失敗でもtrueを返す

## 読みにくいコードの部分
- Sequencer: tick整列の優先順位計算が式の中に埋まり、可読性が低い
- Sequencer: MidiEventTick -> MidiEventの代入が重複し、差分が見えにくい
- MidiParser: ステータス判定のif連鎖が長く、流れが把握しづらい
## 読みにくいコードの部分の可読性を向上させる案
- Sequencer: 優先順位をenum化し比較関数内で名前で表現する（式の意味が見え、ロジックは同一）
- Sequencer: MidiEvent生成をMakeNoteEvent/MakeControlChangeEventに分離する（共通代入が消え差分が明確になる）
- MidiParser: ParseMetaEvent/ParseChannelEventに分割する（責務が分かれ流れが追いやすい。挙動は不変）

## 各プログラムのクラス化によるメリット/デメリット
- SynthEngine: 音声生成の状態(voices/CC/イベント位置)をクラスに閉じ込められ、処理の流れが明確になる; 一方でAPIが増え、初見の追跡ポイントが増える
- Sequencer: テンポ変換や進行位置をメンバで管理でき、引数の多さ/重複が減る; 一方で状態依存が強くなり、ユニットテストで初期化手順が必要になる
- MidiParser: running status等の状態を局所化でき、責務が明確になる; 一方で単純な関数呼び出しより構造が重くなる
- Oscillator: 波形生成は純関数に近く、クラス化の効果は小さい; 逆に抽象化し過ぎると読みづらくなる
- Envelope: ADSRは状態を持つが現状でも十分明確で、クラス化の効果は限定的; ただし将来の拡張(複数EG等)を見据えるなら有効
- AudioBuffer/Writer: 単純なデータ構造/入出力なので、クラス化しても冗長化しやすい; ただし責務分離や拡張フォーマット対応では利点がある

