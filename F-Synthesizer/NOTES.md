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

## FM波形選択の段階実装
- Step1: FMキャリア波形のみ選択可能にする（fmCarrierWave追加、SampleFmPhaseを波形対応に変更）
- Step2: FMモジュレータ波形も選択可能にする（fmModWave追加）
- Step3: FM専用パラメータを構造体化し、SourceTypeにFMを追加するなど設計整理を行う

