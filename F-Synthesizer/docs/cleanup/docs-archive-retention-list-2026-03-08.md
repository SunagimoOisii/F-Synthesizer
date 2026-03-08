# docs-archive retention list (2026-03-08)

## Rule
- `keep`: 現行 docs から直接参照される、または運用上の参照先として明記される
- `delete`: 現行 docs から未参照で、履歴としての価値が相対的に低い
- `conditional`: 現行 docs から未参照だが、将来参照の可能性があるため最終版のみ保持推奨

## Inventory (50 files)
| file | decision | note |
| --- | --- | --- |
| `docs-archive/architecture/HANDBOOK-2026-03-08.md` | keep | `docs/architecture/README.md` から直接参照 |
| `docs-archive/cleanup/archive/2026-03-phase1-6/core-audit.md` | keep | `docs/cleanup/deletion-policy.md` から直接参照 |
| `docs-archive/cleanup/archive/2026-03-phase1-6/docs-audit.md` | keep | `docs/cleanup/deletion-policy.md` から直接参照 |
| `docs-archive/cleanup/archive/2026-03-phase1-6/gui-audit.md` | keep | `docs/cleanup/deletion-policy.md` から直接参照 |
| `docs-archive/cleanup/archive/2026-03-phase1-6/midi-audit.md` | keep | `docs/cleanup/deletion-policy.md` から直接参照 |
| `docs-archive/cleanup/archive/2026-03-phase1-6/scripts-audit.md` | keep | `docs/cleanup/deletion-policy.md` から直接参照 |
| `docs-archive/comment-migration/COMMENT_MIGRATION_BASELINE.md` | conditional | まとめ基準の最終スナップショットとして保持候補 |
| `docs-archive/comment-migration/COMMENT_MIGRATION_PHASES.md` | delete | 未参照。BASELINE保持時は冗長 |
| `docs-archive/comment-migration/COMMENT_MIGRATION_TASKS.md` | delete | 未参照。BASELINE保持時は冗長 |
| `docs-archive/foundation-migration/PARAMETER_SMOOTHING_PHASES.md` | conditional | 仕様履歴の最終版として保持候補 |
| `docs-archive/foundation-migration/PARAMETER_SMOOTHING_TASKS.md` | delete | 未参照。PHASES保持時は冗長 |
| `docs-archive/gui-help/gui-help-phase-plan.md` | delete | 未参照。`docs/GUI_REQUIREMENTS.md` へ昇格済み前提 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v2.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v3.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v4.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v5.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v6.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v7.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_PHASES_v8.md` | delete | 未参照。受け入れ手順は別途 keep |
| `docs-archive/gui-migration/GUI_MIGRATION_PLAN.md` | delete | 未参照。旧計画 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v2.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v3.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v4.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v5.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v6.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v7.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_MIGRATION_TASKS_v8.md` | delete | 未参照。旧版 |
| `docs-archive/gui-migration/GUI_V7_ACCEPTANCE_TEST.md` | keep | `docs/GUI_REQUIREMENTS.md` から直接参照 |
| `docs-archive/gui-migration/GUI_V8_ACCEPTANCE_TEST.md` | keep | `docs/GUI_REQUIREMENTS.md` / `docs/OPERATIONS.md` から直接参照 |
| `docs-archive/gui-requirements/GUI_REQUIREMENTS_HISTORY.md` | keep | `docs/GUI_REQUIREMENTS.md` でディレクトリ参照 |
| `docs-archive/migration/migration_refactor.md` | conditional | refactor履歴の移行要点。必要時参照の可能性あり |
| `docs-archive/migration/migration_v2.md` | delete | 未参照。旧移行ガイド |
| `docs-archive/migration/migration_v3.md` | delete | 未参照。旧移行ガイド |
| `docs-archive/migration/migration_v4.md` | delete | 未参照。旧移行ガイド |
| `docs-archive/migration/migration_v5.md` | delete | 未参照。旧移行ガイド |
| `docs-archive/piano-roll-migration/PIANO_ROLL_ACCEPTANCE_TEST.md` | conditional | 将来の回帰確認用に最小保持候補 |
| `docs-archive/piano-roll-migration/PIANO_ROLL_DIRECT_INTERACTION_PHASES.md` | delete | 未参照。旧フェーズ履歴 |
| `docs-archive/piano-roll-migration/PIANO_ROLL_DIRECT_INTERACTION_TASKS.md` | delete | 未参照。旧タスク履歴 |
| `docs-archive/piano-roll-migration/PIANO_ROLL_PHASES.md` | delete | 未参照。旧フェーズ履歴 |
| `docs-archive/piano-roll-migration/PIANO_ROLL_TASKS.md` | delete | 未参照。旧タスク履歴 |
| `docs-archive/refactor/REFACTOR_PHASES.md` | conditional | 大規模整理の背景として保持候補 |
| `docs-archive/refactor/REFACTOR_TASKS.md` | delete | 未参照。PHASES保持時は冗長 |
| `docs-archive/synth-methods/foundation-audit-2026-03-05.md` | keep | `docs/synth-methods/foundation-contract.md` から直接参照 |
| `docs-archive/synth-migration/WAVE_DRUM_AB_RUNBOOK.md` | keep | `docs/synth-methods/waveform.md` / `drum-drumkit.md` から直接参照 |
| `docs-archive/synth-migration/WAVE_DRUM_BRUSHUP_PHASES.md` | delete | 未参照。旧検討履歴 |
| `docs-archive/synth-migration/WAVE_DRUM_BRUSHUP_TASKS.md` | delete | 未参照。旧検討履歴 |
| `docs-archive/synth-migration/WAVEFORM_PHASES_v0.md` | delete | 未参照。初期版 |
| `docs-archive/synth-migration/WAVEFORM_TASKS_v0.md` | delete | 未参照。初期版 |

## Summary
- keep: 11
- conditional: 5
- delete: 34
