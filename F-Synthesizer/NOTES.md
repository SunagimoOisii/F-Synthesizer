# Notes

## 将来拡張
- Program Change対応: チャンネルプリセットを上書きする
- FM拡張: アルゴリズム追加、feedback、複数オペ、LFO/EGでmodIndex制御
- FMがアホほど使いにくい: プリセット追加？
- ChannelConfig で関係ないパラメータも入力する必要があるのが面倒(NoiseではWave関係はいらない)

## 修正すべき不具合
- MidiParser: メタイベント/ SysEx後にランニングステータスをクリアしていないため、後続データが誤解釈される可能性
- SynthEngine: 同一ノートが重なった場合、NoteOffが最初に一致したVoiceに当たり、解放対象がずれる可能性
- Writer: SaveCsvFileがファイルオープン失敗でもtrueを返す

## 読みにくいコードの部分
## 読みにくいコードの部分の可読性を向上させる案

## 各プログラムのクラス化によるメリット/デメリット
## Pink/Brown/Blueノイズ実装方針
- NoiseTypeにPink/Brown/Blueを追加する
- SampleNoiseでNoiseTypeに応じた生成を行う
- Pink/Brownは1/f, 1/f^2特性になるよう一次/二次の積分フィルタ系を使う（実装が簡単で高速）
- Blueは1/f特性の逆として高域寄りにする（差分フィルタで近似しやすい）
- 生成後は振幅を正規化し、レベルが暴れないよう調整係数を入れる

