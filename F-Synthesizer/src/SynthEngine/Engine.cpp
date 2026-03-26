#include "Internal.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;

void PanToStereoGains(double pan, double& outL, double& outR)
{
    const double p = std::clamp(pan, -1.0, 1.0);
    const double angle = (p + 1.0) * (kPi * 0.25);
    outL = std::cos(angle);
    outR = std::sin(angle);
}

void CleanupVoices(RenderState& state)
{
    if (state.voices.empty() || state.pendingRemoveCount == 0)
    {
        return;
    }

    // 毎回の一時vector確保を避けるため、RenderState内の作業バッファを再利用する。
    const size_t removed = state.voices.CleanupPending(state.cleanupKeepScratch);

    if (removed > 0)
    {
        state.pendingRemoveCount = 0;
    }
}

std::vector<TempoEvent> NormalizeTempoEvents(const std::vector<TempoEvent>* tempoEvents)
{
    std::vector<TempoEvent> sorted = (tempoEvents != nullptr) ? *tempoEvents : std::vector<TempoEvent>{};
    std::sort(sorted.begin(), sorted.end(), [](const TempoEvent& a, const TempoEvent& b)
    {
        return a.tick < b.tick;
    });
    if (sorted.empty() || sorted.front().tick != 0)
    {
        TempoEvent te{};
        te.tick = 0;
        te.bpm = 120.0;
        sorted.insert(sorted.begin(), te);
    }
    return sorted;
}

void BuildTempoSampleMap(
    RenderState& state,
    const std::vector<TempoEvent>* tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    double renderStartSec)
{
    state.tempoChangeSamples.clear();
    state.tempoChangeBpms.clear();
    state.tempoChangeIndex = 0;
    state.currentBpm = 120.0;
    if (ticksPerQuarter <= 0 || sampleRate <= 0)
    {
        return;
    }

    const std::vector<TempoEvent> sorted = NormalizeTempoEvents(tempoEvents);
    const double startOffsetSamples = (std::max)(0.0, renderStartSec) * sampleRate;

    int prevTick = 0;
    double prevBpm = sorted.front().bpm;
    double absSample = 0.0;
    double bpmAtStart = prevBpm;
    bool bpmAtStartResolved = (startOffsetSamples <= 0.0);
    for (size_t i = 1; i < sorted.size(); i++)
    {
        const int tick = sorted[i].tick;
        const int deltaTick = tick - prevTick;
        const double samplesPerTick = ((60.0 / (std::max)(1e-6, prevBpm)) * sampleRate) / ticksPerQuarter;
        absSample += deltaTick * samplesPerTick;
        if (!bpmAtStartResolved && absSample >= startOffsetSamples)
        {
            bpmAtStart = prevBpm;
            bpmAtStartResolved = true;
        }
        if (absSample >= startOffsetSamples)
        {
            const int relSample = static_cast<int>(absSample - startOffsetSamples);
            state.tempoChangeSamples.push_back((std::max)(0, relSample));
            state.tempoChangeBpms.push_back(sorted[i].bpm);
        }
        prevTick = tick;
        prevBpm = sorted[i].bpm;
    }
    if (!bpmAtStartResolved)
    {
        bpmAtStart = prevBpm;
    }
    state.currentBpm = bpmAtStart;
}

int ClampDelaySamples(double sec, int sampleRate, int maxSamples)
{
    const int s = static_cast<int>(sec * sampleRate);
    return std::clamp(s, 1, (std::max)(1, maxSamples - 1));
}

double QuantizeBitDepth(double sample, int bits)
{
    const int clampedBits = std::clamp(bits, 1, 16);
    if (clampedBits >= 16)
    {
        return sample;
    }

    const int levels = 1 << clampedBits;
    const double step = 2.0 / static_cast<double>((std::max)(1, levels - 1));
    const double clamped = std::clamp(sample, -1.0, 1.0);
    const double normalized = (clamped + 1.0) / step;
    return std::clamp(std::round(normalized) * step - 1.0, -1.0, 1.0);
}

