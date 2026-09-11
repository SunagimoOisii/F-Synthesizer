---
name: fsynth-sound-design
description: F-Synthesizer専用の音色制作。FMプリセットやドラムセットの新規作成・刷新・微調整・比較評価を、既存の音源とJSON形式で進める。音色の相談にも使うが、UIデザインや歌詞・作曲だけの依頼には使わない。
---

# F-Synthesizerの音色制作

手持ちのMIDIで使いたくなる音を、役割と違いが分かるプリセットにする。
好みはAGENTS.mdと会話を引き継ぎ、具体的な試聴結果を優先する。相談・比較案・Skill整備の依頼では、本体のプリセットを変更しない。制作・刷新を依頼されたら、その範囲の作成と検証を進める。

## 狙いと比較対象を決める

- 対象パート、用途、欲しい特徴、避けたい特徴を一文にする。例えば「速いフレーズでも輪郭が残るベース、余韻は短め」。不足があっても既知の好みから仮案を作り、結果を左右する点だけ確認する。
- 現在の音を基準として残す。参考曲がある場合は曲名だけで音色を断定せず、どのパート・区間・特徴を指すかを確認する。参考音源を評価できていなければ、その違いを明記する。
- 候補ごとに「何を変え、どんな違いを期待するか」を決める。方向が未定なら2〜3個程度が目安。数やパートは依頼に合わせる。音量や名前だけを変えた候補を増やさない。
- 印象語は仮説として使う。金属感なら周波数比と変調量、輪郭なら弾き始めの倍音と減衰、柔らかさなら高域と立ち上がりを検討する。レトロだから矩形波、FMだから金属的、と決めつけない。

## 本体で実現できる設定を選ぶ

以下はリポジトリ直下からのパス。型定義だけでなく、読込と発音経路を確認する。音源の一般的な機能と、このアプリが公開している設定を区別する。

| 調べること | 必要なときに読む場所 |
|---|---|
| JSONとmetadataの見本 | `F-Synthesizer/config/presets/` の同じパートの音色 |
| FMの入力値・チップへの変換 | `F-Synthesizer/include/SynthEngine/FmConfig.h`、`F-Synthesizer/src/config/load/LoadSourceFm.cpp`、`F-Synthesizer/src/synth/YmfmVoice.cpp` |
| FMの変調・発音終了 | `F-Synthesizer/src/SynthEngine/renderer/RenderFm.inl`、`F-Synthesizer/src/SynthEngine/Voices.cpp`、`F-Synthesizer/src/SynthEngine/Renderer.cpp` |
| ノブの有効範囲と変換 | `F-Synthesizer/src/gui/GUIToneWorkspace.cpp` の `ToneControlSupported`、`ApplyToneValues` |
| ドラムセットの割当・音色 | `F-Synthesizer/include/SynthEngine/DrumKitConfig.h`、`F-Synthesizer/src/SynthEngine/renderer/RenderDrum.inl`、`F-Synthesizer/config/presets/sound_drums_forge.json` |
| 比較音量の読込・適用 | `F-Synthesizer/src/gui/GUIPresetIO.cpp`、`F-Synthesizer/include/project/ProjectModel.h` の `RenderSound` |
| CLI・測定の実例 | `F-Synthesizer/scripts/check_presets.ps1`、`F-Synthesizer/tests/tone_checks.h` の `CalibratePresetLevels` |

JSONは `projectModel.v3`。`project.instruments.<id>.sound` に音色、同じinstrumentにmetadataを置き、`project.channels` の `instrumentId` と対応させる。チャンネルは0始まり、通常のドラムch10は `9`。分類は現行UIに合わせ、説明は聴こえる特徴と用途、推奨音域は試した範囲に合わせる。既存の `macroHints` は検証が旧4項目を参照するため、画面のノブ数だけを理由に削らない。

### 現行FM経路の注意点

- `chip: 0` はYM2151、`1` はYM2612。`algorithm` は0〜7で、DX7の番号ではない。新規音色では `ops` 4要素を明示する（読込自体は1〜4要素を許す）。接続とキャリアの役割は `YmfmVoice.cpp` の `kCarriers` と同梱ymfmを確認する。
- 実際の倍率は0.5または整数1〜15、波形は正弦波。JSONで受理された数値がそのまま鳴るとは限らない。`feedback` は0〜1から0〜7へ、レベルとエンベロープも段階的な値へ変換される。秒数を実測の長さと説明しない。
- 変調側のレベルは `op.level * clamp(op.index * indexScale * 2^((brightness - 0.5) * 4) / 4, 0, 1)` が元になる。上限に張り付くと明るさやindexを上げても変わらない。キャリアの `index` はこの計算に使われない。汎用FMの変調指数をそのまま代入したつもりにならない。
- `indexEnv` は保存できるが、現行ymfm発音では評価されない。倍音の時間変化には変調側の `levelEnv` や実装済みのmodulationを使う。全体のADSRと各オペレーターのADSRの両方を確認し、短い方が他方の変化を隠していないか調べる。
- 接続・周波数比、変調側の強さと時間変化、必要な揺れ・効果の順を出発点にする。明るさだけが違う案に偏らず、輪郭、持続中の倍音、音域への適性も候補の違いにする。
- ノブは、その音色で表示される項目について中央と両端付近を確認する。例えばalgorithm 7では明るさを表示しない。無効な項目を働かせるために音色の構成を変えない。表示されるノブが丸めや飽和で効かない場合は、基準値やレベル配分を見直す。

