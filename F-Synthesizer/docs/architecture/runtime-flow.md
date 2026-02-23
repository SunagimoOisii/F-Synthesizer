# Runtime Flow

最終更新: 2026-02-23

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

## Run Boundary

| 項目 | 位置 |
|---|---|
| 公開API | `include/AppCore.h` |
| 実行本体 | `src/app/RunExecution.cpp` |
| 保存処理 | `src/app/RunSave.cpp` |
| 既定値適用 | `src/app/RunDefaults.cpp` |
| 統計処理 | `src/app/RunStats.cpp` |

## Preview / Export

| Mode | 振る舞い |
|---|---|
| `Export` | WAV保存を伴う通常実行 |
| `Preview` | GUI内プレビュー用実行 |

キャンセルは `IRunObserver::ShouldCancel()` を介してレンダ中断する。

## MIDI Pipeline

- `src/midi/MIDIParser.cpp`: MIDIイベント解析
- `src/midi/Sequencer.cpp`: tick/sample変換
- `src/midi/MidiPipeline.cpp`: app層向けの統合出力

## Special Notes

この節に、実行順序・中断処理・パフォーマンス最適化などの特殊対応を追記する。