void EnsureEffectBuffers(RenderState& state, int sampleRate)
{
    const int delayLen = (std::max)(sampleRate * 4, 1);
    if (state.delayBufferL.size() != static_cast<size_t>(delayLen))
    {
        state.delayBufferL.assign(delayLen, 0.0);
        state.delayBufferR.assign(delayLen, 0.0);
        state.delayWrite = 0;
    }

    const int chorusLen = (std::max)(sampleRate / 4, 1);
    if (state.chorusBufferL.size() != static_cast<size_t>(chorusLen))
    {
        state.chorusBufferL.assign(chorusLen, 0.0);
        state.chorusBufferR.assign(chorusLen, 0.0);
        state.chorusWrite = 0;
        state.chorusPhase = 0.0;
    }

    const int combMs[4] = { 29, 37, 41, 53 };
    for (int i = 0; i < 4; i++)
    {
        const int n = (std::max)(1, sampleRate * combMs[i] / 1000);
        if (state.reverbCombL[i].size() != static_cast<size_t>(n))
        {
            state.reverbCombL[i].assign(n, 0.0);
            state.reverbCombR[i].assign(n, 0.0);
            state.reverbCombWrite[i] = 0;
        }
    }
    const int apMs[2] = { 5, 7 };
    for (int i = 0; i < 2; i++)
    {
        const int n = (std::max)(1, sampleRate * apMs[i] / 1000);
        if (state.reverbAllpassL[i].size() != static_cast<size_t>(n))
        {
            state.reverbAllpassL[i].assign(n, 0.0);
            state.reverbAllpassR[i].assign(n, 0.0);
            state.reverbAllpassWrite[i] = 0;
        }
    }
}

void ApplyDelay(RenderState& state, int sampleRate, double inL, double inR, double& outL, double& outR)
{
    outL = inL;
    outR = inR;
    const auto& cfg = state.effects.delay;
    if (!cfg.enabled || cfg.mix <= 0.0 || state.delayBufferL.empty())
    {
        return;
    }

    const double mix = std::clamp(cfg.mix, 0.0, 1.0);
    const double fb = std::clamp(cfg.feedback, 0.0, 0.95);
    double delaySec = std::clamp(cfg.timeSec, 0.01, 2.0);
    if (cfg.tempoSync)
    {
        const double bpm = (std::max)(1.0, state.currentBpm);
        delaySec = (60.0 / bpm) * std::clamp(cfg.syncBeats, 0.125, 4.0);
    }
    const int delaySamples = ClampDelaySamples(delaySec, sampleRate, static_cast<int>(state.delayBufferL.size()));
    const size_t size = state.delayBufferL.size();
    const size_t read = (state.delayWrite + size - static_cast<size_t>(delaySamples)) % size;
    const double wetL = state.delayBufferL[read];
    const double wetR = state.delayBufferR[read];
    state.delayBufferL[state.delayWrite] = inL + wetL * fb;
    state.delayBufferR[state.delayWrite] = inR + wetR * fb;
    state.delayWrite = (state.delayWrite + 1) % size;
    outL = inL * (1.0 - mix) + wetL * mix;
    outR = inR * (1.0 - mix) + wetR * mix;
}

void ApplyChorus(RenderState& state, int sampleRate, double inL, double inR, double& outL, double& outR)
{
    outL = inL;
    outR = inR;
    const auto& cfg = state.effects.chorus;
    if (!cfg.enabled || cfg.mix <= 0.0 || state.chorusBufferL.empty())
    {
        return;
    }

    const double mix = std::clamp(cfg.mix, 0.0, 1.0);
    const double fb = std::clamp(cfg.feedback, 0.0, 0.9);
    const double baseMs = std::clamp(cfg.baseDelayMs, 2.0, 40.0);
    const double depthMs = std::clamp(cfg.depthMs, 0.0, 20.0);
    const double rateHz = std::clamp(cfg.rateHz, 0.05, 8.0);
    state.chorusPhase += (2.0 * kPi * rateHz) / sampleRate;
    if (state.chorusPhase >= 2.0 * kPi)
    {
        state.chorusPhase -= 2.0 * kPi;
    }
    const double mod = std::sin(state.chorusPhase);
    const double dMsL = baseMs + depthMs * mod;
    const double dMsR = baseMs - depthMs * mod;
    const int dL = ClampDelaySamples(dMsL * 0.001, sampleRate, static_cast<int>(state.chorusBufferL.size()));
    const int dR = ClampDelaySamples(dMsR * 0.001, sampleRate, static_cast<int>(state.chorusBufferR.size()));
    const size_t size = state.chorusBufferL.size();
    const size_t readL = (state.chorusWrite + size - static_cast<size_t>(dL)) % size;
    const size_t readR = (state.chorusWrite + size - static_cast<size_t>(dR)) % size;
    const double wetL = state.chorusBufferL[readL];
    const double wetR = state.chorusBufferR[readR];
    state.chorusBufferL[state.chorusWrite] = inL + wetL * fb;
    state.chorusBufferR[state.chorusWrite] = inR + wetR * fb;
    state.chorusWrite = (state.chorusWrite + 1) % size;
    outL = inL * (1.0 - mix) + wetL * mix;
    outR = inR * (1.0 - mix) + wetR * mix;
}

