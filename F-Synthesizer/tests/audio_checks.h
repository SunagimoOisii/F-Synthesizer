#pragma once
#include <chrono>
#include <cmath>
#include "synth/YmfmVoice.h"
#include "SynthEngine/SynthEngine.h"
#include "midi/MIDIReader.h"
#include "synth/Oscillator.h"
#include "SynthEngine/Internal.h"

inline void CheckTriangleFourierAgreement()
{
    const double pi = std::acos(-1.0);
    double maxError = 0;
    // Exercise every harmonic cutoff, phase wrapping, and the zero/Nyquist paths.
    std::vector<double> increments{0, 1e-300, .5, .75};
    for (int harmonic = 1; harmonic <= 256; ++harmonic)
    {
        const double edge = .5 / harmonic;
        increments.push_back(edge);
        increments.push_back(std::nextafter(edge, 0.0));
        increments.push_back(std::nextafter(edge, 1.0));
    }
    for (double increment : increments)
        for (int n = 0; n <= 128; ++n)
        {
            const double phase = (n - 32) / 64.0;
            const double p = phase - std::floor(phase);
            double expected = 1.0 - 4.0 * std::abs(p - .5);
            if (increment > 0 && increment < .5)
            {
                const int limit = static_cast<int>(std::min(255.0, std::floor(.5 / increment)));
                double sum = 0;
                for (int h = 1; h <= limit; h += 2)
                    sum += std::cos(2 * pi * h * p) / (h * h);
                expected = -8.0 / (pi * pi) * sum;
            }
            for (double sign : {-1.0, 1.0})
            {
                const double actual = SampleWavePhase(WaveType::Triangle, phase, sign * increment);
                Require(std::isfinite(actual), "triangle produced non-finite output");
                maxError = std::max(maxError, std::abs(actual - expected));
            }
        }
    Require(maxError < 1e-11, "triangle changed its band-limited Fourier sum");
    std::cout << "Triangle Fourier agreement: max error=" << maxError << '\n';
}

