double ArpSemitoneRatio(int semitoneOffset)
{
    static const std::array<double, 49> kRatioLut = []()
    {
        std::array<double, 49> t{};
        for (int s = -24; s <= 24; s++)
        {
            t[static_cast<size_t>(s + 24)] = std::exp2(static_cast<double>(s) / 12.0);
        }
        return t;
    }();
    const int clamped = std::clamp(semitoneOffset, -24, 24);
    return kRatioLut[static_cast<size_t>(clamped + 24)];
}

template <typename SourceT, typename StateT, bool EnableDrift>
void RenderWaveformLikeSourceCommon(
    const SourceT& src,
    StateT& state,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    ModulationResult mod = EvaluateModulationSplitRate(
        state.modulation,
        src.modulation,
        in.dt,
        ModulationInput{ in.expressionVelocity, in.modwheel, in.channelPressure, in.polyPressure },
        4);
    double pitchMul = mod.pitchMul;

    if constexpr (EnableDrift)
    {
        if (src.driftDepthCents > 0.0)
        {
            const double driftSine = std::sin((state.driftPhase + state.driftPhaseOffset) * 2.0 * kPi);
            const double driftCents = driftSine * src.driftDepthCents;
            pitchMul *= std::exp2(driftCents / 1200.0);
            state.driftPhase += src.driftRateHz * in.dt;
            if (state.driftPhase >= 1.0) state.driftPhase -= 1.0;
        }
    }

    if (src.smoothing.enabled && src.smoothing.pitchEnabled)
    {
        SetSmoothedTarget(state.pitchSmoothing, pitchMul);
        pitchMul = StepSmoothedParam(state.pitchSmoothing);
    }
    if (src.arpeggio.enabled && src.arpeggio.steps > 0)
    {
        state.arpElapsedSec += in.dt;
        const int steps = std::clamp(src.arpeggio.steps, 1, 8);
        const double stepDur = 1.0 / (std::max)(0.5, src.arpeggio.rateHz);
        while (state.arpElapsedSec >= stepDur)
        {
            state.arpElapsedSec -= stepDur;
            state.arpStep = (state.arpStep + 1) % steps;
        }
        const int semitoneOffset = src.arpeggio.semitones[static_cast<size_t>(state.arpStep)];
        pitchMul *= ArpSemitoneRatio(semitoneOffset);
    }

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    const int unisonVoices = std::clamp(src.unisonVoices, 1, 8);
    const double spread = std::clamp(src.unisonSpread, 0.0, 1.0);
    const double subOscLevel = std::clamp(src.subOscLevel, 0.0, 2.0);
    const double effectivePW = std::clamp(src.pulseWidth + mod.pulseWidthAdd, 0.05, 0.95);

    double unisonSum = 0.0;
    for (int uv = 0; uv < unisonVoices; uv++)
    {
        const double pos = (unisonVoices <= 1) ? 0.0 : (static_cast<double>(uv) / (unisonVoices - 1));
        const double centered = (pos * 2.0) - 1.0;
        const double ratio = state.unisonDetuneRatio[static_cast<size_t>(uv)];
        const double phaseOffset = centered * spread * 0.08;
        const double uvPhase = WrapPhase(voices.phase[i] * ratio + phaseOffset);
        const double uvInc = phaseInc * ratio;
        unisonSum += SampleWavePhase(src.wave, uvPhase, uvInc, effectivePW);
    }

    double mainWave = unisonSum / unisonVoices;
    if (subOscLevel > 0.0)
    {
        const double subPhase = WrapPhase(voices.phase[i] * 0.5);
        const double subWave = SampleWavePhase(src.wave, subPhase, phaseInc * 0.5, effectivePW);
        mainWave = (mainWave + (subOscLevel * subWave)) / (1.0 + subOscLevel);
    }
    if (src.ringModEnabled && src.ringModMix > 0.0)
    {
        const double ringInc = phaseInc * std::clamp(src.ringModRatio, 0.125, 16.0);
        state.ringPhase = WrapPhase(state.ringPhase + ringInc);
        const double ringSignal = std::sin(2.0 * kPi * state.ringPhase);
        const double mix = std::clamp(src.ringModMix, 0.0, 1.0);
        mainWave *= (1.0 - mix) + mix * ringSignal;
    }

    frame.sample = mainWave;
    frame.ampMul = mod.ampMul;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    double effectiveCutoff = src.filterCutoffHz * mod.filterCutoffMul;
    if (src.filterKeytrack != 0.0)
    {
        effectiveCutoff *= state.filterKeytrackRatio;
    }
    frame.shaperCutoffHz = effectiveCutoff;
    frame.shaperResonanceMul = mod.resonanceMul;
    frame.shaperDrive = src.drive;
    frame.shaperDriveNorm = state.driveNorm;

    if (src.hardSyncEnabled)
    {
        const double syncInc = phaseInc * std::clamp(src.hardSyncRatio, 0.5, 8.0);
        const double prevSync = state.syncPhase;
        state.syncPhase = WrapPhase(state.syncPhase + syncInc);
        if (state.syncPhase < prevSync)
        {
            voices.phase[i] = 0.0;
        }
        else
        {
            voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
        }
    }
    else
    {
        voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
    }
}

void RenderWaveformSource(
    const WaveformConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& ws = std::get<WaveformVoiceState>(voices.sourceState[i]);
    RenderWaveformLikeSourceCommon<WaveformConfig, WaveformVoiceState, false>(src, ws, voices, i, in, frame);
}

void RenderAnalogSource(
    const AnalogConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& as = std::get<AnalogVoiceState>(voices.sourceState[i]);
    RenderWaveformLikeSourceCommon<AnalogConfig, AnalogVoiceState, true>(src, as, voices, i, in, frame);
}
