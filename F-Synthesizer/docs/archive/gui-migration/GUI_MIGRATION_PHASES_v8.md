# GUI_MIGRATION_PHASES_v8

v8 は v7 の操作骨格を維持したまま、運用性・見通し・将来拡張余地を強化するフェーズ。

前提:
- v7 で「初心者が迷わず音を出す/書き出す」導線を満たしていること。
- v8 では体験を壊さず、必要な複雑性のみ段階的に追加する。

## GUI_V8_PHASE_A_ASSIGNMENT_AND_MIX_POLISH

Status: DONE

- Soundタブ重複責務の縮退（先行実施）
  - `MIDI Path / Output Path / Target Channel` の入力責務をMusicへ一本化
  - Soundは `音色作成 / 音色試聴` に集中し、楽曲出力設定UIを持たない
- Music側の音色割当UIを磨き込み（一覧性/編集性）
- 16chミックスUIの可読性と操作効率を改善
- `PR Channel`・割当・出力対象の関係を視覚的に明確化

完了条件:
- `Sound` と `Music` の責務重複がUI上で解消される
- 割当/ミックス/出力対象の誤操作が減る
- 編集から試聴までの往復が速い

## GUI_V8_PHASE_B_REFERENCE_AND_SAVE_POLISH

Status: DONE

- `snapshot` 既定運用を前提に保存導線を改善
- `link` を詳細設定として段階的に公開
- `Save Project` / `Save All` の役割をUI上でさらに明確化

完了条件:
- 保存対象の境界（SoundAsset/MusicProject/Workspace）を誤認しない
- 保存挙動が直感的に理解できる
- 再現性を崩さずに運用できる

## GUI_V8_PHASE_C_DRUM_AND_CHANNEL_SPECIALIZATION

Status: DONE

- ドラムch特別扱いのUI/文言を改善
- ch10想定運用時の編集導線を短縮
- 将来の「1ch=複数音色」拡張に備えた内部構造整理（UIは固定維持、過剰実装を避けて段階導入）

完了条件:
- ドラム編集の迷いが減る

## GUI_V8_PHASE_D_ERROR_AND_RECOVERY_POLISH

Status: DONE

- 主要エラーごとの修正アクションを具体化
- ダイアログ文言を初心者向けに最適化
- 画面内エラー導線の一貫性を改善

完了条件:
- エラー発生時に次操作を迷わない
- リカバリ成功率が上がる

## GUI_V8_PHASE_E_ACCEPTANCE_METRICS_AND_RELEASE

Status: DONE

- v8受け入れ基準（初回導線/反復試行/再現性）を計測可能化
- 回帰テストと運用手順を更新
- v8リリース判定を実施
  - 手動受け入れ: `docs/GUI_V8_ACCEPTANCE_TEST.md`
  - 自動回帰: `scripts/gui_smoke.ps1`

完了条件:
- v7より体験品質が定量/定性で改善している
