# Config And IO

最終更新: 2026-02-25

## Data Path

```mermaid
flowchart LR
    Json[Config JSON]
    FileIO[ConfigFileIO]
    Load[ConfigLoad]
    Parts[load/*.cpp]
    Save[ConfigSave]
    Writer[io/Writer]
    Wav[WAV]

    Json --> FileIO --> Load --> Parts
    Parts --> Save --> Json
    Parts --> Writer --> Wav
```

## Config Surface

| Surface | Location |
|---|---|
| `LoadConfigFile(...)` | `src/config/ConfigFileIO.cpp` |
| `SaveConfigFile(...)` | `src/config/ConfigFileIO.cpp` |

## Config Load Split

| 区分 | File |
|---|---|
| 入口 | `src/config/ConfigLoad.cpp` |
| Top-level | `src/config/load/LoadTopLevel.cpp` |
| Channel | `src/config/load/LoadChannel.cpp` |
| Source | `src/config/load/LoadSource.cpp` |
| Modulation | `src/config/load/LoadModulation.cpp` |
| 内部宣言 | `src/config/load/Internal.h` |

## Save / JSON Helpers

| File | 役割 |
|---|---|
| `src/config/ConfigSave.cpp` | Configの書き出し |
| `src/config/ConfigJsonUtils.cpp` | JSON補助 |
| `src/config/SourceJson.cpp` | Source定義のJSON変換 |
| `src/config/SourceRegistry.cpp` | Source種別解決 |

## Path / Writer

| 区分 | File |
|---|---|
| パス解決・UTF補助 | `src/io/PlatformPaths.cpp`, `include/io/PlatformPaths.h` |
| WAV保存 | `src/io/Writer.cpp`, `include/io/Writer.h` |

## Config/IO実装確認ポイント

| 観点 | 確認点 | 参照 |
|---|---|---|
| 読込責務 | 入口 `ConfigLoad.cpp` から `load/*.cpp` へ分割される | `src/config/ConfigLoad.cpp`, `src/config/load/` |
| 保存導線 | Config保存は `ConfigSave.cpp`、WAV保存は `io/Writer.cpp` が担当する | `src/config/ConfigSave.cpp`, `src/io/Writer.cpp` |
| パス境界 | パス/UTF処理は `PlatformPaths` 経由で行う | `src/io/PlatformPaths.cpp`, `include/io/PlatformPaths.h` |

## Compatibility Matrix

| 変更種別 | 互換ポリシー | 追記先 |
|---|---|---|
| キー追加 | 後方互換維持 | `config-and-io.md` Special Notes |
| キー廃止 | 移行方針を明示 | `config-and-io.md` Special Notes |
| デフォルト変更 | 影響範囲を明示 | `runtime-flow.md` 併記 |

変更影響の確認先は `docs/architecture/README.md` の `Impact Map（変更時の影響先）` を参照。

## Operational Policy

- ランタイム/生成物は Git追跡しない
  - `build/`, `output/`, `result/`, `x64/`, `imgui.ini`, `build/*.i`
- 追跡方針はルート `.gitignore` で管理

## Special Notes

### Config互換性

#### 2026-02-25: 設定ファイルの相対パスは「設定ファイル配置ディレクトリ基準」で解決
- カテゴリ: Config互換性
- 背景: 実行ディレクトリ基準だと、ショートカット起動やGUI起動元の違いで同一configでも参照先が変わる。
- 判断: `LoadConfigFileInternal` で `configPath.parent_path()` を基準に解決する。
- 代替案: 常に `current_path()` 基準で解決する案。
- 影響範囲: config配布時の再現性向上。起動場所依存のパス不具合を抑制。
- 関連ファイル: `src/config/ConfigLoad.cpp`, `src/config/ConfigFileIO.cpp`

#### 2026-02-25: GUI状態保存はフラットキーJSONを維持
- カテゴリ: Config互換性
- 背景: GUI状態読込はregexベースで実装されており、入れ子構造へ変更すると既存読込ロジックと互換が崩れる。
- 判断: `SaveGUIStateStorageFile` は入れ子を使わないキー構造で保存し、読込実装と対にする。
- 代替案: 汎用JSONパーサ導入と同時にスキーマを全面改訂する案。
- 影響範囲: 既存GUI状態ファイルとの互換を維持しつつ、実装複雑度を抑制。
- 関連ファイル: `src/gui/GUIStateStorage.cpp`, `src/config/ConfigJsonUtils.cpp`

ADR記法は `docs/architecture/README.md` の `ADR Card Template` を使用。

#### 2026-03-04: TODO (auto-generated)
- カテゴリ: Config互換性
- 背景:
- 判断:
- 代替案:
- 影響範囲:
- 関連ファイル: include/config/ConfigResolver.h, include/config/SourceJson.h, include/config/SourceRegistry.h, src/config/ConfigFileIO.cpp, src/config/ConfigResolver.cpp

