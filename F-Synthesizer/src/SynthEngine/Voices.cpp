#include "Internal.h"

#include <cmath>
#include <type_traits>
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
    drumState.bodyFreq = (drum.bodyFreq > 0.0) ? drum.bodyFreq : 120.0;
    drumState.pitchStart = (drum.pitchStart > 0.0) ? drum.pitchStart : 1.0;
    drumState.pitchDecaySec = (drum.pitchDecaySec > 0.0) ? drum.pitchDecaySec : 0.04;
    phaseInc = drumState.bodyFreq / sampleRate;

    if (drum.type == DrumType::Kick)
    {
        drumState.bodyFreq = (drum.bodyFreq > 0.0) ? drum.bodyFreq : 54.0;
        drumState.pitchStart = (drum.pitchStart > 0.0) ? drum.pitchStart : 4.5;
        drumState.pitchDecaySec = (drum.pitchDecaySec > 0.0) ? drum.pitchDecaySec : 0.045;
        phaseInc = drumState.bodyFreq / sampleRate;
    }
    else if (drum.type == DrumType::Snare)
    {
        drumState.bodyFreq = (drum.bodyFreq > 0.0) ? drum.bodyFreq : 210.0;
        phaseInc = drumState.bodyFreq / sampleRate;
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 1200.0;
        drumState.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 5200.0;
        drumState.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
    else if (drum.type == DrumType::Hat)
    {
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 5200.0;
        drumState.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 11000.0;
        drumState.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
        drumState.bodyFreq = 7600.0;
        phaseInc = drumState.bodyFreq / sampleRate;
    }
    else if (drum.type == DrumType::Tom)
    {
        drumState.bodyFreq = (drum.bodyFreq > 0.0) ? drum.bodyFreq : 150.0;
        drumState.pitchStart = (drum.pitchStart > 0.0) ? drum.pitchStart : 2.4;
        drumState.pitchDecaySec = (drum.pitchDecaySec > 0.0) ? drum.pitchDecaySec : 0.075;
        phaseInc = drumState.bodyFreq / sampleRate;
    }
    else if (drum.type == DrumType::Rim || drum.type == DrumType::Clap)
    {
        drumState.bodyFreq = (drum.bodyFreq > 0.0) ? drum.bodyFreq : ((drum.type == DrumType::Rim) ? 950.0 : 780.0);
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : ((drum.type == DrumType::Rim) ? 1800.0 : 900.0);
        drumState.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : ((drum.type == DrumType::Rim) ? 7600.0 : 5200.0);
        drumState.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
        phaseInc = drumState.bodyFreq / sampleRate;
    }
    else if (drum.type == DrumType::Crash || drum.type == DrumType::Ride)
    {
        const double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 4300.0;
        drumState.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        const double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 12500.0;
        drumState.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
        drumState.bodyFreq = (drum.type == DrumType::Ride) ? 4200.0 : 5200.0;
        phaseInc = drumState.bodyFreq / sampleRate;
    }
}

template <typename SourceT, typename VoiceStateT>
void InitWaveformLikeVoiceStateCommon(
    const SourceT& src,
    VoiceStateT& state,
    int sampleRate,
    int noteNumber)
{
    SetSmoothingRange(state.ampSmoothing, 0.0, 2.0);
    SetSmoothingSampleRate(state.ampSmoothing, sampleRate);
    SetSmoothingTimeMs(state.ampSmoothing, src.smoothing.ampTimeMs);
    ResetSmoothedParam(state.ampSmoothing, 1.0);

    SetSmoothingRange(state.pitchSmoothing, 0.25, 4.0);
    SetSmoothingSampleRate(state.pitchSmoothing, sampleRate);
    SetSmoothingTimeMs(state.pitchSmoothing, src.smoothing.pitchTimeMs);
    ResetSmoothedParam(state.pitchSmoothing, 1.0);

    SetSmoothingRange(state.filterCutoffSmoothing, 10.0, 20000.0);
    SetSmoothingSampleRate(state.filterCutoffSmoothing, sampleRate);
    SetSmoothingTimeMs(state.filterCutoffSmoothing, src.smoothing.filterCutoffTimeMs);
    ResetSmoothedParam(state.filterCutoffSmoothing, src.filterCutoffHz);

    SetFilterSampleRate(state.filter, sampleRate);
    SetFilterMode(state.filter, src.filterMode);
    SetFilterCutoffHz(state.filter, src.filterCutoffHz);
    SetFilterResonance(state.filter, src.filterResonance);
    SetFilterDrive(state.filter, src.filterDrive);
    ResetFilterState(state.filter);

    state.syncPhase = 0.0;
    state.ringPhase = 0.0;
    state.arpStep = 0;
    state.arpElapsedSec = 0.0;
    state.filterKeytrackRatio = std::exp2(src.filterKeytrack * (noteNumber - 60) / 12.0);
    {
        const double detuneCents = std::clamp(src.unisonDetuneCents, 0.0, 120.0);
        const int unisonVoices = std::clamp(src.unisonVoices, 1, 8);
        for (int uv = 0; uv < 8; uv++)
        {
            if (uv < unisonVoices)
            {
                const double pos = (unisonVoices <= 1) ? 0.0 : (static_cast<double>(uv) / (unisonVoices - 1));
                const double centered = (pos * 2.0) - 1.0;
                const double cents = centered * detuneCents;
                state.unisonDetuneRatio[uv] = std::exp2(cents / 1200.0);
            }
            else
            {
                state.unisonDetuneRatio[uv] = 1.0;
            }
        }
    }
    if (src.drive > 0.0)
    {
        const double k = src.drive * 20.0;
        const double tanhK = std::tanh(k);
        state.driveNorm = (tanhK > 1e-9) ? (1.0 / tanhK) : 1.0;
    }
    else
    {
        state.driveNorm = 1.0;
    }
    ResetModulationState(state.modulation);
    NoteOnModulation(state.modulation, src.modulation);
}

