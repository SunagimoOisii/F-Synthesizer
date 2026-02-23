# THIRD PARTY NOTICE

このファイルは、本プロジェクトで利用するサードパーティライブラリと
同梱ライセンス原文の対応を示す。

## ライブラリ対応表

1. Dear ImGui
- License: MIT
- Text: `licenses/SPDX-MIT.txt`
- URL: https://github.com/ocornut/imgui

2. GLFW
- License: zlib/libpng
- Text: `licenses/SPDX-zlib.txt`
- URL: https://www.glfw.org/

3. OpenGL (`opengl32.lib`)
- Source: Windows SDK / OS component
- Note: このリポジトリには OpenGL 実装本体を同梱しない
- URL: https://www.khronos.org/opengl/

4. miniaudio
- License: Unlicense or MIT
- Text: `licenses/SPDX-Unlicense.txt` (Unlicense を選択した場合)
- URL: https://github.com/mackron/miniaudio

## 運用

1. 依存追加時はこの対応表を更新する
2. ライセンス方式が複数ある依存は、採用した方式を明記する
3. 配布物には `licenses/` フォルダごと同梱する
