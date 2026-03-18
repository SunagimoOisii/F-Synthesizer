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

void NoteOffVoiceModulation(PerSourceVoiceState& sourceState)
{
    std::visit([&](auto& st)
    {
        if constexpr (requires { st.modulation; })
        {
            NoteOffModulation(st.modulation);
        }
    }, sourceState);
}

void InitDrumVoice(const DrumConfig& drum, DrumVoiceState& drumState, double& phaseInc, int sampleRate)
{
    if (drum.type == DrumType::Kick)
    {
        drumState.baseFreq = (drum.baseFreq > 0.0) ? drum.baseFreq : 60.0;
        drumState.pitchDrop = (drum.pitchDrop > 0.0) ? drum.pitchDrop : 3.0;
        drumState.pitchDecaySec = (drum.pitchDecaySec > 0.0) ? drum.pitchDecaySec : 0.06;
        phaseInc = drumState.baseFreq / sampleRate;
    }
    else if (drum.type == DrumType::Snare)
    {
        drumState.baseFreq = (drum.toneFreq > 0.0) ? drum.toneFreq : 200.0;
        phaseInc = drumState.baseFreq / sampleRate;
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 1200.0;
        drumState.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 6000.0;
        drumState.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
    else if (drum.type == DrumType::Hat)
    {
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 6000.0;
        drumState.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 12000.0;
        drumState.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
        drumState.baseFreq = (drum.toneFreq > 0.0) ? drum.toneFreq : 8000.0;
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

    if (const auto* wave = std::get_if<WaveformConfig>(&cfg.source))
    {
        voices.sourceState[i] = WaveformVoiceState{};
        auto& ws = std::get<WaveformVoiceState>(voices.sourceState[i]);

        SetSmoothingRange(ws.ampSmoothing, 0.0, 2.0);
        SetSmoothingSampleRate(ws.ampSmoothing, sampleRate);
        SetSmoothingTimeMs(ws.ampSmoothing, wave->smoothing.ampTimeMs);
        ResetSmoothedParam(ws.ampSmoothing, 1.0);

        SetSmoothingRange(ws.pitchSmoothing, 0.25, 4.0);
        SetSmoothingSampleRate(ws.pitchSmoothing, sampleRate);
        SetSmoothingTimeMs(ws.pitchSmoothing, wave->smoothing.pitchTimeMs);
        ResetSmoothedParam(ws.pitchSmoothing, 1.0);

        SetSmoothingRange(ws.filterCutoffSmoothing, 10.0, 20000.0);
        SetSmoothingSampleRate(ws.filterCutoffSmoothing, sampleRate);
        SetSmoothingTimeMs(ws.filterCutoffSmoothing, wave->smoothing.filterCutoffTimeMs);
        ResetSmoothedParam(ws.filterCutoffSmoothing, wave->filterCutoffHz);

        SetFilterSampleRate(ws.filter, sampleRate);
        SetFilterMode(ws.filter, wave->filterMode);
        SetFilterCutoffHz(ws.filter, wave->filterCutoffHz);
        SetFilterResonance(ws.filter, wave->filterResonance);
        ResetFilterState(ws.filter);

        ResetModulationState(ws.modulation);
        NoteOnModulation(ws.modulation);
    }
    else if (std::holds_alternative<FmConfig>(cfg.source))
    {
        voices.sourceState[i] = FmVoiceState{};
        auto& fs = std::get<FmVoiceState>(voices.sourceState[i]);
        ResetModulationState(fs.modulation);
        NoteOnModulation(fs.modulation);
    }
    else if (std::holds_alternative<NoiseConfig>(cfg.source))
    {
        voices.sourceState[i] = NoiseVoiceState{};
    }
    else if (const auto* drum = std::get_if<DrumConfig>(&cfg.source))
    {
        voices.sourceState[i] = DrumVoiceState{};
        auto& ds = std::get<DrumVoiceState>(voices.sourceState[i]);
        InitDrumVoice(*drum, ds, voices.phaseInc[i], sampleRate);
    }
    else
    {
        voices.sourceState[i] = DrumKitVoiceState{};
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
                NoteOffVoiceModulation(voices.sourceState[i]);
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
    sourceState.reserve(n);
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
    sourceState.clear();
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
    sourceState.emplace_back();

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
                    NoteOffVoiceModulation(sourceState[i]);
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
                NoteOffVoiceModulation(sourceState[i]);
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
    CompactVectorByKeep(sourceState, keepScratch);

    return removed;
}
