# Architecture Index

最終更新: 2026-03-08
運用方針: 個人開発向けに「この1枚を正」とし、詳細は必要時のみ参照・更新する。

## 1. 最小把握（この章だけ読めばOK）

- 依存方向: `gui -> app -> core`
- `core` は UI 実装（ImGui/GLFW）へ依存しない
- Config 読込は `src/config/load/` に集約
- 実行入口は `Run(...)`（`include/AppCore.h` / `src/app/*`）
- 合成方式の境界は `docs/synth-methods/*` を正とする
- `SourceRegistry`（`include/config/SourceRegistry.h`）を source 契約（capability / lifecycle / schema）の正本とする

## 2. 詳細ドキュメント（必要時のみ）

| ファイル | 使うタイミング |
|---|---|
| `module-map.md` | 依存方向/責務境界を変更する時 |
| `runtime-flow.md` | Run/レンダ経路を変更する時 |
| `gui.md` | GUI責務や状態遷移を変更する時 |
| `config-and-io.md` | Config/I/O境界を変更する時 |

補足:
- 旧 `HANDBOOK.md` は重複整理のため `docs-archive/architecture/HANDBOOK-2026-03-08.md` へ移管済み。

## 3. 更新ルール（軽量）

1. まず本ファイルだけ更新する。
2. 詳細ファイルは、実際に差分が出た領域だけ更新する。
3. 更新後は `docs/STATUS.md` に要点だけ反映する。

## 4. ADR最小テンプレート

```md
### YYYY-MM-DD: タイトル
- 背景:
- 判断:
- 影響範囲:
- 関連ファイル:
```

## 5. Foundation契約の運用ルール

- 正本は `docs/synth-methods/foundation-contract.md` とする。
- `architecture` 側は要約のみ保持し、詳細手順は正本を参照する。
