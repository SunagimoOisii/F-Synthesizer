# 長期改変計画: Instrument Model から Preset 再編、GUI 再設計まで

この文書は、F-Synthesizer の次の大きな構造変更を AI コンテキスト圧縮後も引き継げるようにするための長期計画です。
現在は Phase 7-B により、`projectModel.v3` の save path を `ProjectModel` 正本へ寄せ、保存時に `AppConfig` 経路へ戻る依存を縮小しています。

## 基本方針

- `projectModel.v3` では Sound Card / Instrument を音色定義の正本にする。
- `channel` は音色そのものではなく、Instrument の割当、音量、パン、演奏設定、必要最小限の override を持つ器に寄せる。
- `ProjectModel`、`AppConfig`、`RenderConfig`、`GUIState`、`GUIProjectFacade` の責務を分け、保存形式、実行設定、GUI 一時状態、renderer 入力を混ぜない。
- GUI の大枠は `Play / Compose / Export / Advanced` を維持するが、中身の責務とデータ接続は作り直す。
- 破壊的変更は許容する。旧 format 互換読み込みは原則残さず、必要なら一時的な機械変換で次 format へ移す。
- 実音の意味的な preset 再設計は Instrument Model 導入後に行う。

## Phase 1: モデル境界整理

目的は、Instrument Model を入れる前に、保存モデル、実行モデル、GUI 状態、renderer 入力の境界を明確にすることです。
このフェーズでは境界入口の固定に限定し、GUI `.inl` 群の大規模置換や `projectModel.v3` 導入には入りません。

- `ProjectModel` は保存、preset、config の正本として扱う。
- `AppConfig` は CLI / GUI 実行境界の設定に限定する。
- `RenderConfig` は SynthEngine へ渡す実行用モデルにし、App 層で既定 render table を解決してから組み立てる。
- `GUIState` は互換用の集約型として扱い、永続 project 状態、画面一時状態、非同期実行状態、ログ状態へ分ける準備を進める。
- `GUIProjectFacade` を GUI と `ProjectModel` の変換点として育て、GUI 表示コードが JSON shape や preset 差分仕様へ直接依存しないようにする。
- 既存 `ChannelConfig` はすぐ捨てず、v3 への移行元として扱う。

完了条件:

- CLI / GUI / renderer が直接 JSON shape や preset 差分仕様に依存しない入口を持つ。
- `ProjectModel -> AppConfig` と `ProjectModel -> RenderConfig` の責務が混ざらない。
- 新規の保存項目を追加するときに `SynthEngine.h` や GUI `.inl` へ直足ししなくてよい判断基準がある。
- Phase 1 時点では `AppConfig -> RenderConfig` 準備 helper までに留め、`ProjectModel -> RenderConfig` 直変換は Phase 3 で扱う。

## Phase 2: ProjectModel v3 + Instrument Model 導入

目的は、音色定義の中心を legacy `project.channels` から Sound Card / Instrument へ移すことです。

- `format: "projectModel.v3"` を導入する。
- v3 config は Sound Card / Instrument を正本として持つ。
- channel は Instrument 参照、channel mix、演奏割当、必要最小限の channel-local override を持つ。
- legacy `source`、`layers`、`expressionMap` は Instrument 内部へ移すか、Instrument 生成時の互換入力として畳む。
- `projectModel.v2` は v3 loader の読み込み対象外にする。
- 既存 config / preset / sample は v3 へ機械移行済みとし、意味的な preset 再設計は Phase 4 で行う。
- Phase 2 の最小導入では `project.instruments.<id>.sound` に legacy `ChannelConfig` 相当を内包し、`project.channels.<ch>.instrumentId` から参照する。

完了条件:

- v3 config を読み込み、現行 renderer へ変換して短尺レンダーが通る。
- v2 config は明確に失敗する。
- 不正な Instrument field は path 付き error になる。
- legacy `layers` の意味は Instrument の内部構造または移行入力として説明できる。

## Phase 3: RenderConfig 変換層の確立