void ApplyVoiceConfig(Voice& voices, size_t i, const InstrumentSoundConfig& cfg, int velocity)
{
    voices.amp[i] = cfg.amp;
    voices.attackSec[i] = cfg.attackSec;
    voices.decaySec[i] = cfg.decaySec;
    voices.sustainLevel[i] = cfg.sustainLevel;
    voices.releaseSec[i] = cfg.releaseSec;
    voices.attackLayer[i] = cfg.attackLayer;
    voices.bassLayer[i] = cfg.bassLayer;
    voices.leadLayer[i] = cfg.leadLayer;
    voices.chordLayer[i] = cfg.chordLayer;
    voices.padLayer[i] = cfg.padLayer;
    voices.pluckLayer[i] = cfg.pluckLayer;
    voices.stringLayer[i] = cfg.stringLayer;
    voices.bodyLayer[i] = cfg.bodyLayer;
    voices.harmonicLayer[i] = cfg.harmonicLayer;
    voices.powerChordLayer[i] = cfg.powerChordLayer;
    voices.chugLayer[i] = cfg.chugLayer;
    voices.ampCabLayer[i] = cfg.ampCabLayer;
    voices.drumBus[i] = cfg.drumBus;
    voices.expressionMap[i] = cfg.expressionMap;
    voices.expressionDefaultVelocityNorm[i] =
        std::clamp(static_cast<double>(std::clamp(velocity, 0, 127)) / 127.0, 0.0, 1.0);
    voices.expressionDefaultAmpVelocity[i] = VelocityToGain(velocity);
    voices.runtimeAmp[i] = cfg.amp;
    voices.runtimeDefaultAmpVelocity[i] = voices.expressionDefaultAmpVelocity[i];
    voices.runtimeHasDrumBus[i] = cfg.drumBus.enabled ? 1 : 0;
    const config::SourceKind sourceKind = config::SourceConfigKind(cfg.source);
    const config::SourceCapability sourceCapability = config::SourceCapabilityOf(sourceKind);
    voices.runtimeSourceKind[i] = static_cast<uint8_t>(config::SourceKindToIndex(sourceKind));
    voices.runtimeSourceHasModTargets[i] = sourceCapability.hasModTargets ? 1 : 0;
    voices.runtimeSourceIsOneShot[i] = sourceCapability.isOneShot ? 1 : 0;
    uint32_t layerMask = 0;
    if (cfg.attackLayer.enabled && cfg.attackLayer.level > 0.0) layerMask |= kVoiceLayerAttack;
    if (cfg.bassLayer.enabled && cfg.bassLayer.level > 0.0) layerMask |= kVoiceLayerBass;
    if (cfg.leadLayer.enabled && cfg.leadLayer.level > 0.0) layerMask |= kVoiceLayerLead;
    if (cfg.chordLayer.enabled && cfg.chordLayer.level > 0.0) layerMask |= kVoiceLayerChord;
    if (cfg.padLayer.enabled && cfg.padLayer.level > 0.0) layerMask |= kVoiceLayerPad;
    if (cfg.pluckLayer.enabled && cfg.pluckLayer.level > 0.0) layerMask |= kVoiceLayerPluck;
    if (cfg.stringLayer.enabled && cfg.stringLayer.level > 0.0) layerMask |= kVoiceLayerString;
    if (cfg.bodyLayer.enabled && cfg.bodyLayer.mix > 0.0) layerMask |= kVoiceLayerBody;
    if (cfg.harmonicLayer.enabled && cfg.harmonicLayer.level > 0.0) layerMask |= kVoiceLayerHarmonic;
    if (cfg.powerChordLayer.enabled && cfg.powerChordLayer.level > 0.0) layerMask |= kVoiceLayerPowerChord;
    if (cfg.chugLayer.enabled && cfg.chugLayer.level > 0.0) layerMask |= kVoiceLayerChug;
    if (cfg.ampCabLayer.enabled) layerMask |= kVoiceLayerAmpCab;
    voices.layerMask[i] = layerMask;
    uint8_t fastPathMask = 0;
    if (!cfg.expressionMap.enabled) fastPathMask |= kVoiceFastPathExpressionDisabled;
    if (cfg.portamentoTimeSec <= 0.0) fastPathMask |= kVoiceFastPathPortamentoDisabled;
    voices.fastPathMask[i] = fastPathMask;
}

