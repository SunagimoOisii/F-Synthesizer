struct VoiceRenderInput
{
    double dt = 0.0;
    double mixGainL = 1.0;
    double mixGainR = 1.0;
    double pitchFactor = 1.0;
    double ccGain = 1.0;
    double velGain = 1.0;
    double envGain = 1.0;
    double modwheel = 0.0;
    double channelPressure = 0.0;
    double polyPressure = 0.0;
    double brightness = 0.5;
    double resonance = 0.5;
};

double TimeScaleFromOffset(double offset)
{
    return std::exp2(std::clamp(offset, -1.0, 1.0) * 2.0);
}

double CutoffScaleFromBrightness(double brightness)
{
    return std::exp2((std::clamp(brightness, 0.0, 1.0) - 0.5) * 4.0);
}

double ResonanceScaleFromCc(double resonance)
{
    return std::exp2((std::clamp(resonance, 0.0, 1.0) - 0.5) * 2.0);
}

double SourceFilterResonance(const SourceConfig& src)
{
    return std::visit([](const auto& sourceCfg) -> double
    {
        using T = std::decay_t<decltype(sourceCfg)>;
        if constexpr (
            std::is_same_v<T, WaveformConfig> ||
            std::is_same_v<T, FmConfig> ||
            std::is_same_v<T, AnalogConfig> ||
            std::is_same_v<T, NoiseConfig>)
        {
            return sourceCfg.filterResonance;
        }
        return 0.707;
    }, src);
}

double WrapPhase(double phase)
{
    phase -= std::floor(phase);
    if (phase < 0.0)
    {
        phase += 1.0;
    }
    return phase;
}

double AttackSoftClip(double x, double drive)
{
    const double amount = std::clamp(drive, 0.0, 1.0);
    if (amount <= 0.0)
    {
        return std::clamp(x, -1.0, 1.0);
    }
    const double k = 1.0 + amount * 12.0;
    const double norm = std::tanh(k);
    return (norm > 1e-9) ? (std::tanh(x * k) / norm) : x;
}

double NextAttackNoise(uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    const double unit = static_cast<double>((state >> 8) & 0x00FFFFFFu) / 8388607.5;
    return unit - 1.0;
}

double RenderAttackLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const AttackLayerConfig& layer = voices.attackLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        voices.ageSec[i] += in.dt;
        return 0.0;
    }

    const double decay = std::clamp(layer.decaySec, 0.001, 0.25);
    const double age = voices.ageSec[i];
    voices.ageSec[i] += in.dt;
    if (age > decay * 6.0)
    {
        return 0.0;
    }

    const double bright = std::clamp(layer.brightness, 0.0, 1.0);
    const double bodyMix = std::clamp(layer.bodyMix, 0.0, 1.0);
    const double envFast = std::exp(-age / (decay * (0.18 + 0.22 * (1.0 - bright))));
    const double envBody = std::exp(-age / decay);
    const double pitchMul = std::exp2(std::clamp(layer.pitchOffsetSemis, -24.0, 24.0) / 12.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;

    double sample = 0.0;
    switch (layer.type)
    {
    case AttackLayerType::Brass:
    {
        const double bend = 1.0 + (0.08 + 0.18 * bright) * std::exp(-age / (decay * 0.55));
        voices.attackPhase[i] = WrapPhase(voices.attackPhase[i] + baseInc * bend * 1.5);
        const double tone = std::sin(2.0 * kPi * voices.attackPhase[i]);
        const double breath = NextAttackNoise(voices.attackNoiseState[i]) * envFast * (0.12 + 0.22 * bright);
        sample = (tone * envBody * bodyMix) + breath;
        break;
    }
    case AttackLayerType::Metal:
    {
        voices.attackPhase[i] = WrapPhase(voices.attackPhase[i] + baseInc * (2.0 + 3.5 * bright));
        const double p = voices.attackPhase[i];
        const double partials =
            std::sin(2.0 * kPi * p * 1.00) * 0.48 +
            std::sin(2.0 * kPi * p * 1.73) * 0.32 +
            std::sin(2.0 * kPi * p * 2.41) * 0.20;
        const double grain = NextAttackNoise(voices.attackNoiseState[i]) * envFast * (0.05 + 0.15 * bright);
        sample = (partials * envBody * (0.35 + bodyMix * 0.65)) + grain;
        break;
    }
    case AttackLayerType::Pick:
    default:
    {
        voices.attackPhase[i] = WrapPhase(voices.attackPhase[i] + baseInc * (1.0 + bright * 2.0));
        const double tri = 4.0 * std::abs(voices.attackPhase[i] - 0.5) - 1.0;
        const double click = NextAttackNoise(voices.attackNoiseState[i]) * envFast * (0.25 + 0.35 * bright);
        sample = (tri * envBody * bodyMix) + click;
        break;
    }
    }

    return AttackSoftClip(sample * std::clamp(layer.level, 0.0, 1.0), layer.drive);
}

double RenderBassLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const BassLayerConfig& layer = voices.bassLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return 0.0;
    }

    const double level = std::clamp(layer.level, 0.0, 1.0);
    const double pitchMul = std::exp2(std::clamp(layer.pitchOffsetSemis, -24.0, 24.0) / 12.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    voices.bassPhase[i] = WrapPhase(voices.bassPhase[i] + baseInc);

    const double p = voices.bassPhase[i];
    const double sine = std::sin(2.0 * kPi * p);
    const double tri = 4.0 * std::abs(p - 0.5) - 1.0;
    const double square = (p < 0.5) ? 1.0 : -1.0;
    const double folded = std::sin(2.0 * kPi * p * 2.0) * 0.55 + square * 0.45;

    double subMul = std::clamp(layer.subLevel, 0.0, 1.0);
    double bodyMul = std::clamp(layer.bodyLevel, 0.0, 1.0);
    double gritMul = std::clamp(layer.gritLevel, 0.0, 1.0);
    switch (layer.type)
    {
    case BassLayerType::Sub:
        gritMul *= 0.35;
        bodyMul *= 0.75;
        break;
    case BassLayerType::Grit:
        gritMul *= 1.35;
        bodyMul *= 1.10;
        break;
    case BassLayerType::Drive:
    default:
        break;
    }

    double sample = sine * subMul + tri * bodyMul + folded * gritMul;
    const double velNorm = std::clamp(in.velGain, 0.0, 1.0);
    const double drive = std::clamp(layer.drive + layer.velocityToDrive * velNorm, 0.0, 1.0);
    sample = AttackSoftClip(sample * level, drive);

    const double cutoff = std::clamp(layer.cutoffHz, 40.0, 8000.0);
    const double rc = 1.0 / (2.0 * kPi * cutoff);
    const double alpha = in.dt / (rc + in.dt);
    voices.bassLpState[i] += alpha * (sample - voices.bassLpState[i]);
    return voices.bassLpState[i];
}
