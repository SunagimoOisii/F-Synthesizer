struct VoiceRenderInput
{
    double dt = 0.0;
    double mixGainL = 1.0;
    double mixGainR = 1.0;
    double pitchFactor = 1.0;
    double ccGain = 1.0;
    double velGain = 1.0;
    double velocityNorm = 1.0;
    double expressionVelocity = 1.0;
    double expressionFmIndexMul = 1.0;
    double expressionAttackMul = 1.0;
    double expressionBassMul = 1.0;
    double expressionLeadMul = 1.0;
    double expressionChordMul = 1.0;
    double expressionPadMul = 1.0;
    double expressionPluckMul = 1.0;
    double expressionStringMul = 1.0;
    double expressionBodyMul = 1.0;
    double expressionPadBrightnessAdd = 0.0;
    double expressionStringBrightnessAdd = 0.0;
    double expressionDriveAdd = 0.0;
    double expressionFilterDriveAdd = 0.0;
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

    const double level = std::clamp(layer.level, 0.0, 1.0) * in.expressionAttackMul;
    return AttackSoftClip(sample * level, std::clamp(layer.drive + in.expressionDriveAdd * 0.35, 0.0, 1.0));
}

double RenderBassLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const BassLayerConfig& layer = voices.bassLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return 0.0;
    }

    const double level = std::clamp(layer.level, 0.0, 1.0) * in.expressionBassMul;
    const double pitchMul = std::exp2(std::clamp(layer.pitchOffsetSemis, -24.0, 24.0) / 12.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    voices.bassPhase[i] = WrapPhase(voices.bassPhase[i] + baseInc);

    const double p = voices.bassPhase[i];
    const double sine = std::sin(2.0 * kPi * p);
    const double tri = 4.0 * std::abs(p - 0.5) - 1.0;
    const double square = (p < 0.5) ? 1.0 : -1.0;
    const double gritTone = std::clamp(layer.gritTone, 0.0, 1.0);
    const double folded =
        std::sin(2.0 * kPi * p * (1.5 + gritTone * 2.5)) * (0.70 - gritTone * 0.25) +
        square * (0.30 + gritTone * 0.25);

    double subMul = std::clamp(layer.subLevel, 0.0, 1.0);
    double bodyMul = std::clamp(layer.bodyLevel, 0.0, 1.0);
    double gritMul = std::clamp(layer.gritLevel, 0.0, 1.0) * (1.0 + (in.expressionBassMul - 1.0) * 0.45);
    double focusMul = std::clamp(layer.focusLevel, 0.0, 1.0);
    switch (layer.type)
    {
    case BassLayerType::Sub:
        gritMul *= 0.35;
        bodyMul *= 0.75;
        focusMul *= 0.65;
        break;
    case BassLayerType::Grit:
        gritMul *= 1.35;
        bodyMul *= 1.10;
        focusMul *= 1.10;
        break;
    case BassLayerType::Drive:
    default:
        break;
    }

    const double bodySat = std::clamp(layer.bodySaturation, 0.0, 1.0);
    const double body = AttackSoftClip((sine * subMul) + (tri * bodyMul), bodySat * 0.65);

    const double focusHz = std::clamp(layer.focusHz, 60.0, 1200.0);
    voices.bassFocusPhase[i] = WrapPhase(voices.bassFocusPhase[i] + focusHz * in.dt);
    const double focusTone =
        std::sin(2.0 * kPi * voices.bassFocusPhase[i]) * 0.72 +
        std::sin(4.0 * kPi * voices.bassFocusPhase[i]) * 0.28;

    const double attackDecay = std::clamp(layer.attackDecaySec, 0.005, 0.25);
    const double attackMul = 1.0 + std::clamp(layer.attackBoost, 0.0, 1.0) * std::exp(-voices.ageSec[i] / attackDecay);

    double sample = (body + (folded * gritMul) + (focusTone * focusMul)) * attackMul;
    const double velNorm = std::clamp(in.expressionVelocity, 0.0, 1.0);
    const double drive = std::clamp(layer.drive + layer.velocityToDrive * velNorm + in.expressionDriveAdd, 0.0, 1.0);
    sample = AttackSoftClip(sample * level, drive);

    const double cutoff = std::clamp(layer.cutoffHz, 40.0, 8000.0);
    const double rc = 1.0 / (2.0 * kPi * cutoff);
    const double alpha = in.dt / (rc + in.dt);
    voices.bassLpState[i] += alpha * (sample - voices.bassLpState[i]);
    return voices.bassLpState[i];
}

double RenderLeadLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const LeadLayerConfig& layer = voices.leadLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return 0.0;
    }

    const double level = std::clamp(layer.level, 0.0, 1.0) * in.expressionLeadMul;
    const double bendDecay = std::clamp(layer.bendDecaySec, 0.005, 0.25);
    const double wobbleDepth = std::clamp(layer.wobbleDepthCents, 0.0, 30.0);
    const double wobbleRate = std::clamp(layer.wobbleRateHz, 0.0, 12.0);
    const double wobbleSemis = (wobbleDepth / 100.0) * std::sin(2.0 * kPi * wobbleRate * voices.ageSec[i]);
    const double bendSemis = std::clamp(layer.pitchBendSemis, -12.0, 12.0) * std::exp(-voices.ageSec[i] / bendDecay) + wobbleSemis;
    const double bendMul = std::exp2(bendSemis / 12.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor * bendMul;
    const double detuneMul = std::exp2(std::clamp(layer.detuneCents, -50.0, 50.0) / 1200.0);
    voices.leadPhase[i] = WrapPhase(voices.leadPhase[i] + baseInc);
    voices.leadDetunePhase[i] = WrapPhase(voices.leadDetunePhase[i] + baseInc * detuneMul);

    const double p = voices.leadPhase[i];
    const double pd = voices.leadDetunePhase[i];
    const double body =
        std::sin(2.0 * kPi * p) * 0.62 +
        std::sin(2.0 * kPi * pd) * 0.38;
    double edge =
        std::sin(2.0 * kPi * p * 2.01) * 0.42 +
        std::sin(2.0 * kPi * p * 3.73) * 0.34 +
        ((p < 0.5) ? 1.0 : -1.0) * 0.24;
    const double characterTone = std::clamp(layer.characterTone, 0.0, 1.0);
    const double character =
        std::sin(2.0 * kPi * p * (2.71 + characterTone * 0.19)) * (0.58 - characterTone * 0.16) +
        std::sin(2.0 * kPi * p * (4.13 + characterTone * 0.37)) * (0.24 + characterTone * 0.18) +
        std::sin(2.0 * kPi * p * (5.89 + characterTone * 0.53)) * (0.10 + characterTone * 0.16);

    double bodyMul = std::clamp(layer.bodyLevel, 0.0, 1.0);
    double edgeMul = std::clamp(layer.edgeLevel, 0.0, 1.0);
    double characterMul = std::clamp(layer.characterLevel, 0.0, 1.0) * (1.0 + (in.expressionLeadMul - 1.0) * 0.45);
    switch (layer.type)
    {
    case LeadLayerType::Brass:
        bodyMul *= 1.15;
        edgeMul *= 0.75;
        characterMul *= 0.75;
        break;
    case LeadLayerType::Edge:
        bodyMul *= 0.55;
        edgeMul *= 1.30;
        characterMul *= 1.25;
        break;
    case LeadLayerType::Blade:
    default:
        characterMul *= 1.10;
        break;
    }

    const double attackDecay = std::clamp(layer.attackDecaySec, 0.005, 0.25);
    const double attack = 1.0 + std::clamp(layer.attackBoost, 0.0, 1.0) * std::exp(-voices.ageSec[i] / attackDecay);
    const double biteDecay = std::clamp(layer.biteDecaySec, 0.005, 0.25);
    const double biteEnv = std::exp(-voices.ageSec[i] / biteDecay);
    const double bite =
        (std::sin(2.0 * kPi * p * 6.27) * 0.5 + ((p < 0.5) ? 0.5 : -0.5)) *
        std::clamp(layer.biteLevel, 0.0, 1.0) * biteEnv;
    const double sample = (body * bodyMul + edge * edgeMul + character * characterMul + bite) * attack * level;
    return AttackSoftClip(sample, std::clamp(layer.drive + in.expressionDriveAdd * 0.45, 0.0, 1.0));
}

