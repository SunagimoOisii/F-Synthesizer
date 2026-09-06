#include "project/ProjectModel.h"

namespace
{
std::string GeneratedInstrumentId(int ch)
{
    return "generated__ch" + std::to_string(ch);
}

InstrumentSoundConfig MakeWaveSound(WaveType wave, double amp, double atk, double dec, double sus, double rel)
{
    return InstrumentSoundConfig{ WaveformConfig{ wave }, amp, atk, dec, sus, rel };
}

DrumKitConfig MakeGmDrumKit()
{
    DrumKitConfig kit{};
    for (auto& d : kit.map)
    {
        d.type = DrumType::None;
    }

    DrumConfig kick{ DrumType::Kick };
    kick.gain = 0.6;
    kick.bodyFreq = 58.0;
    kick.bodyLevel = 0.9;
    kick.bodyDecaySec = 0.18;
    kick.pitchStart = 4.2;
    kick.pitchDecaySec = 0.06;
    kick.transientLevel = 0.22;
    kick.transientDecaySec = 0.008;
    kick.drive = 0.28;

    DrumConfig snare{ DrumType::Snare };
    snare.gain = 0.6;
    snare.bodyFreq = 220.0;
    snare.bodyLevel = 0.48;
    snare.snapLevel = 0.72;
    snare.snapDecaySec = 0.055;
    snare.hpCut = 700.0;
    snare.lpCut = 6000.0;
    snare.drive = 0.22;
    snare.noiseColor = static_cast<int>(NoiseType::Pink);

    DrumConfig hat{ DrumType::Hat };
    hat.gain = 0.15;
    hat.metalLevel = 0.46;
    hat.airLevel = 0.28;
    hat.decaySec = 0.045;
    hat.hpCut = 4000.0;
    hat.lpCut = 6000.0;
    hat.drive = 0.16;
    hat.noiseColor = static_cast<int>(NoiseType::Pink);

    kit.map[36] = kick;
    kit.map[38] = snare;
    kit.map[40] = snare;
    kit.map[42] = hat;
    kit.map[44] = hat;
    kit.map[46] = hat;
    kit.map[49] = hat;
    return kit;
}

InstrumentSoundConfig DefaultInstrumentSound(int ch)
{
    if (ch == 9)
    {
        return InstrumentSoundConfig{ MakeGmDrumKit(), 0.3, 0.001, 0.15, 0.1, 0.3 };
    }
    FmConfig fm{};
    fm.algorithm = 4;
    fm.ops[0].level = 0.18; fm.ops[1].level = 0.65;
    fm.ops[2].ratio = 2; fm.ops[2].level = 0.06; fm.ops[3].level = 0.3;
    return InstrumentSoundConfig{ fm, 0.2, 0.004, 0.25, 0.75, 0.2 };
}
} // namespace

ProjectModel DefaultProjectModel()
{
    ProjectModel model{};
    const std::filesystem::path projectRoot = FindProjectRootPath();
    model.midiPath.clear();
    model.wavPath = projectRoot / "output" / "test.wav";
    model.targetChannel = -1;
    model.initialSeconds = 6;
    model.bits = 16;
    model.sampleRate = 44100;
    model.extraReleaseSec = 0.3;

    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto channels = std::make_shared<std::array<ProjectChannelAssignment, 16>>();
    for (int ch = 0; ch < 16; ch++)
    {
        const std::string instrumentId = GeneratedInstrumentId(ch);
        InstrumentConfig instrument{};
        instrument.sound = DefaultInstrumentSound(ch);
        instrument.displayName = ch == 9 ? "基本ドラム" : "基本 FM";
        instruments->emplace(instrumentId, instrument);

        ProjectChannelAssignment channel{};
        channel.enabled = true;
        channel.instrumentId = instrumentId;
        (*channels)[ch] = channel;
    }
    model.instruments = instruments;
    model.projectChannels = channels;
    return model;
}