inline void CheckParallelDrumMix()
{
    constexpr int rate = 44100;
    auto sounds = std::make_unique<std::array<InstrumentSoundConfig, 16>>();
    std::vector<MIDIEvent> events;
    for (int ch : {0, 2, 5, 9})
    {
        auto& sound = (*sounds)[ch];
        sound.source = FmConfig{}; sound.amp = .2;
        sound.attackSec = .001; sound.decaySec = .02; sound.sustainLevel = .7; sound.releaseSec = .01;
        if (ch == 9)
        {
            DrumConfig drum{}; drum.type = DrumType::Snare;
            drum.bodyFreq = 170; drum.bodyLevel = .5; drum.noiseLevel = .3; drum.decaySec = .06;
            sound.source = drum;
            sound.drumBus.enabled = true; sound.drumBus.glue = .6;
            sound.drumBus.lowTighten = .3; sound.drumBus.presenceCut = .2; sound.drumBus.roomSend = .3;
        }
        for (int n = 0; n < 2; ++n)
        {
            MIDIEvent on{}; on.type = MIDIEventType::Note; on.isNoteOn = true;
            on.channel = ch; on.noteNumber = 48 + n * 7; on.velocity = 100; on.noteInstanceID = ch * 10 + n + 1;
            events.push_back(on);
            auto off = on; off.sample = 1200; off.isNoteOn = false; events.push_back(off);
        }
    }
    std::stable_sort(events.begin(), events.end(), [](const auto& a, const auto& b) { return a.sample < b.sample; });
    auto makeState = [&](bool serial)
    {
        auto state = std::make_unique<RenderState>(); state->renderParallelDisabled = serial;
        state->channelCcGain.fill(1); state->channelPitch.fill(1);
        state->channelRenderable.fill(true); state->channelMixGainL.fill(.7); state->channelMixGainR.fill(.6);
        state->channelAttackScale.fill(1); state->channelDecayScale.fill(1); state->channelReleaseScale.fill(1);
        state->channelBrightness.fill(.5); state->channelResonance.fill(.5);
        state->channelBrightnessCutoffScale.fill(1); state->channelResonanceScale.fill(1);
        state->scopeChannel = 9;
        return state;
    };
    auto serial = makeState(true), parallel = makeState(false);
    SoundData context(1, 16, rate, 2);
    std::vector<StereoFrame> expected, actual;
    double energy = 0;
    for (int sample = 0; sample < 2400;)
    {
        for (auto* state : {serial.get(), parallel.get()})
        {
            ProcessEventsAtSample(events, sample, *sounds, rate, *state);
            if (sample == 640) // A held note changes source at a render boundary.
                for (size_t i = 0; i < state->voices.size(); ++i)
                    if (state->voices.channelIndex[i] == 2)
                    {
                        auto changed = (*sounds)[2]; changed.source = WaveformConfig{};
                        state->voices.UpdateSound(i, changed, rate);
                        MarkActiveVoiceIndicesDirty(*state);
                    }
        }
        const int end = std::min({sample + (sample == 704 ? 17 : 64), sample < 1200 ? 1200 : 2400, 2400});
        serial->scopeFrames.assign(end - sample, 0); parallel->scopeFrames.assign(end - sample, 0);
        RenderVoicesBlock(*serial, context, end - sample, expected);
        RenderVoicesBlock(*parallel, context, end - sample, actual);
        Require(expected.size() == actual.size(), "parallel render lost frames");
        for (size_t n = 0; n < actual.size(); ++n)
        {
            Require(actual[n].left == expected[n].left && actual[n].right == expected[n].right,
                "parallel drum mix changed PCM/order/state");
            energy += actual[n].left * actual[n].left;
        }
        Require(serial->scopeFrames == parallel->scopeFrames, "parallel drum scope changed");
        sample = end;
    }
    Require(energy > .001, "parallel comparison was silent");
    if (std::thread::hardware_concurrency() > 1)
        Require(parallel->renderWorkerPool != nullptr, "drum bus disabled parallel rendering");
    std::cout << "Parallel drum mix, source change, note-off and scope agree with serial output\n";
}