目的は、保存形式と SynthEngine の実行入力を切り離すことです。Phase 3 実装後は、CLI と GUI preview / export が `ProjectModel -> RenderConfig -> SynthEngine` の経路を使います。

- `ProjectModel v3 -> RenderConfig` の変換を正本にする。
- `Run` 境界は `ProjectModel` を受け取り、`AppConfig` は実行経路の正本から外す。
- Piano-roll preview などの一時入力は `RenderRuntimeOverrides` として project 保存形式から分離する。
- SynthEngine は Instrument、Sound Card、保存形式を直接知らない。
- `source`、`layers`、`expressionMap`、`effects` は render 用 config に展開してから renderer へ渡す。
- 音源固有の挙動は該当 renderer に置き、source 非依存の modulation と shared shaping は common engine path に置く。

完了条件:

- 音作りの実行経路が `ProjectModel -> RenderConfig -> SynthEngine` に一本化されている。
- CLI と GUI preview / export が同じ変換経路を使う。
- renderer 側に保存形式や GUI 状態の知識がない。

## Phase 4: Preset / Sound Card 再編

目的は、既存 preset を legacy layer の寄せ集めではなく、Instrument / Sound Card として意味的に再設計することです。Phase 4 実装後は実戦 `sound_*` を8カテゴリへ統合し、表示 metadata、推奨音域、macro hints を持つ Sound Card として扱います。

- 既存 `sound_*` preset を v3 Instrument / Sound Card として再定義する。
- カテゴリは `Lead / Guitar / Bass / Pad / Keys / Drums / SFX / Support` に固定する。
- `demo_*` は完成音色ではなく機能検証用として残す。
- 各 Instrument に用途、macro hints、source、補助レイヤー、推奨演奏域、表示名、説明、tag を持たせる。
- Play 表示対象と Advanced / demo 対象を明確に分ける。
- 品質基準は `docs/PRODUCT_POLICY.md` と `docs/PRESETS.md` に従う。

完了条件:

- 全実戦 Sound Card が v3 形式で、8カテゴリのいずれかに属している。
- Play で選んで短く試聴した時に用途が分かり、不要な hiss、耳につく aliasing、過剰 drive、用途不明な noise が目立たない。
- 代表カテゴリごとの CLI 短尺レンダーが非無音で、過大 peak がない。

## Phase 5: GUI Facade / GUIState 分割

目的は、GUI 表示コードから保存形式と音色契約の知識を減らし、Instrument / Sound Card 中心の操作へ移ることです。

- `GUIState` を永続 project 状態、画面一時状態、非同期実行状態、ログ / 通知状態へ分ける。
- GUI は `ProjectModel` を直接細かく編集せず、Facade / view model 経由で Instrument と channel assignment を操作する。
- preset 読み込み、Sound Card 選択、macro 編集、channel 割当の責務を GUI `.inl` から外す。
- GUI 状態保存は project 保存形式と混ぜない。
- Phase 5 実装後は、Sound Card catalog を `GUIPresetItem` view model として扱い、Play / Compose の主要導線、Layer2 macro、preview / export project 構築を `GUIProjectFacade` 経由へ寄せている。
- Advanced の詳細 Channel Editor には legacy `ChannelConfig` 直接編集が残る。これは Phase 7-B 以降の legacy cleanup で削る。

完了条件:

- Play / Compose の Sound Card 選択、macro 編集、channel assignment が Facade / view model 経由で動く。
- preview と export が同じ project / render 変換を使う。
- GUI state storage は `config/gui_state.json` の既存 schema を維持し、project 保存形式と混ざらない。

## Phase 6: GUI 再設計

目的は、Instrument / Sound Card 中心の利用体験へ GUI を合わせることです。Phase 6 実装後は、Play が Sound Card metadata の `recommendedRange` / `macroHints` を使い、Compose が Sound Card slot assignment と mix を曲作りの中心として扱います。

