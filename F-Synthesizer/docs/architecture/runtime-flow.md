# Runtime Flow

最終更新: 2026-02-24

## End-to-End

```mermaid
flowchart LR
    MIDI[.mid]
    Parser[MIDIParser]
    Seq[Sequencer]
    Pipe[MidiPipeline]
    App[app::run::RunMain]
    Core[RenderGateway]
    Synth[SynthEngine]
    Buffer[SoundData]
    Writer[io/Writer]

    MIDI --> Parser --> Seq --> Pipe --> App --> Core --> Synth --> Buffer --> Writer
```

## Sequence (Run)

```mermaid
sequenceDiagram
    participant U as Caller
    participant R as RunMain
    participant P as MidiPipeline
    participant G as RenderGateway
    participant S as SynthEngine
    participant W as Writer

    U->>R: Run(config, options)
    R->>P: Build timeline
    R->>G: Render request
    G->>S: Synthesize
    S-->>G: SoundData
    G-->>R: Render result
    alt Export
        R->>W: Save wav
    else Preview
        R-->>U: Return preview buffer
    end
```

## Mode Branch

```mermaid
flowchart TD
    Start[Run Start]
    Mode{RenderOptions.mode}
    Export[Export: Save WAV]
    Preview[Preview: Return Buffer]
    Cancel{ShouldCancel}
    Stop[Cancel/End]

    Start --> Mode
    Mode --> Export
    Mode --> Preview
    Export --> Cancel
    Preview --> Cancel
    Cancel --> Stop
```

## Run Boundary

| 項目 | 位置 |
|---|---|
| 公開API | `include/AppCore.h` |
| 実行本体 | `src/app/RunExecution.cpp` |
| 保存処理 | `src/app/RunSave.cpp` |
| 既定値適用 | `src/app/RunDefaults.cpp` |
| 統計処理 | `src/app/RunStats.cpp` |

## Before / After (Execution Path)

| 観点 | Before | After |
|---|---|---|
| 実行経路 | GUI/CLI説明が分離しやすい | 共通Run経路として説明統一 |
| モード説明 | 文だけで分かりにくい | `Mode Branch` 図で可視化 |
| 中断仕様 | 注釈中心 | `ShouldCancel` を明示 |

## MIDI Pipeline

| Component | 役割 |
|---|---|
| `src/midi/MIDIParser.cpp` | MIDIイベント解析 |
| `src/midi/Sequencer.cpp` | tick/sample変換 |
| `src/midi/MidiPipeline.cpp` | app層向けの統合出力 |

## Impact Map (When This Changes)

```mermaid
flowchart LR
    RT[runtime-flow.md]
    MM[module-map.md]
    GUI[gui.md]
    CIO[config-and-io.md]

    RT --> MM
    RT --> GUI
    RT --> CIO
```

## Special Notes

### 実行フロー/キャンセル

- 現在、特記すべき例外なし。

### MIDI時間変換

- 現在、特記すべき例外なし（tick/sample変換ポリシー変更時に追記）。

### ADR Card (Template)

| 項目 | 内容 |
|---|---|
| 背景 | |
| 判断 | |
| 代替案 | |
| 採用理由 | |
| 影響範囲 | |
| 関連ファイル | |