StereoFrame RenderChordLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const ChordLayerConfig& layer = voices.chordLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }

    const double level = std::clamp(layer.level, 0.0, 1.0) * in.expressionChordMul;
    const double detuneCents = std::clamp(layer.detuneCents, 0.0, 50.0);
    const double spread = std::clamp(layer.spread, 0.0, 1.0);
    double sample = 0.0;
    double left = 0.0;
    double right = 0.0;
    double weight = 0.0;
    for (size_t v = 0; v < layer.intervalsSemis.size(); v++)
    {
        const double voiceLevel = std::clamp(layer.voiceLevels[v], 0.0, 1.0);
        if (voiceLevel <= 0.0)
        {
            continue;
        }

        const double centered = (static_cast<double>(v) - 1.5) / 1.5;
        const double semis =
            static_cast<double>(std::clamp(layer.intervalsSemis[v], -24, 24)) +
            centered * detuneCents * spread / 100.0;
        const double inc = voices.phaseInc[i] * in.pitchFactor * std::exp2(semis / 12.0);
        voices.chordPhase[i][v] = WrapPhase(voices.chordPhase[i][v] + inc);
        const double p = voices.chordPhase[i][v];
        const double tri = 4.0 * std::abs(p - 0.5) - 1.0;
        const double saw = (2.0 * p) - 1.0;
        const double tone = std::sin(2.0 * kPi * p) * 0.55 + tri * 0.30 + saw * 0.15;
        const double pan = centered * spread * 0.45;
        sample += tone * voiceLevel;
        left += tone * voiceLevel * (1.0 - pan);
        right += tone * voiceLevel * (1.0 + pan);
        weight += voiceLevel;
    }
    if (weight <= 0.0)
    {
        return {};
    }

    sample /= weight;
    left /= weight;
    right /= weight;
    sample = AttackSoftClip(sample * level, std::clamp(layer.drive + in.expressionDriveAdd * 0.25, 0.0, 1.0));
    left = AttackSoftClip(left * level, std::clamp(layer.drive + in.expressionDriveAdd * 0.25, 0.0, 1.0));
    right = AttackSoftClip(right * level, std::clamp(layer.drive + in.expressionDriveAdd * 0.25, 0.0, 1.0));

    const double cutoff = std::clamp(layer.cutoffHz, 80.0, 10000.0);
    const double rc = 1.0 / (2.0 * kPi * cutoff);
    const double alpha = in.dt / (rc + in.dt);
    voices.chordLpState[i] += alpha * (sample - voices.chordLpState[i]);
    const double monoLp = voices.chordLpState[i];
    return StereoFrame{
        monoLp + (left - sample) * 0.45,
        monoLp + (right - sample) * 0.45
    };
}

StereoFrame RenderPadLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const PadLayerConfig& layer = voices.padLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }

    voices.padMotionPhase[i] = WrapPhase(voices.padMotionPhase[i] + std::clamp(layer.motionRateHz, 0.0, 8.0) * in.dt);
    const double motion = std::sin(2.0 * kPi * voices.padMotionPhase[i]);
    const double fadeIn = std::clamp(layer.fadeInSec, 0.005, 5.0);
    const double fade = 1.0 - std::exp(-voices.ageSec[i] / fadeIn);
    const double brightness = std::clamp(layer.brightness + in.expressionPadBrightnessAdd, 0.0, 1.0);
    const double detuneCents =
        std::clamp(layer.detuneCents, 0.0, 80.0) *
        (1.0 + std::clamp(layer.motionDepth, 0.0, 1.0) * 0.25 * motion);
    const double spread = std::clamp(layer.spread, 0.0, 1.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor;
    const double detuneMul = std::exp2(detuneCents / 1200.0);
    voices.padPhase[i] = WrapPhase(voices.padPhase[i] + baseInc / detuneMul);
    voices.padDetunePhase[i] = WrapPhase(voices.padDetunePhase[i] + baseInc * detuneMul);

    const double p0 = voices.padPhase[i];
    const double p1 = voices.padDetunePhase[i];
    const double saw0 = (2.0 * p0) - 1.0;
    const double saw1 = (2.0 * p1) - 1.0;
    const double tri0 = 4.0 * std::abs(p0 - 0.5) - 1.0;
    const double tri1 = 4.0 * std::abs(p1 - 0.5) - 1.0;
    const double baseTone =
        ((saw0 * (0.25 + brightness * 0.25)) + (tri0 * (0.50 - brightness * 0.15))) * (1.0 - spread * 0.35) +
        ((saw1 * (0.25 + brightness * 0.25)) + (tri1 * (0.50 - brightness * 0.15))) * (0.45 + spread * 0.35);
    const double leftTone =
        ((saw0 * (0.25 + brightness * 0.25)) + (tri0 * (0.50 - brightness * 0.15))) * (1.0 - spread * 0.15) +
        ((saw1 * (0.25 + brightness * 0.25)) + (tri1 * (0.50 - brightness * 0.15))) * (0.35 + spread * 0.10);
    const double rightTone =
        ((saw0 * (0.25 + brightness * 0.25)) + (tri0 * (0.50 - brightness * 0.15))) * (0.55 - spread * 0.10) +
        ((saw1 * (0.25 + brightness * 0.25)) + (tri1 * (0.50 - brightness * 0.15))) * (0.90 + spread * 0.25);

    const double octaveLevel = std::clamp(layer.octaveLevel, 0.0, 1.0);
    const double octaveTone = std::sin(4.0 * kPi * p0) * octaveLevel * (0.18 + brightness * 0.22);
    double sample = (baseTone + octaveTone) * fade * std::clamp(layer.level, 0.0, 1.0) * in.expressionPadMul;
    double left = (leftTone + octaveTone) * fade * std::clamp(layer.level, 0.0, 1.0) * in.expressionPadMul;
    double right = (rightTone + octaveTone) * fade * std::clamp(layer.level, 0.0, 1.0) * in.expressionPadMul;
    sample = AttackSoftClip(sample, std::clamp(layer.drive + in.expressionDriveAdd * 0.20, 0.0, 1.0));
    left = AttackSoftClip(left, std::clamp(layer.drive + in.expressionDriveAdd * 0.20, 0.0, 1.0));
    right = AttackSoftClip(right, std::clamp(layer.drive + in.expressionDriveAdd * 0.20, 0.0, 1.0));

    const double cutoff = std::clamp(layer.cutoffHz * (0.75 + brightness * 0.75), 80.0, 10000.0);
    const double rc = 1.0 / (2.0 * kPi * cutoff);
    const double alpha = in.dt / (rc + in.dt);
    voices.padLpState[i] += alpha * (sample - voices.padLpState[i]);
    const double monoLp = voices.padLpState[i];
    return StereoFrame{
        monoLp + (left - sample) * 0.55,
        monoLp + (right - sample) * 0.55
    };
}

