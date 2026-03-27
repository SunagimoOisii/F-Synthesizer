# 改善案（残カテゴリ: IO / APP / CORE）（一時メモ）

> **破棄前提** — 採否判断後は削除する。
> CONFIG(A) / GUI(B) / SynthEngine(C) / MIDI(D) は実施済みのため省略。

---

## 改善案 E: IO — Writer 出力精度強化 + バリデーション

### 現状の問題

| 問題 | 深刻度 | 詳細 |
|------|--------|------|
| **bits 設定が無視される** | バグ | `sound.bits` に 24 が入っていても `Writer.cpp` は常に PCM16 で書き出す |
| バリデーションが書き込み直前のみ | 保守性 | bits / channels / fs の異常値がファイルを開いてから初めてエラーになる |
| モノラルフォールバックの意図が不明 | 保守性 | `dataL/dataR` が空のとき `data[i]` で代替する分岐があるが、正常状態か異常状態か不明 |

### 改善ステップ

#### ステップ 1: 24bit PCM 書き出し対応（`Writer.cpp`）

```cpp
// 現状: bits に関係なく int16_t に変換
// 改善後: sound.bits で分岐

if (sound.bits == 24)
{
    // 3バイト little-endian
    int32_t s = static_cast<int32_t>(
        std::clamp(sample * 8388607.0, -8388608.0, 8388607.0));
    buf[0] = s & 0xFF;
    buf[1] = (s >> 8) & 0xFF;
    buf[2] = (s >> 16) & 0xFF;
}
else // 16bit
{
    int16_t s = static_cast<int16_t>(
        std::clamp(sample * 32767.0, -32768.0, 32767.0));
    // ...
}
```

WAV ヘッダの `wBitsPerSample` / `nBlockAlign` / `nAvgBytesPerSec` も `sound.bits` に連動させる。

#### ステップ 2: 早期バリデーション（`Writer.cpp` 冒頭、ファイルを開く前）

| 条件 | エラーコード |
|------|------------|
| `sound.bits != 16 && sound.bits != 24` | `"wav_invalid_bits"` |
| `sound.fs <= 0` | `"wav_invalid_samplerate"` |
| `sound.channels < 1 \|\| sound.channels > 2` | `"wav_invalid_channels"` |
| `sound.length <= 0` | `"wav_empty"` |

#### ステップ 3: モノラルフォールバック分岐の明示化

`Writer.cpp` の「`dataL/dataR` が空なら `data[i]` にフォールバック」分岐を、
`channels == 1` の正規パスとして `GetSampleL/R()` ヘルパー経由に統一する（後述 改善案 G と連携）。

### 効果
- `bits=24` 設定が実際に 24bit WAV として出力される（設定と出力の不一致が解消）
- 無効な設定が早期エラーになり診断が明確になる

### リスクと対策
- 24bit 出力は互換性が高いが、受け取り側ソフトで稀に問題が出る可能性がある
- 変更前後でファイルバイト列が変わるため smoke テスト（再生確認）が必須

---

## 改善案 F: APP — バグ修正 + 保守性改善

### 現状の問題

| 問題 | 深刻度 | 詳細 |
|------|--------|------|
| **LocalFree リーク** | バグ | `CLI.cpp` の `ParseWideArgs` で引数不足時に途中 `return`、`LocalFree(wargv)` が呼ばれない |
| **重複ログ** | バグ | `RunExecution.cpp` で `BuildMIDIPipeline` 失敗時に「No note events found」が 2 回出力される |
| **Build Marker ハードコード** | コード品質 | `"2026-02-21-save-debug-v1"` が `RunExecution.cpp` に埋め込まれたまま |
| プロジェクトルート探索の脆弱性 | 保守性 | `FindProjectRootInternal` が `F-Synthesizer.vcxproj` ファイル名に依存（リネームで壊れる） |
| MAX_PATH 制限 | 保守性 | `GetModuleFileNameW` が 260 文字制限（日本語パス + 深いネストで超過リスク） |

### 改善ステップ

#### ステップ 1: LocalFree リーク修正（`CLI.cpp`）

RAII ガードで `wargv` を管理する:

```cpp
struct LocalFreeGuard
{
    LPWSTR* ptr = nullptr;
    ~LocalFreeGuard() { if (ptr) LocalFree(ptr); }
};

// 使用例
int wargc = 0;
LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
LocalFreeGuard guard{ wargv };
// 以降どこで return しても LocalFree が保証される
```

#### ステップ 2: 重複ログ削除（`RunExecution.cpp`）

`BuildMIDIPipeline` が `err` を設定して `false` を返すとき既にログ出力している場合、
呼び出し側の `events.empty()` チェックによるログを削除するか、`err` が空でなければスキップする。

#### ステップ 3: Build Marker 除去（`RunExecution.cpp`）

```cpp
// 以下の行を削除する
observer->OnLogLine("[Build] 2026-02-21-save-debug-v1");
```

開発用マーカーとして残す場合は `#ifdef _DEBUG` で囲む。

#### ステップ 4: プロジェクトルート探索の改善（`RunDefaults.cpp`）

現在 `F-Synthesizer.vcxproj` の存在をマーカーに使っているが、
配布・リネーム時に壊れる。代替案:

