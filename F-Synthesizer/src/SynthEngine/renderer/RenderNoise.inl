void RenderNoiseSource(const NoiseConfig& src, SourceRenderFrame& frame)
{
    frame.sample = SampleNoise(src.noise);
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = src.filterCutoffHz;
    frame.shaperFilterDrive = src.filterDrive;
}

void RenderPsgSource(
    const PsgConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& ps = std::get<PsgVoiceState>(voices.sourceState[i]);

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor;
    ps.phase += phaseInc;
    if (ps.phase >= 1.0) ps.phase -= 1.0;

    double s = 0.0;
    switch (src.wave)
    {
    case PsgWaveType::Square:
        s = SampleWavePhase(WaveType::Square, ps.phase, phaseInc, 0.5);
        break;
    case PsgWaveType::Pulse:
    {
        const double pulseWidth = std::clamp((src.duty + 1) / 8.0, 0.05, 0.95);
        s = SampleWavePhase(WaveType::Square, ps.phase, phaseInc, pulseWidth);
        break;
    }
    case PsgWaveType::Triangle:
        s = SampleWavePhase(WaveType::Triangle, ps.phase, phaseInc);
        break;
    case PsgWaveType::Noise:
    {
        const uint16_t bit = ((ps.lfsrState >> 0) ^ (ps.lfsrState >> 1)) & 1u;
        ps.lfsrState = static_cast<uint16_t>((ps.lfsrState >> 1) | (bit << 15));
        s = (ps.lfsrState & 1u) ? 1.0 : -1.0;
        break;
    }
    }

    if (src.volumeSteps < 15)
    {
        const int steps = src.volumeSteps;
        s = (steps <= 0) ? 0.0 : std::round(s * steps) / steps;
    }

    frame.sample = s;
    frame.sourceGain = 1.0;
    frame.shaperKind = CommonShaperKind::None;
}