void InitializeVoiceAtIndex(
    Voice& voices,
    size_t i,
    const InstrumentSoundConfig& cfg,
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
    voices.sustainedPendingOff[i] = 0;

    ApplyVoiceConfig(voices, i, cfg, e.velocity);
    ADSRState envState{};
    NoteOn(envState);
    voices.env[i] = envState;

    voices.phase[i] = 0.0;
    voices.phaseInc[i] = NoteNumberToFreq(e.noteNumber) / sampleRate;
    voices.ageSec[i] = 0.0;
    voices.attackPhase[i] = 0.0;
    voices.attackNoiseState[i] = 0x9E3779B9u
        ^ static_cast<uint32_t>((std::max)(0, e.noteNumber) * 73856093)
        ^ static_cast<uint32_t>((std::max)(0, e.channel) * 19349663)
        ^ static_cast<uint32_t>((std::max)(0, e.noteInstanceID) * 83492791);
    if (voices.attackNoiseState[i] == 0)
    {
        voices.attackNoiseState[i] = 0xA5A5A5A5u;
    }
    voices.bassPhase[i] = 0.0;
    voices.bassFocusPhase[i] = 0.0;
    voices.bassLpState[i] = 0.0;
    voices.leadPhase[i] = 0.0;
    voices.leadDetunePhase[i] = 0.0;
    voices.chordPhase[i] = {};
    voices.chordLpState[i] = 0.0;
    voices.padPhase[i] = 0.0;
    voices.padDetunePhase[i] = 0.0;
    voices.padMotionPhase[i] = 0.0;
    voices.padLpState[i] = 0.0;
    voices.pluckPhase[i] = 0.0;
    voices.pluckLpState[i] = 0.0;
    voices.stringPhaseA[i] = 0.0;
    voices.stringPhaseB[i] = 0.25;
    voices.stringMotionPhase[i] = 0.0;
    voices.bodyStateL[i] = {};
    voices.bodyStateR[i] = {};
    voices.bodyPhase[i] = {};
    voices.powerChordPhase[i] = {};
    voices.powerChordPhaseR[i] = {};
    voices.chugPhase[i] = 0.0;
    voices.chugBodyPhase[i] = 0.0;
    voices.chugLpState[i] = 0.0;
    voices.ampCabLpStateL[i] = 0.0;
    voices.ampCabLpStateR[i] = 0.0;
    voices.ampCabHpStateL[i] = 0.0;
    voices.ampCabHpStateR[i] = 0.0;

    // ポルタメント初期化
    const double targetHz = NoteNumberToFreq(e.noteNumber);
    voices.portamentoTargetHz[i] = targetHz;
    voices.portamentoTimeSec[i] = cfg.portamentoTimeSec;

    if (cfg.portamentoTimeSec > 0.0)
    {
        // 同一チャンネルで直前に発音中のボイスのピッチを開始点にする。
        double startHz = targetHz;
        for (size_t j = 0; j < voices.size(); j++)
        {
            if (j == i) continue;
            if (voices.pendingRemove[j] != 0) continue;
            if (voices.channel[j] != e.channel) continue;
            startHz = voices.portamentoPitchHz[j];
            break;
        }
        voices.portamentoPitchHz[i] = startHz;
    }
    else
    {
        voices.portamentoPitchHz[i] = targetHz;
    }

    if (const auto* wave = std::get_if<WaveformConfig>(&cfg.source))
    {
        voices.sourceState[i] = WaveformVoiceState{};
        auto& ws = std::get<WaveformVoiceState>(voices.sourceState[i]);
        InitWaveformLikeVoiceStateCommon(*wave, ws, sampleRate, e.noteNumber);
    }
    else if (const auto* analog = std::get_if<AnalogConfig>(&cfg.source))
    {
        voices.sourceState[i] = AnalogVoiceState{};
        auto& as = std::get<AnalogVoiceState>(voices.sourceState[i]);
        InitWaveformLikeVoiceStateCommon(*analog, as, sampleRate, e.noteNumber);

        // ボイス固有のドリフト位相オフセット（0..1）を確定する。
        // 目的: 同時発音ボイスのドリフトが同位相にならないようにする。
        as.driftPhase = 0.0;
        as.driftPhaseOffset = static_cast<double>((i * 37 + e.channel * 13) % 97) / 97.0;
    }
    else if (const auto* fm = std::get_if<FmConfig>(&cfg.source))
    {
        voices.sourceState[i] = FmVoiceState{};
        auto& fs = std::get<FmVoiceState>(voices.sourceState[i]);
        fs.chip = std::make_shared<YmfmVoice>(sampleRate, fm->chip);
        ResetModulationState(fs.modulation);
        NoteOnModulation(fs.modulation, fm->modulation);
        SetFilterSampleRate(fs.filter, sampleRate);
        SetFilterMode(fs.filter, fm->filterMode);
        SetFilterCutoffHz(fs.filter, fm->filterCutoffHz);
        SetFilterResonance(fs.filter, fm->filterResonance);
        SetFilterDrive(fs.filter, fm->filterDrive);
        ResetFilterState(fs.filter);
        if (fm->drive > 0.0)
        {
            const double k = fm->drive * 20.0;
            const double tanhK = std::tanh(k);
            fs.driveNorm = (tanhK > 1e-9) ? (1.0 / tanhK) : 1.0;
        }
        else
        {
            fs.driveNorm = 1.0;
        }
    }
    else if (const auto* noise = std::get_if<NoiseConfig>(&cfg.source))
    {
        voices.sourceState[i] = NoiseVoiceState{};
        auto& ns = std::get<NoiseVoiceState>(voices.sourceState[i]);
        SetFilterSampleRate(ns.filter, sampleRate);
        SetFilterMode(ns.filter, noise->filterMode);
        SetFilterCutoffHz(ns.filter, noise->filterCutoffHz);
        SetFilterResonance(ns.filter, noise->filterResonance);
        SetFilterDrive(ns.filter, noise->filterDrive);
        ResetFilterState(ns.filter);
    }
    else if (const auto* drum = std::get_if<DrumConfig>(&cfg.source))
    {
        voices.sourceState[i] = DrumVoiceState{};
        auto& ds = std::get<DrumVoiceState>(voices.sourceState[i]);
        InitDrumVoice(*drum, ds, voices.phaseInc[i], sampleRate);
        const double velNorm = std::clamp(static_cast<double>(e.velocity) / 127.0, 0.0, 1.0);
        uint32_t seed = 0x9E3779B9u
            ^ static_cast<uint32_t>((std::max)(0, e.noteNumber) * 73856093)
            ^ static_cast<uint32_t>((std::max)(0, e.channel) * 19349663)
            ^ static_cast<uint32_t>((std::max)(0, e.noteInstanceID) * 83492791);
        if (seed == 0) seed = 0xA5A5A5A5u;
        ds.noiseState = seed;
        auto nextUnit = [&]() {
            seed = seed * 1664525u + 1013904223u;
            return (static_cast<double>((seed >> 8) & 0x00FFFFFFu) / 8388607.5) - 1.0;
        };
        ds.velocityNorm = velNorm;
        ds.pitchRatio = std::exp2((nextUnit() * drum->humanizePitchCents) / 1200.0);
        ds.decayScale = std::max(0.1, 1.0 + nextUnit() * drum->humanizeDecayPct);
        ds.noiseState = seed;
        for (size_t b = 0; b < ds.burstDelaySec.size(); b++)
        {
            ds.burstDelaySec[b] = static_cast<double>(b) * 0.008 + std::max(0.0, nextUnit()) * 0.002;
        }
    }
    else if (std::get_if<PsgConfig>(&cfg.source))
    {
        voices.sourceState[i] = PsgVoiceState{};
    }
    else
    {
        voices.sourceState[i] = DrumKitVoiceState{};
    }
}