inline void CheckVocalFilterAndFmSweep()
{
    for (int rate : {22050, 44100})
    {
        auto filteredEnergy = [&](double hz)
        {
            FilterInstance filter{};
            SetFilterSampleRate(filter, rate); SetFilterMode(filter, FilterMode::Vocal);
            SetFilterCutoffHz(filter, 700); SetFilterResonance(filter, 5);
            double energy = 0;
            for (int n = 0; n < rate / 5; ++n)
            {
                const double sample = ProcessFilterSample(filter, std::sin(6.283185307179586 * hz * n / rate));
                Require(std::isfinite(sample), "vocal filter produced a non-finite sample");
                if (n > rate / 10) energy += sample * sample;
            }
            ResetFilterState(filter);
            Require(ProcessFilterSample(filter, 0) == 0, "vocal filter reset retained ringing");
            return energy;
        };
        const double outside = filteredEnergy(7000);
        for (double resonance : {700.0, 1120.0, 2520.0})
            Require(filteredEnergy(resonance) > outside * 8, "vocal resonance is missing");
    }
    // The filter-cutoff route was previously evaluated, then discarded by FM rendering.
    std::array<InstrumentSoundConfig, 16> sounds{};
    auto& sound = sounds[0];
    sound.amp = .2; sound.attackSec = .001; sound.decaySec = .01;
    sound.sustainLevel = 1; sound.releaseSec = .02;
    FmConfig fm{}; fm.algorithm = 7; fm.filterMode = FilterMode::LowPass; fm.filterCutoffHz = 400;
    for (auto& op : fm.ops) { op.level = 0; op.levelEnv.attackSec = .001; op.levelEnv.sustainLevel = 1; }
    fm.ops[3].level = 1;
    fm.modulation.lfo1 = {LfoWave::Square, 2, .8, true, true, 0, 0};
    fm.modulation.matrix.routes[0] = {ModSource::Lfo1, ModDestination::FilterCutoff, .9, true};
    sound.source = fm;
    MIDIEvent note{}; note.type = MIDIEventType::Note; note.isNoteOn = true;
    note.channel = 0; note.noteNumber = 69; note.velocity = 100; note.noteInstanceID = 1;
    double open = 0, closed = 0;
    constexpr int rate = 44100;
    auto measureWindows = [&] {
        open = closed = 0;
        RenderMIDIEventsWithFrameBlockCallback(rate / 2, rate, {note}, sounds, {},
        [&](int sample, const double* frames, int count)
        {
            for (int n = 0; n < count; ++n)
            {
                const double energy = frames[n * 2] * frames[n * 2];
                if (sample + n > rate * .08 && sample + n < rate * .18) open += energy;
                if (sample + n > rate * .33 && sample + n < rate * .43) closed += energy;
            }
            return true;
        });
    };
    measureWindows();
    Require(open > closed * 4 && closed > 0, "FM cutoff modulation did not reach the audio filter");
    // Even a layer-only FM instrument must receive its amplitude modulation.
    fm.ops[3].level = 0; fm.filterMode = FilterMode::Bypass;
    fm.modulation.matrix.routes[0].destination = ModDestination::Amp;
    sound.source = fm;
    sound.harmonicLayer.enabled = true; sound.harmonicLayer.level = .4;
    sound.harmonicLayer.harmonicLevels = {1, 0, 0, 0, 0, 0, 0, 0};
    sound.harmonicLayer.stereo = 0;
    measureWindows();
    Require(open > closed * 10 && closed > 0, "FM tremolo did not reach auxiliary layers");
    std::cout << "Vocal resonances, reset, FM cutoff and layer tremolo OK\n";
}

inline void CheckPercussionCoverage(const InstrumentSoundConfig& sound, const std::string& label)
{
    const auto* kit = std::get_if<DrumKitConfig>(&sound.source);
    if (!kit) return;
    std::array<InstrumentSoundConfig, 16> sounds{};
    sounds[9] = sound;
    const std::array<ChannelMixState, 16> mixes{};
    for (int rate : {22050, 44100})
    {
        for (int note = 35; note <= 81; ++note)
        {
            const auto context = label + " GM " + std::to_string(note);
            Require(kit->map[note].type != DrumType::None, context + ": missing percussion");
            MIDIEvent on{}; on.type = MIDIEventType::Note; on.channel = 9;
            on.noteNumber = note; on.velocity = 100; on.isNoteOn = true; on.noteInstanceID = 1;
            MIDIEvent off = on; off.sample = rate / 100; off.isNoteOn = false;
            double energy = 0.0, peak = 0.0;
            bool finite = true;
            RenderMIDIEventsWithFrameBlockCallback(rate / 5, rate, {on, off}, sounds, mixes,
                [&](int, const double* frames, int count)
                {
                    for (int i = 0; i < count * 2; ++i)
                    {
                        finite = finite && std::isfinite(frames[i]);
                        peak = std::max(peak, std::abs(frames[i]));
                        energy += frames[i] * frames[i];
                    }
                    return true;
                });
            Require(finite, context + ": non-finite sample");
            Require(energy > 0.00001, context + ": silent percussion");
            Require(peak < 0.98, context + ": clipping percussion");
        }
    }
    std::cout << label << ": all 47 GM hits audible and finite at 22050/44100 Hz\n";
}

inline void CheckAudioIntegration()
{
    CheckTriangleFourierAgreement();
    CheckParallelDrumMix();
    CheckVocalFilterAndFmSweep();
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
    settings->scope = std::make_shared<AudioScope>();
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
    Require(settings->scope->cursor.load() == rate, "scope skipped rendered or silent frames");
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    std::cout << "Live held-note update + mute passed; 1 second rendered in " << elapsed << " seconds\n";
}