StereoFrame RenderPluckLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const PluckLayerConfig& layer = voices.pluckLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }
    const double decay = std::clamp(layer.decaySec, 0.02, 2.0);
    const double age = voices.ageSec[i];
    const double env = std::exp(-age / decay);
    if (env < 0.0002)
    {
        return {};
    }
    const double bright = std::clamp(layer.brightness + in.brightness * 0.2, 0.0, 1.0);
    const double pitchMul = std::exp2(std::clamp(layer.pitchOffsetSemis, -24.0, 24.0) / 12.0);
    const double inc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    voices.pluckPhase[i] = WrapPhase(voices.pluckPhase[i] + inc);
    const double p = voices.pluckPhase[i];
    const double tri = 4.0 * std::abs(p - 0.5) - 1.0;
    const double saw = 2.0 * p - 1.0;
    const double pulse = (p < (0.38 + bright * 0.18)) ? 1.0 : -1.0;
    const double noise = NextAttackNoise(voices.attackNoiseState[i]);
    const double clickEnv = std::exp(-age / (0.006 + bright * 0.018));
    double sample =
        tri * (0.45 - bright * 0.18) +
        saw * (0.25 + bright * 0.18) +
        pulse * (0.12 + bright * 0.12) +
        noise * std::clamp(layer.noiseMix, 0.0, 1.0) * clickEnv;
    sample *= env * std::clamp(layer.level, 0.0, 1.0) * in.expressionPluckMul;
    sample = AttackSoftClip(sample, std::clamp(layer.drive + in.expressionDriveAdd * 0.35, 0.0, 1.0));

    const double cutoff = std::clamp(700.0 + bright * 7200.0, 80.0, 12000.0);
    const double rc = 1.0 / (2.0 * kPi * cutoff);
    const double alpha = in.dt / (rc + in.dt);
    voices.pluckLpState[i] += alpha * (sample - voices.pluckLpState[i]);
    const double bodySend = std::clamp(layer.bodySend, 0.0, 1.0);
    return StereoFrame{
        voices.pluckLpState[i] * (1.0 + bodySend * 0.05),
        voices.pluckLpState[i] * (1.0 - bodySend * 0.05)
    };
}

