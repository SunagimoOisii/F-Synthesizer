#pragma once
#include <chrono>
#include <cmath>
#include "synth/YmfmVoice.h"
#include "SynthEngine/SynthEngine.h"
#include "midi/MIDIReader.h"
#include "synth/Oscillator.h"
#include "SynthEngine/Internal.h"
#include <future>

// The device is initialized but stopped: invoke its real callback directly so
// wraparound, partial reads and stop races are independent of hardware timing.
inline void CheckPreviewRingTransfer(PreviewPlaybackState& playback)
{
    auto reset = [&](int channels, uint64_t capacity, uint64_t offset = 0) {
        StopPreviewAudio(playback);
        playback.channels = channels;
        playback.streamCapacityFrames = capacity;
        playback.streamStartupFrames = 1;
        playback.streamRing.assign(capacity * channels, 0);
        playback.streamReadFrame.store(offset); playback.streamWriteFrame.store(offset);
        playback.streamAvailableFrames.store(0);
        playback.streamMode.store(true);
        return playback.streamSession.load();
    };
    auto finish = [](std::future<bool>& writer) {
        if (writer.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
        {
            // Do not let future destruction hang the checker on a lost wake.
            std::cerr << "Preview producer did not wake after consume/stop\n";
            std::quick_exit(1);
        }
        return writer.get();
    };
    auto read = [&](float* output, int count) { playback.device.onData(&playback.device, output, nullptr, count); };
    for (int channels : {1, 2})
    {
        const auto session = reset(channels, 7, 5);
        constexpr int frames = 259;
        std::array<double, frames * 2> source{};
        for (size_t i = 0; i < source.size(); ++i) source[i] = (int(i % 31) - 15) / 9.0;
        auto writer = std::async(std::launch::async, [&] {
            if (!WriteStreamingPreviewFrame(playback, session, source[0], source[1])) return false;
            return WriteStreamingPreviewFrames(playback, session, source.data() + 2, frames - 1);
        });
        std::vector<float> actual;
        int consumed = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (consumed < frames && std::chrono::steady_clock::now() < deadline)
        {
            const auto available = playback.streamAvailableFrames.load();
            if (!available || !playback.playing.load()) { std::this_thread::yield(); continue; }
            const int count = std::min({int(available), 1 + consumed % 5, frames - consumed});
            float out[10]{}; read(out, count);
            actual.insert(actual.end(), out, out + count * channels);
            consumed += count;
        }
        if (consumed != frames) StopPreviewAudio(playback);
        Require(finish(writer) && consumed == frames, "preview ring lost frames or a wake");
        for (int i = 0; i < frames; ++i)
        {
            const float left = static_cast<float>(std::clamp(source[i * 2], -1.0, 1.0));
            const float right = static_cast<float>(std::clamp(source[i * 2 + 1], -1.0, 1.0));
            Require(actual[i * channels] == (channels == 1 ? (left + right) * .5f : left),
                "preview wraparound changed PCM/order/clipping");
            if (channels == 2) Require(actual[i * 2 + 1] == right, "preview right channel changed");
        }
        Require(playback.streamAvailableFrames.load() == 0 && playback.streamReadFrame.load() == 5 + frames &&
            playback.streamWriteFrame.load() == 5 + frames, "preview cursor/count mismatch");
    }

    for (bool stop : {false, true})
    {
        const auto session = reset(2, 7);
        double input[16]{};
        Require(WriteStreamingPreviewFrames(playback, session, input, 7), "preview fill failed");
        auto writer = std::async(std::launch::async, [&] { return WriteStreamingPreviewFrame(playback, session, .25, -.5); });
        const bool waited = writer.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout;
        if (stop) StopPreviewAudio(playback);
        else { float out[2]{}; read(out, 1); }
        Require(finish(writer) == !stop && waited, "full preview ring did not block/resume/cancel correctly");
        if (stop) Require(!WriteStreamingPreviewFrames(playback, session, input, 1), "stale preview session accepted PCM");
    }
    // Stop racing the writer's initial checks must also release it; there need
    // not have been a wait when the notification was sent.
    for (int trial = 0; trial < 32; ++trial)
    {
        const auto session = reset(2, 1);
        Require(WriteStreamingPreviewFrame(playback, session, 0, 0), "preview race fill failed");
        auto writer = std::async(std::launch::async, [&] { return WriteStreamingPreviewFrame(playback, session, 1, 1); });
        StopPreviewAudio(playback);
        Require(!finish(writer), "stopped preview writer resumed");
    }
    const auto session = reset(2, 7);
    playback.streamStartupFrames = 4;
    Require(WriteStreamingPreviewFrame(playback, session, .25, -.5) && !playback.playing.load(), "preview started before prefill");
    CompleteStreamingPreviewAudio(playback, session, false);
    float tail[6]{}; read(tail, 3);
    Require(tail[0] == .25f && tail[1] == -.5f && tail[2] == 0 && tail[5] == 0 && !playback.playing.load(),
        "short preview did not drain its final frame");
    StopPreviewAudio(playback);
    std::cout << "Preview ring: exact mono/stereo PCM, wrapping, backpressure, consume/stop wake and short drain OK\n";
}

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

inline void CheckCoefficientBlockBoundaries()
{
    // Rendering one sample at a time is an oracle for coefficients that must
    // stay constant only inside a MIDI/sound-bounded block, including moving detune.
    for (int rate : {22050, 44100, 48000})
    {
        auto render = [&](int blockSize) {
            auto state = std::make_unique<RenderState>();
            state->renderParallelDisabled = true;
            state->channelCcGain.fill(1); state->channelPitch.fill(1);
            state->channelRenderable.fill(true); state->channelMixGainL.fill(.7); state->channelMixGainR.fill(.6);
            state->channelAttackScale.fill(1); state->channelDecayScale.fill(1); state->channelReleaseScale.fill(1);
            state->channelBrightness.fill(.5); state->channelResonance.fill(.5);
            state->channelBrightnessCutoffScale.fill(1); state->channelResonanceScale.fill(1);
            auto tone = std::make_unique<InstrumentSoundConfig>();
            tone->source = WaveformConfig{}; tone->amp = .1; tone->sustainLevel = 1;
            auto enable = [](auto& layer) { layer.enabled = true; layer.level = .2; layer.drive = .3; };
            enable(tone->attackLayer); enable(tone->bassLayer); enable(tone->leadLayer);
            enable(tone->chordLayer); enable(tone->padLayer); enable(tone->pluckLayer);
            enable(tone->stringLayer); enable(tone->harmonicLayer); enable(tone->powerChordLayer); enable(tone->chugLayer);
            tone->bodyLayer.enabled = true; tone->bodyLayer.mix = .2;
            tone->ampCabLayer.enabled = true; tone->ampCabLayer.drive = .3;
            tone->expressionMap.enabled = true; tone->expressionMap.pressureToDrive = .4;
            tone->padLayer.motionDepth = .7; tone->padLayer.motionRateHz = 5;
            tone->stringLayer.motionDepth = .6; tone->stringLayer.motionRateHz = 7;
            MIDIEvent on{}; on.type = MIDIEventType::Note; on.isNoteOn = true;
            on.noteNumber = 57; on.velocity = 103; on.noteInstanceID = 1;
            state->voices.AddVoice(*tone, on, rate);
            auto drum = std::make_unique<InstrumentSoundConfig>();
            DrumConfig hit{}; hit.type = DrumType::Snare; hit.drive = .25;
            drum->source = hit; drum->amp = .1; drum->attackSec = 0; drum->decaySec = .08; drum->releaseSec = .01;
            drum->drumBus.enabled = true; drum->drumBus.lowTighten = .2; drum->drumBus.presenceCut = .7;
            on.channel = 9; on.noteInstanceID = 2; state->voices.AddVoice(*drum, on, rate);
            MarkActiveVoiceIndicesDirty(*state);
            std::vector<StereoFrame> output, block;
            SoundData context(1, 16, rate, 2);
            for (int sample = 0; sample < 1024;)
            {
                if (sample == 128)
                {
                    // The newer hit ends inside a 64-frame block; the older bus settings resume.
                    drum->decaySec = .001; drum->releaseSec = .0005;
                    drum->drumBus.lowTighten = .9; drum->drumBus.presenceCut = .1;
                    on.noteInstanceID = 3; state->voices.AddVoice(*drum, on, rate);
                    MarkActiveVoiceIndicesDirty(*state);
                }
                if (sample == 320)
                {
                    tone->bassLayer.pitchOffsetSemis = -12; tone->bassLayer.drive = .8;
                    tone->leadLayer.detuneCents = 31; tone->chordLayer.intervalsSemis[1] = 5;
                    tone->powerChordLayer.spread = .8; tone->powerChordLayer.detuneCents = 15;
                    tone->bodyLayer.damping = .8; tone->ampCabLayer.cabHigh = .2;
                    state->voices.UpdateSound(0, *tone, rate);
                }
                if (sample == 576)
                {
                    state->channelPressure[0] = .9; state->channelBrightness[0] = .2;
                    state->channelPitch[0] = 1.25;
                    state->voices.MarkNoteOff(0, 57, 1, false);
                }
                const int boundary = sample < 128 ? 128 : sample < 320 ? 320 : sample < 576 ? 576 : 1024;
                const int count = std::min(blockSize, boundary - sample);
                RenderVoicesBlock(*state, context, count, block);
                output.insert(output.end(), block.begin(), block.end());
                sample += count;
            }
            return output;
        };
        const auto expected = render(1), actual = render(64);
        double energy = 0;
        for (size_t i = 0; i < actual.size(); ++i)
        {
            Require(actual[i].left == expected[i].left && actual[i].right == expected[i].right,
                "prepared coefficients changed sound across block boundaries");
            energy += actual[i].left * actual[i].left;
        }
        Require(energy > .0001, "coefficient comparison was silent");
    }
    std::cout << "Coefficient blocks agree sample-for-sample through tone/control changes and overlapping drum tails\n";
}

inline void CheckAudioIntegration()
{
    CheckTriangleFourierAgreement();
    CheckParallelDrumMix();
    CheckCoefficientBlockBoundaries();
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