```cpp
// 環境変数で上書き可能にし、vcxproj 依存を取り除く
if (const char* envRoot = std::getenv("FSYNTH_ROOT"))
{
    return std::filesystem::path(envRoot);
}
// フォールバック: 実行ファイルの親ディレクトリを固定使用
```

または設定ファイル (`default.json`) の存在をマーカーとして使うことで、
ビルドシステム固有ファイルへの依存を排除する。

#### ステップ 5: MAX_PATH 対応（`RunDefaults.cpp`）（低優先度）

```cpp
// 固定バッファ(MAX_PATH) → 動的確保に変更
DWORD needed = GetModuleFileNameW(nullptr, nullptr, 0);
std::wstring buf(needed, L'\0');
GetModuleFileNameW(nullptr, buf.data(), needed);
```

### 効果
- ステップ 1〜3 は変更量が小さく即修正可能
- ステップ 4 はポータビリティ向上（将来のディレクトリ構成変更への耐性）

### 着手順の推奨
ステップ 1 → 2 → 3 は1コミットでまとめて実施（バグ修正 PR）。
ステップ 4 → 5 は別 PR（設計変更を含むため）。

---

## 改善案 G: CORE — SoundData モノ/ステレオ設計の明確化

### 現状の問題

`SoundData` が `data`（モノ互換）と `dataL/dataR`（ステレオ）を同時に持ち、
`channels` の値で使い分けが必要な構造になっている。

```cpp
struct SoundData
{
    std::vector<double> data;   // モノラル互換 (L+R)*0.5
    std::vector<double> dataL;
    std::vector<double> dataR;
    int channels;               // 1 or 2
};
```

| 問題 | 詳細 |
|------|------|
| どの `data*` が有効か不明 | `channels` を読まないと判断できない |
| `data` と `dataL/dataR` の同期責任が不明確 | どちらが source-of-truth か定義されていない |
| Writer のフォールバック分岐 | `dataL/dataR` が空なら `data[i]` で代替 — 異常状態を正常扱いしている |

### 改善ステップ

#### ステップ 1（最小変更）: サンプルアクセスを関数化（`AudioBuffer.h`）

```cpp
// SoundData にインラインヘルパーを追加
double SampleL(size_t i) const
{
    return (channels >= 2) ? dataL[i] : data[i];
}
double SampleR(size_t i) const
{
    return (channels >= 2) ? dataR[i] : data[i];
}
```

`Writer.cpp` の `dataL/dataR` 直接アクセスをこれで統一し、フォールバック分岐を削除する。
`TrimPreviewSoundByDuration` など他の `data*` 直接アクセス箇所も同様に統一する。

#### ステップ 2（将来設計）: 書き込み先の一本化

`RenderWithEngine` から `SoundData` へ書き込む際、
`channels == 1` なら `data` のみ、`channels == 2` なら `dataL/dataR` のみに書き込むルールを明示し、
「一方が常に空である」状態を正常として設計する。

これにより `Writer.cpp` のフォールバック分岐が不要になる（ステップ 1 と合わせて完全に除去できる）。

### 効果
- ステップ 1: 変更量は `AudioBuffer.h` に 2 メソッド追加 + `Writer.cpp` 数行の置き換えのみ
- モノラル/ステレオの判断が一箇所に集約され、読み手の認知負荷が下がる
- 改善案 E（Writer 改善）と同時実施すると相乗効果がある

---

## synth ヘルパー: 変更不要

分析資料の通り「全カテゴリ中、最も品質が安定している」ため積極的な変更は不要。

潜在的な将来拡張:
- `Envelope.cpp` の `ModEnvelopeConfig.curve` 対応（現在は SynthEngine 側で閉じており `StepADSR` には未反映）
  — ただし SynthEngine 側との連携変更が必要なため、単独では着手しない。

---

## 全体俯瞰

| 案 | 対象 | 変更の種類 | 修正箇所の広がり | 推定リスク |
|----|------|----------|-----------------|-----------|
| E-1 | IO / 24bit 出力 | バグ修正 + 機能追加 | Writer.cpp のみ | 低（smoke テスト必須） |
| E-2 | IO / 早期バリデーション | 保守性 | Writer.cpp のみ | 低 |
| F-1 | APP / LocalFree リーク | バグ修正 | CLI.cpp のみ | 低 |
| F-2 | APP / 重複ログ | バグ修正 | RunExecution.cpp のみ | 低 |
| F-3 | APP / Build Marker 除去 | コード品質 | RunExecution.cpp 1行 | 低 |
| F-4 | APP / プロジェクトルート | 保守性 | RunDefaults.cpp | 低〜中 |
| F-5 | APP / MAX_PATH | 保守性 | RunDefaults.cpp | 低 |
| G-1 | CORE / SoundData ヘルパー | 設計整理 | AudioBuffer.h + Writer.cpp | 低 |
| G-2 | CORE / 書き込み先一本化 | 設計整理 | RenderWithEngine + Writer | 中 |

### 着手順の推奨

1. **F-1 + F-2 + F-3**（バグ修正セット、1 PR でまとめて実施）
2. **E-1 + E-2**（IO 出力改善、smoke テストとセット）
3. **G-1**（SoundData ヘルパー、E-2 と同時実施で相乗効果）
4. **F-4 + F-5**（保守性改善、別 PR）
5. **G-2**（設計変更のため十分な検証後）
