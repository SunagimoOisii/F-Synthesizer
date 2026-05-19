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

## 運用

1. 依存追加時はこの対応表を更新する
2. ライセンス方式が複数ある依存は、採用した方式を明記する
3. 配布物には `licenses/` フォルダごと同梱する
