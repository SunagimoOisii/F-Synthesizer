#include "Internal.h"

#include <cmath>
#include <utility>

#include "synth/Oscillator.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

void InitDrumVoice(const DrumConfig& drum, VoicesSoA& voices, size_t i, int sampleRate)
{
    if (drum.type == DrumType::Kick)
    {
        voices.drumBaseFreq[i] = (drum.baseFreq > 0.0) ? drum.baseFreq : 60.0;
        voices.drumPitchDrop[i] = (drum.pitchDrop > 0.0) ? drum.pitchDrop : 3.0;
        voices.drumPitchDecaySec[i] = (drum.pitchDecaySec > 0.0) ? drum.pitchDecaySec : 0.06;
        voices.phaseInc[i] = voices.drumBaseFreq[i] / sampleRate;
    }
    else if (drum.type == DrumType::Snare)
    {
        voices.drumBaseFreq[i] = (drum.toneFreq > 0.0) ? drum.toneFreq : 200.0;
        voices.phaseInc[i] = voices.drumBaseFreq[i] / sampleRate;
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 1200.0;
        voices.drumHpAlpha[i] = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 6000.0;
        voices.drumLpAlpha[i] = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
    else if (drum.type == DrumType::Hat)
    {
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 6000.0;
        voices.drumHpAlpha[i] = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 12000.0;
        voices.drumLpAlpha[i] = std::exp(-2.0 * kPi * lpCut / sampleRate);
        voices.drumBaseFreq[i] = (drum.toneFreq > 0.0) ? drum.toneFreq : 8000.0;
    }
}

template <typename T>
void CompactVectorByKeep(std::vector<T>& v, const std::vector<uint8_t>& keep)
{
    // keep フラグに従って前方へ詰める共通処理。
    // 目的: SoA 各配列を同じ生存集合で同期圧縮する。
    size_t out = 0;
    for (size_t i = 0; i < v.size(); i++)
    {
        if (keep[i] != 0)
        {
            if (out != i)
            {
                v[out] = std::move(v[i]);
            }
            out++;
        }
    }
    v.resize(out);
}
} // namespace

size_t VoicesSoA::size() const
{
    return source.size();
}

bool VoicesSoA::empty() const
{
    return source.empty();
}

void VoicesSoA::reserve(size_t n)
{
    source.reserve(n);
    noteNumber.reserve(n);
    velocity.reserve(n);
    channel.reserve(n);
    channelIndex.reserve(n);
    noteInstanceId.reserve(n);
    released.reserve(n);
    pendingRemove.reserve(n);
    amp.reserve(n);
    attackSec.reserve(n);
    decaySec.reserve(n);
    sustainLevel.reserve(n);
    releaseSec.reserve(n);
    env.reserve(n);
    phase.reserve(n);
    phaseInc.reserve(n);
    fmCarrierPhase.reserve(n);
    fmModPhase.reserve(n);
    drumTime.reserve(n);
    drumBaseFreq.reserve(n);
    drumPitchDrop.reserve(n);
    drumPitchDecaySec.reserve(n);
    drumNoisePrev.reserve(n);
    drumHpPrev.reserve(n);
    drumHpAlpha.reserve(n);
    drumLpPrev.reserve(n);
    drumLpAlpha.reserve(n);
    waveformFilter.reserve(n);
    waveformModulation.reserve(n);
    waveformAmpSmoothing.reserve(n);
    waveformPitchSmoothing.reserve(n);
    waveformFilterCutoffSmoothing.reserve(n);
}

