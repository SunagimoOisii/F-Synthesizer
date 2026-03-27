# レイヤー別コード分析（一時メモ）

> **破棄前提** — リファクタ計画立案後は削除する。

---

## GUI（src/gui, include/gui） — 約 4,900 LOC

### 特性
- ImGui を用いたピアノロール・チャンネルエディタ・プリセット管理を担う最大レイヤー
- `GUIState` が全 GUI 状態を一括保持し、`GUIRunObserver` による非同期 Run 観測を内包
- `std::future<int> runFuture` でバックグラウンド実行、mutex ログベクタでスレッドセーフ通知

### 問題点
| 問題 | 詳細 |
|------|------|
| God Object | `GUIState` が 50+ フィールドを包含。可読性・テスト困難 |
| 長大ファイル | `GUIChannelEditor.cpp` 988行、`GUIActions.cpp` 856行。責務混在 |
| 型の不統一 | `char[1024]` パスバッファと `std::string` が混在 |
| 描画とロジックの混在 | ImGui 制御フロー内にビジネスロジックが散在 |
| ソース追加コスト | 新ソース種別ごとに `GUIChannelEditor` の switch が増殖 |

---

## APP（src/app, include/app） — 約 910 LOC

### 特性
- CLI 解析 → Config 解決 → Run 実行の直線的フロー
- `RunStats` で統計収集、`RunDefaults` でデフォルト値管理
- 全カテゴリ中、責務境界が最も明確でシンプル

### 問題点
| 問題 | 詳細 |
|------|------|
| ハードコードデフォルト | `RunDefaults.cpp` 187行に数値が直書き。設定化困難 |
| エラーリカバリ皆無 | 実行失敗後の再試行・部分リカバリの仕組みがない |

---

## CORE（src/core, include/core） — 約 100 LOC

### 特性
- APP/GUI と SynthEngine を繋ぐ薄いゲートウェイ層
- `SoundData` がモノ/ステレオ両対応バッファを保持

### 問題点
| 問題 | 詳細 |
|------|------|
| モノ/ステレオ共存 | `data`（モノ）と `dataL/dataR`（ステレオ）が同一 struct に共存。`channels` 値で使い分けが必要 |
| 境界契約が薄い | ゲートウェイが薄すぎて SynthEngine との契約が暗黙的 |
| マルチバス非対応 | バス追加には `SoundData` の抜本的修正が必要 |

---

## SynthEngine（src/SynthEngine, include/SynthEngine） — 約 3,730 LOC

### 特性
- Voice 管理（SoA パターン・13 本の並列ベクタ）、サンプル生成、モジュレーション評価を担う中核
- `std::variant` + `std::visit` によるソース種別ディスパッチ
- split-rate モジュレーション（pitch 系は高頻度、amp/filter 系は低頻度更新）

### 問題点
| 問題 | 詳細 |
|------|------|
| Renderer 肥大 | `Renderer.cpp` 736行。visit ネスト + switch が深く追跡困難 |
| split-rate 複雑性 | `ModulationRuntimeState` に 12 個のキャッシュ変数。不一致バグのリスク |
| Voice state 膨張 | arpeggio / hardsync / ringmod 追加で struct が肥大化傾向 |
| ModRoute 上限 | 8 スロット固定。ルーティング拡張の天井 |
| フィルタ再計算タイミング | dirty flag の有無がコードを追わないと不明 |

---

## MIDI（src/midi, include/midi） — 約 1,050 LOC

### 特性
- 外部ライブラリ不使用の自前 SMF バイナリパーサ
- tick→sample 変換・テンポ補間・サステインペダル処理・重複ノート追跡を担当
- GUI/APP/SynthEngine に対して無依存（最も分離されたレイヤー）

### 問題点
| 問題 | 詳細 |
|------|------|
| 単一ファイルへの責務集中 | `MIDIParser.cpp` 455行がフォーマット解析とイベント構築を兼任 |
| エラーハンドリング限定的 | トラック外メタイベントや不正データへの対処が一部のみ |
| ステップ分割テスト困難 | `MIDIBuildOutput` に全結果が集約され、段階的な単体テストがしにくい |

---

## CONFIG（src/config, include/config） — 約 3,330 LOC

### 特性
- JSON の読み書き、ソース種別の能力/ライフサイクル管理、設定マージを担う
- 外部JSONライブラリ不使用で `std::regex` による独自パース
- ベース設定 + プリセットの2段階ロードでマージ

### 問題点
| 問題 | 詳細 |
|------|------|
| **最大の技術的負債** | `LoadSource.cpp` 1,101行にパース・バリデーション・スキーマ検証が混在 |
| regex JSON パース | 不正 JSON でサイレント失敗しやすく、エラーリカバリが弱い |
| スキーマが C++ ハードコード | 宣言的スキーマファイルがなく、追加・変更時に見落としが起きやすい |
| 暗黙的マージ | `LoadConfigFile` 二度呼びによる上書きマージが直感に反しやすい |
| 新ソース追加コスト | `LoadSource` 分岐 + `SourceRegistry` 登録 + スキーマ追加の3点セット必須 |

---

## IO（src/io, include/io） — 約 300 LOC

### 特性
- WAV ファイルの自前バイナリ書き出しとプラットフォーム別パス処理
- `WAVWriteError` による詳細診断構造体
- シンプルAPIと診断付きAPIの2種類を提供

### 問題点
| 問題 | 詳細 |
|------|------|
| バリデーション薄 | ビット深度・チャンネル数の検証が書き込み直前のみ |
| プロジェクト依存パス | `FindProjectRootPath` が固有ディレクトリ名に依存（移植性低） |
| プラットフォーム分岐 | `#ifdef` 直書きのため新環境追加が煩雑 |

---

## synth ヘルパー（src/synth, include/synth） — 約 330 LOC

### 特性
- ADSR エンベロープと波形/ノイズ生成の基礎ユーティリティ
- SynthEngine からのみ使用される内部ライブラリ的な位置づけ
- スレッドローカルノイズ状態（スレッド間発散を設計上許容）

### 問題点
| 問題 | 詳細 |
|------|------|
| ADSR カーブ | 線形補間のみ。`ModEnvelopeConfig.curve` 対応は SynthEngine 側で閉じており、ここには非反映 |

> **全カテゴリ中、最も品質が安定している。深刻な問題なし。**

---

## サマリ

| カテゴリ | LOC | 可読性 | 拡張性 | 保守性 | 最優先問題 |
|---------|-----|--------|--------|--------|-----------|
| GUI | 4,900 | △ | △ | △ | God Object / 長大ファイル |
| APP | 910 | ○ | ○ | ○ | デフォルト値ハードコード |
| CORE | 100 | ○ | △ | △ | モノ/ステレオ共存 |
| SynthEngine | 3,730 | △ | ○ | △ | Renderer 肥大 / split-rate 複雑性 |
| MIDI | 1,050 | △ | △ | △ | MIDIParser の責務過多 |
| CONFIG | 3,330 | × | △ | × | LoadSource 1100行 / regex JSON |
| IO | 300 | ○ | △ | ○ | バリデーション薄 |
| synth | 330 | ◎ | ◎ | ◎ | なし |

### 改善優先度

1. **CONFIG** — `LoadSource.cpp` 分割 + `nlohmann/json` 等の正規 JSON パーサ導入が最大効果
2. **GUI** — `GUIState` の責務分割、`GUIChannelEditor` のソース別ファイル分離
3. **SynthEngine** — `Renderer.cpp` をソース種別単位のサブファイルへ分割
4. **MIDI** — `MIDIParser.cpp` をフォーマット解析とイベント構築に分離