double ReverbAllpassStep(std::vector<double>& buf, size_t& write, double x, double g)
{
    const double y = buf[write];
    const double out = -g * x + y;
    buf[write] = x + g * out;
    write++;
    if (write >= buf.size()) write = 0;
    return out;
}

void ApplyReverb(RenderState& state, double inL, double inR, double& outL, double& outR)
{
    outL = inL;
    outR = inR;
    const auto& cfg = state.effects.reverb;
    if (!cfg.enabled || cfg.mix <= 0.0)
    {
        return;
    }
    const double mix = std::clamp(cfg.mix, 0.0, 1.0);
    const double room = std::clamp(cfg.roomSize, 0.1, 1.0);
    const double damping = std::clamp(cfg.damping, 0.0, 1.0);
    const double combFb = std::clamp(0.55 + room * 0.35 - damping * 0.15, 0.1, 0.95);

    double wetL = 0.0;
    double wetR = 0.0;
    for (int i = 0; i < 4; i++)
    {
        auto& bL = state.reverbCombL[i];
        auto& bR = state.reverbCombR[i];
        size_t& w = state.reverbCombWrite[i];
        const double yL = bL[w];
        const double yR = bR[w];
        bL[w] = inL + yL * combFb;
        bR[w] = inR + yR * combFb;
        w++;
        if (w >= bL.size()) w = 0;
        wetL += yL;
        wetR += yR;
    }
    wetL *= 0.25;
    wetR *= 0.25;
    for (int i = 0; i < 2; i++)
    {
        wetL = ReverbAllpassStep(state.reverbAllpassL[i], state.reverbAllpassWrite[i], wetL, 0.5);
        wetR = ReverbAllpassStep(state.reverbAllpassR[i], state.reverbAllpassWrite[i], wetR, 0.5);
    }
    outL = inL * (1.0 - mix) + wetL * mix;
    outR = inR * (1.0 - mix) + wetR * mix;
}

void ApplyRetroEffects(RenderState& state, double inL, double inR, double& outL, double& outR)
{
    outL = inL;
    outR = inR;

    const double ratio = std::clamp(state.effects.sampleRateReducer.ratio, 0.0, 1.0);
    if (ratio < 1.0)
    {
        const int holdInterval = (std::max)(1, static_cast<int>(std::round(1.0 / (std::max)(ratio, 1e-6))));
        if (state.sampleRateReduceCounter <= 0)
        {
            state.sampleRateReduceHoldL = outL;
            state.sampleRateReduceHoldR = outR;
            state.sampleRateReduceCounter = holdInterval - 1;
        }
        else
        {
            state.sampleRateReduceCounter--;
        }

        outL = state.sampleRateReduceHoldL;
        outR = state.sampleRateReduceHoldR;
    }
    else
    {
        state.sampleRateReduceCounter = 0;
    }

    const int bits = std::clamp(state.effects.bitCrusher.bits, 1, 16);
    if (bits < 16)
    {
        outL = QuantizeBitDepth(outL, bits);
        outR = QuantizeBitDepth(outR, bits);
    }
}

