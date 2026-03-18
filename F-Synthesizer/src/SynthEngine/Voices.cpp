#include "Internal.h"

#include <cmath>
#include <utility>

#include "config/SourceRegistry.h"
#include "synth/Oscillator.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr size_t kMaxVoices = 256;

// 目的: retrigger/steal の「同系統判定」を capability 基準でそろえる。
// 制約: SourceKind が異なっても capability が同一なら同系統として扱う。
bool HasSameCapabilityProfile(const SourceConfig& a, const SourceConfig& b)
{
    const config::SourceCapability lhs = config::SourceCapabilityOf(a);
    const config::SourceCapability rhs = config::SourceCapabilityOf(b);
    return lhs.hasPitch == rhs.hasPitch &&
        lhs.hasAmpEnv == rhs.hasAmpEnv &&
        lhs.hasFilterIn == rhs.hasFilterIn &&
        lhs.hasModTargets == rhs.hasModTargets &&
        lhs.supportsPolyphony == rhs.supportsPolyphony &&
        lhs.isOneShot == rhs.isOneShot &&
        lhs.isPercussion == rhs.isPercussion;
}

void InitDrumVoice(const DrumConfig& drum, Voice& voices, size_t i, int sampleRate)
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

void InitializeVoiceAtIndex(
    Voice& voices,
    size_t i,
    const ChannelConfig& cfg,
    const MIDIEvent& e,
    int sampleRate)
{
    // 1 voice の実行状態を初期化し、retrigger=restart 時も同じ経路で再利用する。
    voices.source[i] = cfg.source;
    voices.noteNumber[i] = e.noteNumber;
    voices.velocity[i] = e.velocity;
    voices.channel[i] = e.channel;
    voices.channelIndex[i] = ClampChannel(e.channel);
    voices.noteInstanceID[i] = e.noteInstanceID;
    voices.released[i] = 0;
    voices.pendingRemove[i] = 0;

    voices.amp[i] = cfg.amp;
    voices.attackSec[i] = cfg.attackSec;
    voices.decaySec[i] = cfg.decaySec;
    voices.sustainLevel[i] = cfg.sustainLevel;
    voices.releaseSec[i] = cfg.releaseSec;
    ADSRState envState{};
    NoteOn(envState);
    voices.env[i] = envState;

    voices.phase[i] = 0.0;
    voices.phaseInc[i] = NoteNumberToFreq(e.noteNumber) / sampleRate;
    voices.fmCarrierPhase[i] = 0.0;
    voices.fmModPhase[i] = 0.0;

    voices.drumTime[i] = 0.0;
    voices.drumBaseFreq[i] = 0.0;
    voices.drumPitchDrop[i] = 1.0;
    voices.drumPitchDecaySec[i] = 0.0;
    voices.drumNoisePrev[i] = 0.0;
    voices.drumHpPrev[i] = 0.0;
    voices.drumHpAlpha[i] = 0.0;
    voices.drumLpPrev[i] = 0.0;
    voices.drumLpAlpha[i] = 0.0;

    SetSmoothingRange(voices.waveformAmpSmoothing[i], 0.0, 2.0);
    SetSmoothingSampleRate(voices.waveformAmpSmoothing[i], sampleRate);
    SetSmoothingTimeMs(voices.waveformAmpSmoothing[i], 4.0);
    ResetSmoothedParam(voices.waveformAmpSmoothing[i], 1.0);

    SetSmoothingRange(voices.waveformPitchSmoothing[i], 0.25, 4.0);
    SetSmoothingSampleRate(voices.waveformPitchSmoothing[i], sampleRate);
    SetSmoothingTimeMs(voices.waveformPitchSmoothing[i], 2.0);
    ResetSmoothedParam(voices.waveformPitchSmoothing[i], 1.0);

    SetSmoothingRange(voices.waveformFilterCutoffSmoothing[i], 10.0, 20000.0);
    SetSmoothingSampleRate(voices.waveformFilterCutoffSmoothing[i], sampleRate);
    SetSmoothingTimeMs(voices.waveformFilterCutoffSmoothing[i], 8.0);
    ResetSmoothedParam(voices.waveformFilterCutoffSmoothing[i], 1200.0);

    if (const auto* drum = std::get_if<DrumConfig>(&cfg.source))
    {
        InitDrumVoice(*drum, voices, i, sampleRate);
    }
    if (const auto* wave = std::get_if<WaveformConfig>(&cfg.source))
    {
        FilterInstance& filter = voices.waveformFilter[i];
        SetFilterSampleRate(filter, sampleRate);
        SetFilterMode(filter, wave->filterMode);
        SetFilterCutoffHz(filter, wave->filterCutoffHz);
        SetFilterResonance(filter, wave->filterResonance);
        ResetFilterState(filter);

        SetSmoothingTimeMs(voices.waveformAmpSmoothing[i], wave->smoothing.ampTimeMs);
        SetSmoothingTimeMs(voices.waveformPitchSmoothing[i], wave->smoothing.pitchTimeMs);
        SetSmoothingTimeMs(voices.waveformFilterCutoffSmoothing[i], wave->smoothing.filterCutoffTimeMs);
        ResetSmoothedParam(voices.waveformFilterCutoffSmoothing[i], wave->filterCutoffHz);

        ResetModulationState(voices.waveformModulation[i]);
        NoteOnModulation(voices.waveformModulation[i]);
    }
    else if (std::holds_alternative<FmConfig>(cfg.source))
    {
        SetFilterMode(voices.waveformFilter[i], FilterMode::Bypass);
        ResetModulationState(voices.waveformModulation[i]);
        NoteOnModulation(voices.waveformModulation[i]);
    }
    else
    {
        SetFilterMode(voices.waveformFilter[i], FilterMode::Bypass);
        ResetModulationState(voices.waveformModulation[i]);
    }
}