bool TryRestartVoiceOnRetrigger(Voice& voices, const InstrumentSoundConfig& cfg, const MIDIEvent& e, int sampleRate)
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

bool TryHandleVoiceLimitAndSteal(Voice& voices, const InstrumentSoundConfig& cfg, const MIDIEvent& e, int sampleRate)
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
    sustainedPendingOff.reserve(n);
    amp.reserve(n);
    attackSec.reserve(n);
    decaySec.reserve(n);
    sustainLevel.reserve(n);
    releaseSec.reserve(n);
    attackLayer.reserve(n);
    bassLayer.reserve(n);
    leadLayer.reserve(n);
    chordLayer.reserve(n);
    padLayer.reserve(n);
    pluckLayer.reserve(n);
    stringLayer.reserve(n);
    bodyLayer.reserve(n);
    harmonicLayer.reserve(n);
    powerChordLayer.reserve(n);
    chugLayer.reserve(n);
    ampCabLayer.reserve(n);
    drumBus.reserve(n);
    expressionMap.reserve(n);
    expressionDefaultVelocityNorm.reserve(n);
    expressionDefaultAmpVelocity.reserve(n);
    layerMask.reserve(n);
    fastPathMask.reserve(n);
    runtimeAmp.reserve(n);
    runtimeDefaultAmpVelocity.reserve(n);
    runtimeHasDrumBus.reserve(n);
    runtimeSourceKind.reserve(n);
    runtimeSourceHasModTargets.reserve(n);
    runtimeSourceIsOneShot.reserve(n);
    env.reserve(n);
    phase.reserve(n);
    phaseInc.reserve(n);
    ageSec.reserve(n);
    attackPhase.reserve(n);
    attackNoiseState.reserve(n);
    bassPhase.reserve(n);
    bassFocusPhase.reserve(n);
    bassLpState.reserve(n);
    leadPhase.reserve(n);
    leadDetunePhase.reserve(n);
    chordPhase.reserve(n);
    chordLpState.reserve(n);
    padPhase.reserve(n);
    padDetunePhase.reserve(n);
    padMotionPhase.reserve(n);
    padLpState.reserve(n);
    pluckPhase.reserve(n);
    pluckLpState.reserve(n);
    stringPhaseA.reserve(n);
    stringPhaseB.reserve(n);
    stringMotionPhase.reserve(n);
    bodyStateL.reserve(n);
    bodyStateR.reserve(n);
    bodyPhase.reserve(n);
    harmonicPhase.reserve(n);
    harmonicPhaseR.reserve(n);
    powerChordPhase.reserve(n);
    powerChordPhaseR.reserve(n);
    chugPhase.reserve(n);
    chugBodyPhase.reserve(n);
    chugLpState.reserve(n);
    ampCabLpStateL.reserve(n);
    ampCabLpStateR.reserve(n);
    ampCabHpStateL.reserve(n);
    ampCabHpStateR.reserve(n);
    portamentoPitchHz.reserve(n);
    portamentoTargetHz.reserve(n);
    portamentoTimeSec.reserve(n);
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
    sustainedPendingOff.clear();
    amp.clear();
    attackSec.clear();
    decaySec.clear();
    sustainLevel.clear();
    releaseSec.clear();
    attackLayer.clear();
    bassLayer.clear();
    leadLayer.clear();
    chordLayer.clear();
    padLayer.clear();
    pluckLayer.clear();
    stringLayer.clear();
    bodyLayer.clear();
    harmonicLayer.clear();
    powerChordLayer.clear();
    chugLayer.clear();
    ampCabLayer.clear();
    drumBus.clear();
    expressionMap.clear();
    expressionDefaultVelocityNorm.clear();
    expressionDefaultAmpVelocity.clear();
    layerMask.clear();
    fastPathMask.clear();
    runtimeAmp.clear();
    runtimeDefaultAmpVelocity.clear();
    runtimeHasDrumBus.clear();
    runtimeSourceKind.clear();
    runtimeSourceHasModTargets.clear();
    runtimeSourceIsOneShot.clear();
    env.clear();
    phase.clear();
    phaseInc.clear();
    ageSec.clear();
    attackPhase.clear();
    attackNoiseState.clear();
    bassPhase.clear();
    bassFocusPhase.clear();
    bassLpState.clear();
    leadPhase.clear();
    leadDetunePhase.clear();
    chordPhase.clear();
    chordLpState.clear();
    padPhase.clear();
    padDetunePhase.clear();
    padMotionPhase.clear();
    padLpState.clear();
    pluckPhase.clear();
    pluckLpState.clear();
    stringPhaseA.clear();
    stringPhaseB.clear();
    stringMotionPhase.clear();
    bodyStateL.clear();
    bodyStateR.clear();
    bodyPhase.clear();
    harmonicPhase.clear();
    harmonicPhaseR.clear();
    powerChordPhase.clear();
    powerChordPhaseR.clear();
    chugPhase.clear();
    chugBodyPhase.clear();
    chugLpState.clear();
    ampCabLpStateL.clear();
    ampCabLpStateR.clear();
    ampCabHpStateL.clear();
    ampCabHpStateR.clear();
    portamentoPitchHz.clear();
    portamentoTargetHz.clear();
    portamentoTimeSec.clear();
    sourceState.clear();
}