StereoFrame ApplyMasterEffects(RenderState& state, int sampleRate, StereoFrame in)
{
    double l0 = 0.0;
    double r0 = 0.0;
    ApplyRetroEffects(state, in.left, in.right, l0, r0);

    double l1 = 0.0;
    double r1 = 0.0;
    ApplyChorus(state, sampleRate, l0, r0, l1, r1);

    double l2 = 0.0;
    double r2 = 0.0;
    ApplyDelay(state, sampleRate, l1, r1, l2, r2);

    double l3 = 0.0;
    double r3 = 0.0;
    ApplyReverb(state, l2, r2, l3, r3);
    return StereoFrame{ l3, r3 };
}
} // namespace

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const MasterEffectConfig& effects,
    const std::vector<TempoEvent>* tempoEvents,
    int ticksPerQuarter,
    double renderStartSec,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    if (canceled != nullptr)
    {
        *canceled = false;
    }

    RenderState state;
    // 初期化時にチャンネル状態を展開して、サンプルループ中の分岐/参照を最小化する。
    state.voices.reserve(256);
    state.cleanupKeepScratch.reserve(256);
    state.effects = effects;
    BuildTempoSampleMap(state, tempoEvents, ticksPerQuarter, sound.fs, renderStartSec);
    EnsureEffectBuffers(state, sound.fs);
    for (int i = 0; i < 16; i++)
    {
        state.channelCc7[i] = 1.0;
        state.channelCc11[i] = 1.0;
        state.channelPitch[i] = 1.0;
        state.channelModwheel[i] = 0.0;
        state.channelSustain[i] = false;
        state.channelBrightness[i] = 0.5;
        state.channelResonance[i] = 0.5;
        state.channelAdsrOffset[i] = ChannelAdsrOffset{};
        state.channelPortamentoTimeSec[i] = 0.0;
        state.channelPortamentoOn[i] = true;
        state.channelPitchBendNorm[i] = 0.0;
        state.channelPitchBendRangeSemis[i] = 2.0;
        state.channelRpnMsb[i] = 127;
        state.channelRpnLsb[i] = 127;
        const ChannelMixState& mix = channelMixStates[i];
        state.channelMute[i] = mix.mute;
        state.channelSolo[i] = mix.solo;
        if (mix.solo)
        {
            state.hasAnySolo = true;
        }
        double panL = 1.0;
        double panR = 1.0;
        PanToStereoGains(mix.pan, panL, panR);
        const double baseGain = mix.level * mix.gain;
        state.channelMixGainL[i] = baseGain * panL;
        state.channelMixGainR[i] = baseGain * panR;
    }
    // 可聴判定を先に確定し、RenderVoices内の条件分岐を1回にまとめる。
    for (int i = 0; i < 16; i++)
    {
        const bool soloVisible = !state.hasAnySolo || state.channelSolo[i];
        state.channelRenderable[i] = (!state.channelMute[i]) &&
            soloVisible &&
            (state.channelMixGainL[i] > 0.0 || state.channelMixGainR[i] > 0.0);
    }

    // 目的: 毎サンプルで削除圧縮を走らせず、一定間隔でまとめて掃除して負荷を抑える。
    // 前提: pendingRemove は短時間遅延しても音として破綻しない。
    // トレードオフ: 削除タイミングが最大 cleanupInterval サンプルぶん遅れる。
    const int cleanupInterval = 256;
    for (int i = 0; i < sound.length; i++)
    {
        // キャンセル確認も間引いて実施し、ホットパスの分岐コストを抑える。
        if (shouldCancel && ((i % cleanupInterval) == 0) && shouldCancel())
        {
            if (canceled != nullptr)
            {
                *canceled = true;
            }
            break;
        }

        ProcessEventsAtSample(events, i, channelConfigs, sound.fs, state);
        while (state.tempoChangeIndex < state.tempoChangeSamples.size() &&
            i >= state.tempoChangeSamples[state.tempoChangeIndex])
        {
            state.currentBpm = state.tempoChangeBpms[state.tempoChangeIndex];
            state.tempoChangeIndex++;
        }
        StereoFrame frame = RenderVoices(state, sound);
        frame = ApplyMasterEffects(state, sound.fs, frame);
        sound.dataL[i] = frame.left;
        sound.dataR[i] = frame.right;
        sound.data[i] = (frame.left + frame.right) * 0.5;

        if ((i % cleanupInterval) == 0)
        {
            CleanupVoices(state);
        }
    }
}