bool TryRestartVoiceOnRetrigger(Voice& voices, const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate)
{
    // 前提: source.lifecycle.retrigger=restart の場合のみ既存voiceを再初期化する。
    const config::SourceLifecyclePolicy policy = config::SourceLifecycleOf(cfg.source);
    if (policy.retrigger != config::SourceLifecycleRetrigger::Restart)
    {
        return false;
    }

    size_t restartIndex = static_cast<size_t>(-1);
    for (size_t i = 0; i < voices.size(); i++)
    {
        if (voices.pendingRemove[i] != 0)
        {
            continue;
        }
        if (voices.channel[i] != e.channel || voices.noteNumber[i] != e.noteNumber)
        {
            continue;
        }
        if (!HasSameCapabilityProfile(voices.source[i], cfg.source))
        {
            continue;
        }

        if (restartIndex == static_cast<size_t>(-1))
        {
            restartIndex = i;
            continue;
        }

        // 既存の重複voiceは release 済みへ倒し、後段の cleanup で回収する。
        if (voices.released[i] == 0)
        {
            NoteOff(voices.env[i]);
            if (config::SourceCapabilityOf(voices.source[i]).hasModTargets)
            {
                NoteOffModulation(voices.waveformModulation[i]);
            }
            voices.released[i] = 1;
        }
        voices.env[i] = ADSRState{};
    }

    if (restartIndex == static_cast<size_t>(-1))
    {
        return false;
    }

    InitializeVoiceAtIndex(voices, restartIndex, cfg, e, sampleRate);
    return true;
}