- `Play` は Sound Card 選択、4 macro、試聴、簡易割当を中心にする。
- `Compose` は MIDI、ピアノロール、ステップシーケンサー、channel assignment を中心にする。
- `Export` は WAV 出力と出力条件に集中する。
- `Advanced` は Instrument 詳細編集、Master FX、Mixer、検証 preset を扱う。
- 専門的な音色編集は Play に常時表示せず、Advanced または Play の Inspector 導線から開く。
- Sound Card の推奨 preview note と macro label / hint は Play の試聴と4 macro表示へ反映する。

完了条件:

- GUI から JSON や legacy `layers` を意識せず、Instrument / Sound Card 中心に操作できる。
- Sound Card 選択から preview までが短い導線で動く。
- Compose の channel assignment が preview と export の両方へ反映される。
- Advanced は詳細編集と検証 preset の到達先として残し、legacy direct edit の完全削除は Phase 7 で扱う。

## Phase 7: Legacy Cleanup

目的は、v3 移行後に残った古い契約を削除し、新規実装時に legacy 構造へ戻らない状態にすることです。

- Phase 7-A では一括削除ではなく、安全に切れる範囲を先に整理する。
- v2 専用の config shape、手書き JSON 出力、古い preset fallback、旧 GUI 状態の直接編集経路を段階的に削除する。
- `Architecture.md`、`PRESETS.md`、`channel_schema.md`、`OPERATIONS.md` を v3 前提へ同期する。
- legacy `channel/layers` へ直足しする実装経路をなくす。
- Phase 7-A 実装後は、CLI の自動 fallback は現行 Sound Card に統一し、GUI preset save は `project.instruments` / `project.channels` の v3 shape を JSON object 構築から出力する。
- Phase 7-B 実装後は、`SaveProjectModelFileInternal` が `ProjectModel` から直接 v3 JSON object を構築し、`project.instruments` / `project.channels` / `effects` を出力する。
- `AppConfig`、`ToAppConfig()`、`ProjectModelFromAppConfig()`、loader 側の AppConfig 補助、SynthEngine 入力としての `ChannelConfig`、Advanced Channel Editor の直接編集は 7-B では削らず、7-C 以降の対象に残す。

完了条件:

- 新規 Instrument や GUI 操作を追加するとき、legacy channel / layers を直接編集しなくてよい。
- v3 の保存、読み込み、レンダー、GUI 操作、preset check が標準経路になっている。
- 7-A の完了条件は、v2 互換を復活させず、旧 preset fallback と旧 docs 前提を消し、Play / Compose / preview / export が Phase 5/6 の Facade / view model 経由を維持すること。
- 7-B の完了条件は、ProjectModel save path が旧 channel sound shape や `project.channelMix` を出さず、保存済み v3 JSON を再ロードして render できること。

## 検証方針

各フェーズで最低限次を確認する。

- `.\scripts\check.ps1`
- `.\scripts\check.ps1 -RunRuntimeSmoke`
- `.\scripts\check_presets.ps1`
- 代表 preset / Sound Card の CLI 短尺レンダー
- GUI 手動確認: Play preview、Compose assignment、Export、Advanced editor

v3 導入時は追加で次を確認する。

- v2 config が明確に失敗する。
- v3 config が `ProjectModel -> RenderConfig -> SynthEngine` 経路で通る。
- 不正な Instrument field は path 付き error になる。

preset 再編時は追加で次を確認する。

- 全 committed Sound Card が v3 形式である。
- Play 表示対象と Advanced / demo 対象が分離されている。
- 代表カテゴリごとに非無音、過大 peak なし、用途に合う音色である。

GUI 再設計時は追加で次を確認する。

- Sound Card 選択から preview までが短い導線で動く。
- Compose の channel assignment が preview / export の両方へ反映される。
- GUI 状態保存が project 保存形式と混ざらない。

## 判断メモ

- `projectModel.v2` は Config / Schema 整理済みの足場だったが、Instrument Model の最終形ではない。
- `projectModel.v3` は Sound Card / Instrument を正本にするための破壊的変更として扱う。
- v2 互換を残さず、committed config / preset / sample は機械移行して v3 を正本にする。
- GUI の画面名は維持するが、内部の責務と data flow は v3 に合わせて作り直す。