void Voice::UpdateSound(size_t i, const InstrumentSoundConfig& cfg, int sampleRate)
{
    // A drum hit keeps its body/decay; the next hit uses the edited kit.
    if (std::holds_alternative<DrumKitConfig>(cfg.source))
    {
        amp[i] = runtimeAmp[i] = cfg.amp;
        return;
    }
    const auto* oldFm = std::get_if<FmConfig>(&source[i]);
    const auto* newFm = std::get_if<FmConfig>(&cfg.source);
    const bool sameChip = oldFm && newFm && oldFm->chip == newFm->chip;
    if (source[i].index() != cfg.source.index() || (newFm && !sameChip))
    {
        MIDIEvent event{};
        event.channel = channel[i]; event.noteNumber = noteNumber[i];
        event.velocity = velocity[i]; event.noteInstanceID = noteInstanceID[i]; event.isNoteOn = true;
        const auto wasReleased = released[i];
        const auto wasSustained = sustainedPendingOff[i];
        InitializeVoiceAtIndex(*this, i, cfg, event, sampleRate);
        released[i] = wasReleased;
        sustainedPendingOff[i] = wasSustained;
        if (wasReleased) { NoteOff(env[i]); NoteOffVoiceModulation(sourceState[i]); }
        return;
    }
    source[i] = cfg.source;
    ApplyVoiceConfig(*this, i, cfg, velocity[i]);
    portamentoTimeSec[i] = cfg.portamentoTimeSec;
    std::visit([&](const auto& src)
    {
        using Config = std::decay_t<decltype(src)>;
        if constexpr (std::is_same_v<Config, FmConfig>)
        {
            auto& st = std::get<FmVoiceState>(sourceState[i]);
            SetFilterMode(st.filter, src.filterMode);
            SetFilterResonance(st.filter, src.filterResonance);
            st.modulation.splitRatePrepared = false;
        }
        else if constexpr (std::is_same_v<Config, WaveformConfig> || std::is_same_v<Config, AnalogConfig>)
        {
            using State = std::conditional_t<std::is_same_v<Config, WaveformConfig>, WaveformVoiceState, AnalogVoiceState>;
            auto& st = std::get<State>(sourceState[i]);
            State cache{};
            InitWaveformLikeVoiceStateCommon(src, cache, sampleRate, noteNumber[i]);
            st.unisonDetuneRatio = cache.unisonDetuneRatio;
            st.filterKeytrackRatio = cache.filterKeytrackRatio;
            st.driveNorm = cache.driveNorm;
            SetFilterMode(st.filter, src.filterMode);
            SetFilterResonance(st.filter, src.filterResonance);
            SetSmoothingTimeMs(st.ampSmoothing, src.smoothing.ampTimeMs);
            SetSmoothingTimeMs(st.pitchSmoothing, src.smoothing.pitchTimeMs);
            SetSmoothingTimeMs(st.filterCutoffSmoothing, src.smoothing.filterCutoffTimeMs);
            st.modulation.splitRatePrepared = false;
        }
        else if constexpr (std::is_same_v<Config, NoiseConfig>)
        {
            auto& st = std::get<NoiseVoiceState>(sourceState[i]);
            SetFilterMode(st.filter, src.filterMode);
            SetFilterCutoffHz(st.filter, src.filterCutoffHz);
            SetFilterResonance(st.filter, src.filterResonance);
        }
    }, cfg.source);
}

