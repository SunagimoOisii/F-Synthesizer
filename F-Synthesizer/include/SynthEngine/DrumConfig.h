#pragma once

enum class DrumType
{
    None,
    Kick,
    Snare,
    Hat,
    Tom,
    Rim,
    Clap,
    Crash,
    Ride
};

// 単発ドラム音源の設定。
// type別に使う項目が異なり、0/負値は「内部既定値を使う」意味を持つ。
struct DrumConfig
{
    bool operator==(const DrumConfig&) const = default;
    // DrumType ごとに参照する項目が異なるため、未使用値は 0/負値で未指定を表す。
    DrumType type = DrumType::None;
    double gain = 1.0;
    double bodyFreq = 0.0;
    double bodyLevel = 0.0;
    double bodyDecaySec = 0.0;
    double pitchStart = 0.0;
    double pitchDecaySec = 0.0;
    double transientLevel = 0.0;
    double transientDecaySec = 0.0;
    double noiseLevel = 0.0;
    double snapLevel = 0.0;
    double snapDecaySec = 0.0;
    double metalLevel = 0.0;
    double airLevel = 0.0;
    double decaySec = 0.0;
    double hpCut = 0.0;
    double lpCut = 0.0;
    double drive = 0.0;
    int noiseColor = 0;
    double velocityToTone = 0.0;
    double velocityToDecay = 0.0;
    double humanizePitchCents = 0.0;
    double humanizeDecayPct = 0.0;
    // one-shot アタック保護のため smoothing は非対応（契約上 waveform 専用）。
};

struct DrumBusConfig
{
    bool operator==(const DrumBusConfig&) const = default;
    bool enabled = false;
    double level = 1.0;
    double attackTrim = 0.0;
    double sustainLift = 0.0;
    double glue = 0.0;
    double presenceCut = 0.0;
    double lowTighten = 0.0;
    double roomSend = 0.0;
    double driveTrim = 0.0;
};