StereoFrame RenderStringLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const StringLayerConfig& layer = voices.stringLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }
    voices.stringMotionPhase[i] = WrapPhase(voices.stringMotionPhase[i] + std::clamp(layer.motionRateHz, 0.0, 12.0) * in.dt);
    const double motion = std::sin(2.0 * kPi * voices.stringMotionPhase[i]);
    const double bright = std::clamp(layer.brightness + in.expressionStringBrightnessAdd, 0.0, 1.0);
    const double fade = 1.0 - std::exp(-voices.ageSec[i] / std::clamp(layer.fadeInSec, 0.005, 3.0));
    const double detune = std::clamp(layer.detuneCents, 0.0, 80.0) *
        (1.0 + std::clamp(layer.motionDepth, 0.0, 1.0) * 0.25 * motion);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor;
    voices.stringPhaseA[i] = WrapPhase(voices.stringPhaseA[i] + baseInc * std::exp2(-detune / 1200.0));
    voices.stringPhaseB[i] = WrapPhase(voices.stringPhaseB[i] + baseInc * std::exp2(detune / 1200.0));
    const double a = voices.stringPhaseA[i];
    const double b = voices.stringPhaseB[i];
    const double sawA = 2.0 * a - 1.0;
    const double sawB = 2.0 * b - 1.0;
    const double triA = 4.0 * std::abs(a - 0.5) - 1.0;
    const double triB = 4.0 * std::abs(b - 0.5) - 1.0;
    const double bowNoise = NextAttackNoise(voices.attackNoiseState[i]) * std::clamp(layer.bowLevel, 0.0, 1.0) * (0.15 + bright * 0.20);
    const double leftTone = sawA * (0.30 + bright * 0.20) + triB * (0.42 - bright * 0.10) + bowNoise;
    const double rightTone = sawB * (0.30 + bright * 0.20) + triA * (0.42 - bright * 0.10) - bowNoise * 0.45;
    const double spread = std::clamp(layer.spread, 0.0, 1.0);
    const double level = std::clamp(layer.level, 0.0, 1.0) * in.expressionStringMul * fade;
    const double drive = std::clamp(layer.drive + in.expressionDriveAdd * 0.25, 0.0, 1.0);
    const double mono = (leftTone + rightTone) * 0.5;
    return StereoFrame{
        AttackSoftClip((mono * (1.0 - spread) + leftTone * spread) * level, drive),
        AttackSoftClip((mono * (1.0 - spread) + rightTone * spread) * level, drive)
    };
}

void ApplyBodyLayer(Voice& voices, size_t i, const VoiceRenderInput& in, SourceRenderFrame& frame)
{
    const BodyLayerConfig& layer = voices.bodyLayer[i];
    if (!layer.enabled || layer.mix <= 0.0)
    {
        return;
    }
    const double mix = std::clamp(layer.mix, 0.0, 1.0) * in.expressionBodyMul;
    const double size = std::clamp(layer.size, 0.0, 1.0);
    const double tone = std::clamp(layer.tone, 0.0, 1.0);
    const double damping = std::clamp(layer.damping, 0.0, 1.0);
    const double stereo = std::clamp(layer.stereo, 0.0, 1.0);
    const std::array<double, 5> ratios{ 1.00, 1.48, 2.02, 2.71, 3.36 };
    const std::array<double, 5> weights{ 0.42, 0.28, 0.18, 0.09, 0.05 };
    const double baseHz = std::clamp(95.0 + size * 330.0 + tone * 140.0, 70.0, 780.0);
    double bodyL = 0.0;
    double bodyR = 0.0;
    for (size_t r = 0; r < ratios.size(); r++)
    {
        const double freqL = std::clamp(baseHz * ratios[r] * (1.0 - stereo * 0.006 * static_cast<double>(r + 1)), 40.0, 6000.0);
        const double freqR = std::clamp(baseHz * ratios[r] * (1.0 + stereo * 0.007 * static_cast<double>(r + 1)), 40.0, 6000.0);
        voices.bodyPhase[i][r] = WrapPhase(voices.bodyPhase[i][r] + freqL * in.dt);
        const double band = std::sin(2.0 * kPi * voices.bodyPhase[i][r]);
        const double decay = std::exp(-in.dt * (2.0 + damping * 28.0 + static_cast<double>(r) * 4.0));
        voices.bodyStateL[i][r] = voices.bodyStateL[i][r] * decay + frame.sample * weights[r] * (0.08 + tone * 0.09);
        voices.bodyStateR[i][r] = voices.bodyStateR[i][r] * decay + frame.sample * weights[r] * (0.08 + tone * 0.09);
        bodyL += voices.bodyStateL[i][r] * band;
        bodyR += voices.bodyStateR[i][r] * std::sin(2.0 * kPi * WrapPhase(voices.bodyPhase[i][r] * freqR / freqL));
    }
    bodyL = AttackSoftClip(bodyL, layer.drive);
    bodyR = AttackSoftClip(bodyR, layer.drive);
    frame.stereoOffsetL += bodyL * mix;
    frame.stereoOffsetR += bodyR * mix;
    frame.sample = frame.sample * (1.0 - mix * 0.10) + (bodyL + bodyR) * 0.5 * mix * 0.55;
}