void Voice::AddVoice(const InstrumentSoundConfig& cfg, const MIDIEvent& e, int sampleRate)
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
    sustainedPendingOff.push_back(0);

    amp.push_back(0.0);
    attackSec.push_back(0.0);
    decaySec.push_back(0.0);
    sustainLevel.push_back(0.0);
    releaseSec.push_back(0.0);
    attackLayer.emplace_back();
    bassLayer.emplace_back();
    leadLayer.emplace_back();
    chordLayer.emplace_back();
    padLayer.emplace_back();
    pluckLayer.emplace_back();
    stringLayer.emplace_back();
    bodyLayer.emplace_back();
    harmonicLayer.emplace_back();
    powerChordLayer.emplace_back();
    chugLayer.emplace_back();
    ampCabLayer.emplace_back();
    drumBus.emplace_back();
    expressionMap.emplace_back();
    expressionDefaultVelocityNorm.push_back(1.0);
    expressionDefaultAmpVelocity.push_back(1.0);
    layerMask.push_back(0);
    fastPathMask.push_back(0);
    runtimeAmp.push_back(0.0);
    runtimeDefaultAmpVelocity.push_back(1.0);
    runtimeHasDrumBus.push_back(0);
    runtimeSourceKind.push_back(0);
    runtimeSourceHasModTargets.push_back(0);
    runtimeSourceIsOneShot.push_back(0);
    env.emplace_back();

    phase.push_back(0.0);
    phaseInc.push_back(0.0);
    ageSec.push_back(0.0);
    attackPhase.push_back(0.0);
    attackNoiseState.push_back(0);
    bassPhase.push_back(0.0);
    bassFocusPhase.push_back(0.0);
    bassLpState.push_back(0.0);
    leadPhase.push_back(0.0);
    leadDetunePhase.push_back(0.0);
    chordPhase.emplace_back();
    chordLpState.push_back(0.0);
    padPhase.push_back(0.0);
    padDetunePhase.push_back(0.0);
    padMotionPhase.push_back(0.0);
    padLpState.push_back(0.0);
    pluckPhase.push_back(0.0);
    pluckLpState.push_back(0.0);
    stringPhaseA.push_back(0.0);
    stringPhaseB.push_back(0.0);
    stringMotionPhase.push_back(0.0);
    bodyStateL.emplace_back();
    bodyStateR.emplace_back();
    bodyPhase.emplace_back();
    harmonicPhase.emplace_back();
    harmonicPhaseR.emplace_back();
    powerChordPhase.emplace_back();
    powerChordPhaseR.emplace_back();
    chugPhase.push_back(0.0);
    chugBodyPhase.push_back(0.0);
    chugLpState.push_back(0.0);
    ampCabLpStateL.push_back(0.0);
    ampCabLpStateR.push_back(0.0);
    ampCabHpStateL.push_back(0.0);
    ampCabHpStateR.push_back(0.0);
    portamentoPitchHz.push_back(0.0);
    portamentoTargetHz.push_back(0.0);
    portamentoTimeSec.push_back(0.0);
    sourceState.emplace_back();

    const size_t i = size() - 1;
    InitializeVoiceAtIndex(*this, i, cfg, e, sampleRate);
}

