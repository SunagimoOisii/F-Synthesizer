#pragma once
#include <chrono>
#include <cmath>
#include "synth/YmfmVoice.h"
#include "SynthEngine/SynthEngine.h"
#include "midi/MIDIReader.h"

inline void CheckAudioIntegration()
{
    constexpr int rate = 44100;
    FmConfig fm{};
    fm.algorithm = 7;
    for (auto& op : fm.ops) { op.level = 0; op.levelEnv.attackSec = 0.001; op.levelEnv.sustainLevel = 1; }
    fm.ops[3].level = 1;
    for (int chip = 0; chip < 2; ++chip)
    {
        fm.chip = chip;
        YmfmVoice voice(rate, chip);
        double previous = 0, firstCrossing = 0, lastCrossing = 0, energy = 0;
        int crossings = 0;
        for (int i = 0; i < rate / 2; ++i)
        {
            const double value = voice.Sample(fm, 440, 1, false);
            Require(std::isfinite(value), "FM generated a non-finite sample");
            if (i > rate / 10)
            {
                energy += value * value;
                if (previous <= 0 && value > 0)
                {
                    const double crossing = i - value / (value - previous);
                    if (crossings++ == 0) firstCrossing = crossing;
                    lastCrossing = crossing;
                }
            }
            previous = value;
        }
        const double hz = (crossings - 1) * rate / (lastCrossing - firstCrossing);
        std::cout << "ymfm chip " << chip << ": A4=" << hz << " Hz, energy=" << energy << '\n';
        Require(energy > 1 && crossings > 10 && std::abs(hz - 440) < 3, "FM tuning or output failure");
    }
    auto mailbox = std::make_shared<LiveRenderMailbox>();
    auto settings = std::make_shared<LiveRenderSettings>();
    fm.chip = 0;
    auto& sound = settings->sounds[0];
    sound.source = fm; sound.amp = 0.2;
    sound.attackSec = 0.001; sound.decaySec = 0.01; sound.sustainLevel = 1; sound.releaseSec = 0.03;
    mailbox->store(settings);
    MIDIEvent on{}; on.type = MIDIEventType::Note; on.channel = 0; on.noteNumber = 69;
    on.velocity = 127; on.isNoteOn = true; on.noteInstanceID = 1;
    MIDIEvent off = on; off.sample = rate; off.isNoteOn = false;
    std::vector<MIDIEvent> events{on, off};
    std::vector<double> samples;
    samples.reserve(rate);
    bool changedSound = false, changedMix = false, canceled = false;
    const auto begin = std::chrono::steady_clock::now();
    RenderMIDIEventsWithFrameBlockCallback(rate, rate, events, settings->sounds, settings->mixes,
        [&](int sample, const double* frames, int count)
        {
            for (int i = 0; i < count; ++i) samples.push_back(frames[i * 2]);
            if (!changedSound && sample >= rate / 3)
            {
                auto next = std::make_shared<LiveRenderSettings>(*settings);
                next->sounds[0].amp = 0.02;
                mailbox->store(next); changedSound = true;
            }
            if (!changedMix && sample >= rate * 2 / 3)
            {
                auto next = std::make_shared<LiveRenderSettings>(*mailbox->load());
                next->mixes[0].mute = true;
                mailbox->store(next); changedMix = true;
            }
            return true;
        }, {}, nullptr, 480, 0, {}, &canceled, mailbox);
    auto energy = [&](double start, double end)
    {
        double sum = 0;
        for (int i = static_cast<int>(start * rate); i < static_cast<int>(end * rate); ++i) sum += samples[i] * samples[i];
        return sum / ((end - start) * rate);
    };
    const double ratio = std::sqrt(energy(0.45, 0.6) / energy(0.15, 0.3));
    Require(!canceled && changedSound && changedMix, "live render failed");
    Require(std::abs(ratio - 0.1) < 0.01, "held note did not receive the sound change");
    Require(energy(0.75, 0.9) == 0, "live mute did not reach audio output");
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    std::cout << "Live held-note update + mute passed; 1 second rendered in " << elapsed << " seconds\n";
}