これらの注意点は現行実装についてのもの。反映されない設定を見つけても、プリセット制作のついでに音源エンジンを拡張しない。まず既存設定で狙いを満たせるか判断し、満たせない点を具体的に伝える。
ドラムではノート番号ごとの役割を保ち、セット内の音量、低域、ハイハットとシンバルの余韻を整える。単発と短いビートで確認し、キックだけを測ってセット全体の良否を決めない。

## 候補を生成して比較する

候補JSON、短いMIDI、WAVは `F-Synthesizer/output/sound-design/<作業名>/` に置く。既存音色のコピーも比較に含める。本体と同じ `ProjectModel -> RenderConfig -> SynthEngine` を使い、ユーザーの曲・workspace・お気に入りを試験用に書き換えない。

本体CLIの実行例（Debugビルド済みの場合、リポジトリ直下）：

```powershell
& .\F-Synthesizer\build\x64\Debug\F-Synthesizer.exe --cli --config '<比較用設定JSONの絶対パス>'
```

一時設定の `project` に `midiPath`、`wavPath`、`targetChannel`、`sampleRate`、`bits`、`initialSeconds`、`extraReleaseSec` を入れる。パスは絶対パスを使う。`initialSeconds` は初期バッファ長で、切り詰め指定ではない。短いMIDIを作り、離鍵後が収まる `extraReleaseSec` を設定する。作成例は `check_presets.ps1` の `Write-ShortPresetConfig` と `Invoke-ShortRenderCheck` を参照する。

- 基準音と候補でMIDI、ベロシティ、テンポ、曲のミックス、マスター効果、出力条件をそろえる。音色自体に含まれる効果は候補の一部として扱い、効果が差を覆う場合は別途ドライ音も比較する。
- 音域の低・中・高、弱・強、短音・長音・離鍵後を、用途に合う短いフレーズで確認する。和音や連打は用途に応じて追加する。提供済みの曲があれば短い区間に混ぜて確かめる。無音の長さだけでRMSが変わらないよう、同じ時間窓で測る。
- ピーク、RMS、無音、音切れを確認する。必要なら波形や時間ごとのスペクトルで仮説を調べる。数値だけで音の良さや好みへの適合を採点しない。乱数を使うドラム等は、毎回の差を編集の効果と取り違えない。

### 比較音量を取り違えない

付属音色の補正表 `F-Synthesizer/assets/ui/preset-levels.json` はGUIの読込時に使われ、CLIが生のプリセットJSONを読むだけでは適用されない。

- 未変更の付属音色は、表のgainを一時設定のinstrumentの `comparisonGain` に設定する。曲・お気に入りに既に保存された補正はそのまま使い、重ねて掛けない。`amp` にも同じ補正を焼き込まない。
- 音色や推奨試聴音高を変更した候補に、古い表のgainを最新の実測値として使わない。比較段階では隔離した候補を同条件で測り、一時設定で補正する。測定元の `comparisonGain` は1にし、元の音色の `amp` は保つ。
- 全パートの採用前候補を測るためだけに、本体の補正表を書き換えない。最終版を付属音色へ反映した後、AGENTS.mdの `check_persistence.ps1 -CalibratePresetLevels` で表を再測定する。
- 既存のRMS補正は近似で、知覚的な音量一致を保証しない。大きいだけの音を選ばないために使い、弱音から強音までの変化は保つ。ノートごとに別の補正を掛けて均すことはしない。

## 絞り込みと完了

短尺チェックが通るだけでは音色の完成としない。依頼の範囲で次を確認する。

- **技術面**：本体で読めて鳴る、対象音域で意図しない無音や音切れがない、離鍵後の挙動と表示されるノブの変化を確認した。
- **比較面**：基準音と候補を同条件で比較でき、候補の違いと狙いが説明できる。実際に音を評価していなければ「生成・数値確認済み、試聴評価は未実施」と報告する。
- **受け渡し**：適用できるJSONと短い比較用WAV、変えた特徴、確認条件、残る試聴事項を簡潔に示す。仮候補と付属音色へ反映した版を明確にする。

好みの判断が必要なら、絞った候補を提示して試聴を受ける。任せると依頼されている範囲は既知の好みから暫定選定して進め、試聴のためだけに毎段階で止まらない。狙いに合い、必要な確認が済んだら数合わせや際限のない探索を続けない。
検証はAGENTS.mdに従い、変更に合うものだけ行う。付属音色の更新ではpreset checkと補正表の再測定を行う。Skillの文章だけの更新では音色一式の再ビルド・再較正は不要。
試聴で得た一曲限りの印象を、Skillの普遍的な規則にしない。Skillへ残すのはユーザーが継続方針として示した好みや、繰り返し役立つ実装上の注意点。外部の文章・コード・音色データを取り込む場合は配布条件と出典を確認する。