void Voice::MarkNoteOff(int ch, int note, int noteInstanceID, bool holdBySustain)
{
    auto applyRelease = [&](size_t i, bool hasModTargets)
    {
        if (holdBySustain)
        {
            sustainedPendingOff[i] = 1;
            return;
        }
        NoteOff(env[i]);
        if (hasModTargets)
        {
            NoteOffVoiceModulation(sourceState[i]);
        }
        released[i] = 1;
        sustainedPendingOff[i] = 0;
    };

    // 原則: NoteOn/NoteOff の対応IDで閉じる。
    // 互換性: IDが無い/不一致の場合は旧ロジック(ch+note)へフォールバックする。
    if (noteInstanceID >= 0)
    {
        for (size_t i = 0; i < size(); i++)
        {
            if (runtimeSourceIsOneShot[i] != 0)
            {
                continue;
            }
            if (released[i] == 0 && this->noteInstanceID[i] == noteInstanceID)
            {
                applyRelease(i, runtimeSourceHasModTargets[i] != 0);
                return;
            }
        }
    }

    // 旧互換経路: 既存データや異常入力でIDが取れない場合のみ利用。
    for (size_t i = 0; i < size(); i++)
    {
        if (runtimeSourceIsOneShot[i] != 0)
        {
            continue;
        }
        if (released[i] == 0 && noteNumber[i] == note && channel[i] == ch)
        {
            applyRelease(i, runtimeSourceHasModTargets[i] != 0);
            break;
        }
    }
}

