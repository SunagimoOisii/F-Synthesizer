#include "Internal.h"

#include <cmath>
#include <type_traits>

#include "Oscillator.h"

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    void EnsureDrumFilters(Voice& v, double hpCut, double lpCut, int sampleRate)
    {
        if (v.drumHpAlpha <= 0.0)
        {
            v.drumHpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
        }
        if (v.drumLpAlpha <= 0.0)
        {
            v.drumLpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
        }
    }

    void PrepareDrumRelease(Voice& v)
    {
        if (!v.released && v.drumTime >= (v.attackSec + v.decaySec))
        {
            NoteOff(v.env);
            v.released = true;
        }
    }

    double RenderKickSample(const DrumConfig& src, Voice& v, int sampleRate)
    {
        double pitchFactor = 1.0;
        if (v.drumPitchDecaySec > 0.0)
        {
            pitchFactor += (v.drumPitchDrop - 1.0) * std::exp(-v.drumTime / v.drumPitchDecaySec);
        }
        double freq = v.drumBaseFreq * pitchFactor;
        v.phase += (freq / sampleRate);
        if (v.phase >= 1.0) v.phase -= 1.0;
        return SampleWavePhase(WaveType::Sine, v.phase);
    }

    double RenderSnareSample(const DrumConfig& src, Voice& v, int sampleRate)
    {
        double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.3;
        double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 0.7;
        double hpCut = (src.hpCut > 0.0) ? src.hpCut : 1200.0;
        double lpCut = (src.lpCut > 0.0) ? src.lpCut : 6000.0;
        WaveType toneWave = (src.toneWave >= 0) ? static_cast<WaveType>(src.toneWave) : WaveType::Sine;
        NoiseType noiseType = (src.noiseType >= 0) ? static_cast<NoiseType>(src.noiseType) : NoiseType::White;
        EnsureDrumFilters(v, hpCut, lpCut, sampleRate);
        v.phase += v.drumBaseFreq / sampleRate;
        if (v.phase >= 1.0) v.phase -= 1.0;
        double tone = SampleWavePhase(toneWave, v.phase);
        double noise = SampleNoise(noiseType);
        double hp = v.drumHpAlpha * (v.drumHpPrev + noise - v.drumNoisePrev);
        double lp = (1.0 - v.drumLpAlpha) * hp + v.drumLpAlpha * v.drumLpPrev;
        v.drumNoisePrev = noise;
        v.drumHpPrev = hp;
        v.drumLpPrev = lp;
        return toneLevel * tone + noiseLevel * lp;
    }

    double RenderHatSample(const DrumConfig& src, Voice& v, int sampleRate)
    {
        double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 1.0;
        double hpCut = (src.hpCut > 0.0) ? src.hpCut : 6000.0;
        double lpCut = (src.lpCut > 0.0) ? src.lpCut : 12000.0;
        double toneFreq = (src.toneFreq > 0.0) ? src.toneFreq : 8000.0;
        double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.2;
        WaveType toneWave = (src.toneWave >= 0) ? static_cast<WaveType>(src.toneWave) : WaveType::Square;
        NoiseType noiseType = (src.noiseType >= 0) ? static_cast<NoiseType>(src.noiseType) : NoiseType::White;
        EnsureDrumFilters(v, hpCut, lpCut, sampleRate);
        double noise = SampleNoise(noiseType);
        double hp = v.drumHpAlpha * (v.drumHpPrev + noise - v.drumNoisePrev);
        double lp = (1.0 - v.drumLpAlpha) * hp + v.drumLpAlpha * v.drumLpPrev;
        v.drumNoisePrev = noise;
        v.drumHpPrev = hp;
        v.drumLpPrev = lp;
        v.phase += toneFreq / sampleRate;
        if (v.phase >= 1.0) v.phase -= 1.0;
        double tone = SampleWavePhase(toneWave, v.phase);
        return noiseLevel * lp + toneLevel * tone;
    }

    double RenderDrumSample(const DrumConfig& src, Voice& v, double dt, int sampleRate)
    {
        PrepareDrumRelease(v);

        double w = 0.0;
        if (src.type == DrumType::Kick)
        {
            w = RenderKickSample(src, v, sampleRate);
        }
        else if (src.type == DrumType::Snare)
        {
            w = RenderSnareSample(src, v, sampleRate);
        }
        else if (src.type == DrumType::Hat)
        {
            w = RenderHatSample(src, v, sampleRate);
        }

        v.drumTime += dt;
        return w;
    }
}

double RenderVoices(RenderState& state, const SoundData& sound)
{
    double sum = 0.0;
    int activeVoices = 0;
    for (auto& v : state.voices)
    {
        if (v.env.stage == ADSRStage::Off)
        {
            continue;
        }
        activeVoices++;

        double dt = 1.0 / sound.fs;
        double envGain = StepADSR(v.env, dt, v.attackSec, v.decaySec, v.sustainLevel, v.releaseSec);
        if (!v.pendingRemove && v.env.stage == ADSRStage::Off)
        {
            v.pendingRemove = true;
            state.pendingRemoveCount++;
            continue;
        }
        double w = 0.0;
        double velGain = VelocityToGain(v.velocity);
        int ch = v.channelIndex;
        if (state.channelMute[ch])
        {
            continue;
        }
        if (state.hasAnySolo && !state.channelSolo[ch])
        {
            continue;
        }
        double mixGain = state.channelMixGain[ch];
        if (mixGain <= 0.0)
        {
            continue;
        }
        double pitchFactor = state.channelPitch[ch];

        std::visit([&](const auto& src)
        {
            using T = std::decay_t<decltype(src)>;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                w = SampleWavePhase(src.wave, v.phase);
                sum += mixGain * v.amp * state.channelCc7[ch] * state.channelCc11[ch] * velGain * w * envGain;

                v.phase += v.phaseInc * pitchFactor;
                if (v.phase >= 1.0) v.phase -= 1.0;
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                w = SampleNoise(src.noise);
                sum += mixGain * v.amp * state.channelCc7[ch] * state.channelCc11[ch] * velGain * w * envGain;
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                w = SampleFmPhase(src.carrierWave, src.modWave, v.fmCarrierPhase, v.fmModPhase, src.index);
                sum += mixGain * v.amp * src.outLevel * state.channelCc7[ch] * state.channelCc11[ch] * velGain * w * envGain;

                v.fmCarrierPhase += v.phaseInc * pitchFactor * src.carrierRatio;
                if (v.fmCarrierPhase >= 1.0) v.fmCarrierPhase -= 1.0;
                v.fmModPhase += v.phaseInc * pitchFactor * src.modRatio;
                if (v.fmModPhase >= 1.0) v.fmModPhase -= 1.0;
            }
            else if constexpr (std::is_same_v<T, DrumConfig>)
            {
                double drumGain = (src.gain > 0.0) ? src.gain : 1.0;
                w = RenderDrumSample(src, v, dt, sound.fs);
                sum += mixGain * drumGain * v.amp * state.channelCc7[ch] * state.channelCc11[ch] * velGain * w * envGain;
            }
        }, v.source);
    }

    return sum;
}