void VoicesSoA::clear()
{
    source.clear();
    noteNumber.clear();
    velocity.clear();
    channel.clear();
    channelIndex.clear();
    noteInstanceId.clear();
    released.clear();
    pendingRemove.clear();
    amp.clear();
    attackSec.clear();
    decaySec.clear();
    sustainLevel.clear();
    releaseSec.clear();
    env.clear();
    phase.clear();
    phaseInc.clear();
    fmCarrierPhase.clear();
    fmModPhase.clear();
    drumTime.clear();
    drumBaseFreq.clear();
    drumPitchDrop.clear();
    drumPitchDecaySec.clear();
    drumNoisePrev.clear();
    drumHpPrev.clear();
    drumHpAlpha.clear();
    drumLpPrev.clear();
    drumLpAlpha.clear();
    waveformFilter.clear();
    waveformModulation.clear();
    waveformAmpSmoothing.clear();
    waveformPitchSmoothing.clear();
    waveformFilterCutoffSmoothing.clear();
}

void VoicesSoA::AddVoice(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate)
{
    // SoA 全配列へ同一インデックスで push し、列単位アクセス可能な状態を維持する。
    source.push_back(cfg.source);
    noteNumber.push_back(e.noteNumber);
    velocity.push_back(e.velocity);
    channel.push_back(e.channel);
    channelIndex.push_back(ClampChannel(e.channel));
    noteInstanceId.push_back(e.noteInstanceId);
    released.push_back(0);
    pendingRemove.push_back(0);

    amp.push_back(cfg.amp);
    attackSec.push_back(cfg.attackSec);
    decaySec.push_back(cfg.decaySec);
    sustainLevel.push_back(cfg.sustainLevel);
    releaseSec.push_back(cfg.releaseSec);
    ADSRState envState{};
    NoteOn(envState);
    env.push_back(envState);

    phase.push_back(0.0);
    phaseInc.push_back(NoteNumberToFreq(e.noteNumber) / sampleRate);
    fmCarrierPhase.push_back(0.0);
    fmModPhase.push_back(0.0);

    drumTime.push_back(0.0);
    drumBaseFreq.push_back(0.0);
    drumPitchDrop.push_back(1.0);
    drumPitchDecaySec.push_back(0.0);
    drumNoisePrev.push_back(0.0);
    drumHpPrev.push_back(0.0);
    drumHpAlpha.push_back(0.0);
    drumLpPrev.push_back(0.0);
    drumLpAlpha.push_back(0.0);
    waveformFilter.emplace_back();
    waveformModulation.emplace_back();
    waveformAmpSmoothing.emplace_back();
    waveformPitchSmoothing.emplace_back();
    waveformFilterCutoffSmoothing.emplace_back();

    const size_t i = size() - 1;
    SetSmoothingRange(waveformAmpSmoothing[i], 0.0, 2.0);
    SetSmoothingSampleRate(waveformAmpSmoothing[i], sampleRate);
    SetSmoothingTimeMs(waveformAmpSmoothing[i], 4.0);
    ResetSmoothedParam(waveformAmpSmoothing[i], 1.0);

    SetSmoothingRange(waveformPitchSmoothing[i], 0.25, 4.0);
    SetSmoothingSampleRate(waveformPitchSmoothing[i], sampleRate);
    SetSmoothingTimeMs(waveformPitchSmoothing[i], 2.0);
    ResetSmoothedParam(waveformPitchSmoothing[i], 1.0);

    SetSmoothingRange(waveformFilterCutoffSmoothing[i], 10.0, 20000.0);
    SetSmoothingSampleRate(waveformFilterCutoffSmoothing[i], sampleRate);
    SetSmoothingTimeMs(waveformFilterCutoffSmoothing[i], 8.0);
    ResetSmoothedParam(waveformFilterCutoffSmoothing[i], 1200.0);

    if (const auto* drum = std::get_if<DrumConfig>(&cfg.source))
    {
        InitDrumVoice(*drum, *this, i, sampleRate);
    }
    if (const auto* wave = std::get_if<WaveformConfig>(&cfg.source))
    {
        FilterInstance& filter = waveformFilter[i];
        SetFilterSampleRate(filter, sampleRate);
        SetFilterMode(filter, wave->filterMode);
        SetFilterCutoffHz(filter, wave->filterCutoffHz);
        SetFilterResonance(filter, wave->filterResonance);
        ResetFilterState(filter);

        SetSmoothingTimeMs(waveformAmpSmoothing[i], wave->smoothing.ampTimeMs);
        SetSmoothingTimeMs(waveformPitchSmoothing[i], wave->smoothing.pitchTimeMs);
        SetSmoothingTimeMs(waveformFilterCutoffSmoothing[i], wave->smoothing.filterCutoffTimeMs);
        ResetSmoothedParam(waveformFilterCutoffSmoothing[i], wave->filterCutoffHz);

        ResetModulationState(waveformModulation[i]);
        NoteOnModulation(waveformModulation[i]);
    }
    else
    {
        SetFilterMode(waveformFilter[i], FilterMode::Bypass);
        ResetModulationState(waveformModulation[i]);
    }
}