void Voice::ReleaseSustained(int ch)
{
    for (size_t i = 0; i < size(); i++)
    {
        if (released[i] != 0 || sustainedPendingOff[i] == 0 || channel[i] != ch)
        {
            continue;
        }
        if (runtimeSourceIsOneShot[i] != 0)
        {
            sustainedPendingOff[i] = 0;
            continue;
        }
        NoteOff(env[i]);
        if (runtimeSourceHasModTargets[i] != 0)
        {
            NoteOffVoiceModulation(sourceState[i]);
        }
        released[i] = 1;
        sustainedPendingOff[i] = 0;
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
    CompactVectorByKeep(sustainedPendingOff, keepScratch);
    CompactVectorByKeep(amp, keepScratch);
    CompactVectorByKeep(attackSec, keepScratch);
    CompactVectorByKeep(decaySec, keepScratch);
    CompactVectorByKeep(sustainLevel, keepScratch);
    CompactVectorByKeep(releaseSec, keepScratch);
    CompactVectorByKeep(attackLayer, keepScratch);
    CompactVectorByKeep(bassLayer, keepScratch);
    CompactVectorByKeep(leadLayer, keepScratch);
    CompactVectorByKeep(chordLayer, keepScratch);
    CompactVectorByKeep(padLayer, keepScratch);
    CompactVectorByKeep(pluckLayer, keepScratch);
    CompactVectorByKeep(stringLayer, keepScratch);
    CompactVectorByKeep(bodyLayer, keepScratch);
    CompactVectorByKeep(harmonicLayer, keepScratch);
    CompactVectorByKeep(powerChordLayer, keepScratch);
    CompactVectorByKeep(chugLayer, keepScratch);
    CompactVectorByKeep(ampCabLayer, keepScratch);
    CompactVectorByKeep(drumBus, keepScratch);
    CompactVectorByKeep(expressionMap, keepScratch);
    CompactVectorByKeep(expressionDefaultVelocityNorm, keepScratch);
    CompactVectorByKeep(expressionDefaultAmpVelocity, keepScratch);
    CompactVectorByKeep(layerMask, keepScratch);
    CompactVectorByKeep(fastPathMask, keepScratch);
    CompactVectorByKeep(runtimeAmp, keepScratch);
    CompactVectorByKeep(runtimeDefaultAmpVelocity, keepScratch);
    CompactVectorByKeep(runtimeHasDrumBus, keepScratch);
    CompactVectorByKeep(runtimeSourceKind, keepScratch);
    CompactVectorByKeep(runtimeSourceHasModTargets, keepScratch);
    CompactVectorByKeep(runtimeSourceIsOneShot, keepScratch);
    CompactVectorByKeep(env, keepScratch);
    CompactVectorByKeep(phase, keepScratch);
    CompactVectorByKeep(phaseInc, keepScratch);
    CompactVectorByKeep(ageSec, keepScratch);
    CompactVectorByKeep(attackPhase, keepScratch);
    CompactVectorByKeep(attackNoiseState, keepScratch);
    CompactVectorByKeep(bassPhase, keepScratch);
    CompactVectorByKeep(bassFocusPhase, keepScratch);
    CompactVectorByKeep(bassLpState, keepScratch);
    CompactVectorByKeep(leadPhase, keepScratch);
    CompactVectorByKeep(leadDetunePhase, keepScratch);
    CompactVectorByKeep(chordPhase, keepScratch);
    CompactVectorByKeep(chordLpState, keepScratch);
    CompactVectorByKeep(padPhase, keepScratch);
    CompactVectorByKeep(padDetunePhase, keepScratch);
    CompactVectorByKeep(padMotionPhase, keepScratch);
    CompactVectorByKeep(padLpState, keepScratch);
    CompactVectorByKeep(pluckPhase, keepScratch);
    CompactVectorByKeep(pluckLpState, keepScratch);
    CompactVectorByKeep(stringPhaseA, keepScratch);
    CompactVectorByKeep(stringPhaseB, keepScratch);
    CompactVectorByKeep(stringMotionPhase, keepScratch);
    CompactVectorByKeep(bodyStateL, keepScratch);
    CompactVectorByKeep(bodyStateR, keepScratch);
    CompactVectorByKeep(bodyPhase, keepScratch);
    CompactVectorByKeep(harmonicPhase, keepScratch);
    CompactVectorByKeep(harmonicPhaseR, keepScratch);
    CompactVectorByKeep(powerChordPhase, keepScratch);
    CompactVectorByKeep(powerChordPhaseR, keepScratch);
    CompactVectorByKeep(chugPhase, keepScratch);
    CompactVectorByKeep(chugBodyPhase, keepScratch);
    CompactVectorByKeep(chugLpState, keepScratch);
    CompactVectorByKeep(ampCabLpStateL, keepScratch);
    CompactVectorByKeep(ampCabLpStateR, keepScratch);
    CompactVectorByKeep(ampCabHpStateL, keepScratch);
    CompactVectorByKeep(ampCabHpStateR, keepScratch);
    CompactVectorByKeep(portamentoPitchHz, keepScratch);
    CompactVectorByKeep(portamentoTargetHz, keepScratch);
    CompactVectorByKeep(portamentoTimeSec, keepScratch);
    CompactVectorByKeep(sourceState, keepScratch);

    return removed;
}
