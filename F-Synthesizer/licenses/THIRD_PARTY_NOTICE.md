# サードパーティ通知

このファイルは、本プロジェクトで利用するサードパーティライブラリと
同梱ライセンス原文の対応を示す。

## ライブラリ対応表

1. Dear ImGui
- ライセンス: MIT
- 本文: `licenses/SPDX-MIT.txt`
- URL: https://github.com/ocornut/imgui

2. GLFW
- ライセンス: zlib/libpng
- 本文: `licenses/SPDX-zlib.txt`
- URL: https://www.glfw.org/

3. OpenGL (`opengl32.lib`)
- 由来: Windows SDK / OS component
- 注記: このリポジトリには OpenGL 実装本体を同梱しない
- URL: https://www.khronos.org/opengl/

4. miniaudio
- ライセンス: Unlicense or MIT
- 本文: `licenses/SPDX-Unlicense.txt` (Unlicense を選択した場合)
- URL: https://github.com/mackron/miniaudio

5. nlohmann/json
- ライセンス: MIT
- 本文: `licenses/SPDX-MIT.txt`
- URL: https://github.com/nlohmann/json

6. ymfm
- ライセンス: BSD-3-Clause
- 著作権・原文: [third_party/ymfm/LICENSE](../third_party/ymfm/LICENSE)
- 固定リビジョン: `81aec25ccbb98f4873a255f7551ac4dadac59b4a`
- URL: https://github.com/aaronsgiles/ymfm
- YM2151 / YM2612 の FM 発音に使用。ソースは変更せず同梱する。

7. midifile
- ライセンス: BSD-2-Clause
- 著作権・原文: [third_party/midifile/LICENSE.txt](../third_party/midifile/LICENSE.txt)
- 固定リビジョン: `98917df5b1bf0d6e8d4c0e5fff86d6b05343e793`
- URL: https://github.com/craigsapp/midifile
- MIDI の読込、音符の対応付け、テンポを含む時刻変換に使用。ソースは変更せず同梱する。

付属音色は本プロジェクトで作成した設定データ。ゲームの音色バンク・ROM・録音素材は同梱しない。

## 運用

1. 依存追加時はこの対応表を更新する
2. ライセンス方式が複数ある依存は、採用した方式を明記する
3. 配布物には `licenses/` と、上記 ymfm / midifile のライセンス原文を同梱する