void VoicesSoA::MarkNoteOff(int ch, int note)
{
    // 同一 note の重なりでは最も古い未 release Voice から閉じる方針。
    // トレードオフ: MPE/voice-id 由来の厳密対応はしていない。
    for (size_t i = 0; i < size(); i++)
    {
        if (std::holds_alternative<DrumConfig>(source[i]))
        {
            continue;
        }
        if (released[i] == 0 && noteNumber[i] == note && channel[i] == ch)
        {
            NoteOff(env[i]);
            if (std::holds_alternative<WaveformConfig>(source[i]))
            {
                NoteOffModulation(waveformModulation[i]);
            }
            released[i] = 1;
            break;
        }
    }
}

size_t VoicesSoA::CleanupPending(std::vector<uint8_t>& keepScratch)
{
    if (empty())
    {
        return 0;
    }

    keepScratch.assign(size(), 1);
    size_t removed = 0;
    for (size_t i = 0; i < size(); i++)
    {
        if (pendingRemove[i] != 0)
        {
            keepScratch[i] = 0;
            removed++;
        }
    }
    if (removed == 0)
    {
        return 0;
    }

    // source 以外も完全に同順で圧縮し、列間のインデックス整合を崩さない。
    CompactVectorByKeep(source, keepScratch);
    CompactVectorByKeep(noteNumber, keepScratch);
    CompactVectorByKeep(velocity, keepScratch);
    CompactVectorByKeep(channel, keepScratch);
    CompactVectorByKeep(channelIndex, keepScratch);
    CompactVectorByKeep(noteInstanceId, keepScratch);
    CompactVectorByKeep(released, keepScratch);
    CompactVectorByKeep(pendingRemove, keepScratch);
    CompactVectorByKeep(amp, keepScratch);
    CompactVectorByKeep(attackSec, keepScratch);
    CompactVectorByKeep(decaySec, keepScratch);
    CompactVectorByKeep(sustainLevel, keepScratch);
    CompactVectorByKeep(releaseSec, keepScratch);
    CompactVectorByKeep(env, keepScratch);
    CompactVectorByKeep(phase, keepScratch);
    CompactVectorByKeep(phaseInc, keepScratch);
    CompactVectorByKeep(fmCarrierPhase, keepScratch);
    CompactVectorByKeep(fmModPhase, keepScratch);
    CompactVectorByKeep(drumTime, keepScratch);
    CompactVectorByKeep(drumBaseFreq, keepScratch);
    CompactVectorByKeep(drumPitchDrop, keepScratch);
    CompactVectorByKeep(drumPitchDecaySec, keepScratch);
    CompactVectorByKeep(drumNoisePrev, keepScratch);
    CompactVectorByKeep(drumHpPrev, keepScratch);
    CompactVectorByKeep(drumHpAlpha, keepScratch);
    CompactVectorByKeep(drumLpPrev, keepScratch);
    CompactVectorByKeep(drumLpAlpha, keepScratch);
    CompactVectorByKeep(waveformFilter, keepScratch);
    CompactVectorByKeep(waveformModulation, keepScratch);
    CompactVectorByKeep(waveformAmpSmoothing, keepScratch);
    CompactVectorByKeep(waveformPitchSmoothing, keepScratch);
    CompactVectorByKeep(waveformFilterCutoffSmoothing, keepScratch);

    return removed;
}