bool TryHandleVoiceLimitAndSteal(Voice& voices, const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate)
{
    if (voices.size() < kMaxVoices)
    {
        return false;
    }

    const config::SourceLifecyclePolicy policy = config::SourceLifecycleOf(cfg.source);
    if (policy.steal == config::SourceLifecycleSteal::RejectNew)
    {
        return true;
    }

    size_t victim = static_cast<size_t>(-1);
    // Oldest: まず同一capabilityの最古voiceを優先して差し替える。
    for (size_t i = 0; i < voices.size(); i++)
    {
        if (voices.pendingRemove[i] != 0)
        {
            continue;
        }
        if (HasSameCapabilityProfile(voices.source[i], cfg.source))
        {
            victim = i;
            break;
        }
    }
    // 同一capabilityが無い場合は、全体の最古voiceを差し替える。
    if (victim == static_cast<size_t>(-1))
    {
        for (size_t i = 0; i < voices.size(); i++)
        {
            if (voices.pendingRemove[i] == 0)
            {
                victim = i;
                break;
            }
        }
    }
    if (victim == static_cast<size_t>(-1))
    {
        // 全voiceが pendingRemove の場合は次回cleanupを待ち、新規は受け付けない。
        return true;
    }

    InitializeVoiceAtIndex(voices, victim, cfg, e, sampleRate);
    return true;
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

size_t Voice::size() const
{
    return source.size();
}

bool Voice::empty() const
{
    return source.empty();
}

void Voice::reserve(size_t n)
{
    source.reserve(n);
    noteNumber.reserve(n);
    velocity.reserve(n);
    channel.reserve(n);
    channelIndex.reserve(n);
    noteInstanceID.reserve(n);
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

void Voice::clear()
{
    source.clear();
    noteNumber.clear();
    velocity.clear();
    channel.clear();
    channelIndex.clear();
    noteInstanceID.clear();
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

void Voice::AddVoice(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate)
{
    if (TryRestartVoiceOnRetrigger(*this, cfg, e, sampleRate))
    {
        return;
    }
    if (TryHandleVoiceLimitAndSteal(*this, cfg, e, sampleRate))
    {
        return;
    }

    // SoA 全配列へ同一インデックスで push し、列単位アクセス可能な状態を維持する。
    source.emplace_back();
    noteNumber.push_back(0);
    velocity.push_back(0);
    channel.push_back(0);
    channelIndex.push_back(0);
    noteInstanceID.push_back(-1);
    released.push_back(0);
    pendingRemove.push_back(0);

    amp.push_back(0.0);
    attackSec.push_back(0.0);
    decaySec.push_back(0.0);
    sustainLevel.push_back(0.0);
    releaseSec.push_back(0.0);
    env.emplace_back();

    phase.push_back(0.0);
    phaseInc.push_back(0.0);
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
    InitializeVoiceAtIndex(*this, i, cfg, e, sampleRate);
}

void Voice::MarkNoteOff(int ch, int note, int noteInstanceID)
{
    // 原則: NoteOn/NoteOff の対応IDで閉じる。
    // 互換性: IDが無い/不一致の場合は旧ロジック(ch+note)へフォールバックする。
    if (noteInstanceID >= 0)
    {
        for (size_t i = 0; i < size(); i++)
        {
            const config::SourceCapability cap = config::SourceCapabilityOf(source[i]);
            if (cap.isOneShot)
            {
                continue;
            }
            if (released[i] == 0 && this->noteInstanceID[i] == noteInstanceID)
            {
                NoteOff(env[i]);
                if (cap.hasModTargets)
                {
                    NoteOffModulation(waveformModulation[i]);
                }
                released[i] = 1;
                return;
            }
        }
    }

    // 旧互換経路: 既存データや異常入力でIDが取れない場合のみ利用。
    for (size_t i = 0; i < size(); i++)
    {
        const config::SourceCapability cap = config::SourceCapabilityOf(source[i]);
        if (cap.isOneShot)
        {
            continue;
        }
        if (released[i] == 0 && noteNumber[i] == note && channel[i] == ch)
        {
            NoteOff(env[i]);
            if (cap.hasModTargets)
            {
                NoteOffModulation(waveformModulation[i]);
            }
            released[i] = 1;
            break;
        }
    }
}

size_t Voice::CleanupPending(std::vector<uint8_t>& keepScratch)
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
    CompactVectorByKeep(noteInstanceID, keepScratch);
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
