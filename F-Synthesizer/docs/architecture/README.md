# Architecture Overview

最終更新: 2026-02-25

## Big Picture

```mermaid
flowchart LR
    H[HANDBOOK]
    A[module-map]
    B[runtime-flow]
    C[gui]
    D[config-and-io]
    H --> A --> B --> C --> D
```

## Scope

このディレクトリは、現行実装の構造を以下の観点で分割管理します。

1. モジュール責務と依存方向
2. 実行時データフロー
3. GUI構成
4. Config/I/O 境界

## Read Order

1. `docs/architecture/HANDBOOK.md`
2. `docs/architecture/module-map.md`
3. `docs/architecture/runtime-flow.md`
4. `docs/architecture/gui.md`
5. `docs/architecture/config-and-io.md`

## Quick Map

| ファイル | 主題 | 更新タイミング |
|---|---|---|
| `HANDBOOK.md` | 全体方針、原則、更新ルール | 原則・評価基準を変更した時 |
| `module-map.md` | レイヤー責務/依存方向 | `src/*` の依存方向や責務境界を変更した時 |
| `runtime-flow.md` | 実行経路・境界 | `src/app/Run*.cpp` や `include/AppCore.h` を変更した時 |
| `gui.md` | GUI分割・状態管理 | `src/gui/main/*` または `src/gui/pianoroll/*` の責務を変更した時 |
| `config-and-io.md` | Config読込/保存・I/O方針 | `src/config/*` や `src/io/*` の入出力仕様を変更した時 |

## Design Rules

- 依存方向は `gui -> app -> core` を維持する
- `core` は UI 実装（ImGui/GLFW）に依存しない
- Config 解析ロジックは `src/config/load/` に集約する
- GUI巨大化は `src/gui/main/` と `src/gui/pianoroll/` の責務分割で抑制する

## Impact Map（変更時の影響先）

| 変更対象 | 影響を確認する先 |
|---|---|
| `module-map.md` | `runtime-flow.md`, `gui.md`, `config-and-io.md` |
| `runtime-flow.md` | `module-map.md`, `gui.md`, `config-and-io.md` |
| `gui.md` | `runtime-flow.md`, `module-map.md` |
| `config-and-io.md` | `runtime-flow.md`, `module-map.md` |

## Documentation Operation Rule

- 記録カテゴリと追記先は `docs/architecture/HANDBOOK.md` の `6. 設計判断ログ導線（Special Notes）` に統一。
- 詳細の更新判断は本ファイルの `Quick Map` と `Impact Map` に従う。

## ADR Card Template

Special Notes に設計判断を記録する際のテンプレート:

```md
### YYYY-MM-DD: タイトル
- カテゴリ:
- 背景:
- 判断:
- 代替案:
- 影響範囲:
- 関連ファイル:
```
