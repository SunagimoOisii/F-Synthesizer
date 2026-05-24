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
    double brightnessCutoffScale = 1.0;
    double resonanceScale = 1.0;
};

double TimeScaleFromOffset(double offset)
{
    return RenderTimeScaleFromOffset(offset);
}

double CutoffScaleFromBrightness(double brightness)
{
    return RenderCutoffScaleFromBrightness(brightness);
}

double ResonanceScaleFromCc(double resonance)
{
    return RenderResonanceScaleFromCc(resonance);
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

double StepLayerPhase(double& phase, double phaseInc)
{
    phase = WrapPhase(phase + phaseInc);
    return phase;
}

double LayerWave(WaveType type, double phase, double phaseInc, double pulseWidth = 0.5)
{
    return SampleWavePhase(type, phase, phaseInc, pulseWidth);
}

double OnePoleAlphaFromCutoff(double cutoffHz, double dt)
{
    const double rc = 1.0 / (2.0 * kPi * std::max(cutoffHz, 20.0));
    return dt / (rc + dt);
}

double RenderAttackLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const AttackLayerConfig& layer = voices.attackLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return 0.0;
    }

    const double decay = std::clamp(layer.decaySec, 0.001, 0.25);
    const double age = voices.ageSec[i];
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
        const double inc = baseInc * bend * 1.5;
        const double phase = StepLayerPhase(voices.attackPhase[i], inc);
        const double tone =
            LayerWave(WaveType::Sine, phase, inc) * 0.78 +
            LayerWave(WaveType::Triangle, phase, inc) * 0.22;
        const double breath = NextAttackNoise(voices.attackNoiseState[i]) * envFast * (0.035 + 0.075 * bright);
        sample = (tone * envBody * bodyMix) + breath;
        break;
    }
    case AttackLayerType::Metal:
    {
        const double inc = baseInc * (1.6 + 2.2 * bright);
        const double p = StepLayerPhase(voices.attackPhase[i], inc);
        const double partials =
            std::sin(2.0 * kPi * p * 1.00) * 0.48 +
            std::sin(2.0 * kPi * p * 1.50) * 0.27 +
            std::sin(2.0 * kPi * p * 2.00) * 0.16;
        const double grain = NextAttackNoise(voices.attackNoiseState[i]) * envFast * (0.025 + 0.065 * bright);
        sample = (partials * envBody * (0.35 + bodyMix * 0.65)) + grain;
        break;
    }
    case AttackLayerType::Pick:
    default:
    {
        const double inc = baseInc * (1.0 + bright * 1.6);
        const double phase = StepLayerPhase(voices.attackPhase[i], inc);
        const double tri = LayerWave(WaveType::Triangle, phase, inc);
        const double click = LayerWave(WaveType::Square, phase, inc, 0.42) * envFast * (0.06 + 0.10 * bright);
        const double texture = NextAttackNoise(voices.attackNoiseState[i]) * envFast * (0.025 + 0.045 * bright);
        sample = (tri * envBody * bodyMix) + click + texture;
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
    const double p = StepLayerPhase(voices.bassPhase[i], baseInc);
    const double sine = LayerWave(WaveType::Sine, p, baseInc);
    const double tri = LayerWave(WaveType::Triangle, p, baseInc);
    const double square = LayerWave(WaveType::Square, p, baseInc, 0.50);
    const double gritTone = std::clamp(layer.gritTone, 0.0, 1.0);
    const double foldedPhase = WrapPhase(p * (1.0 + gritTone * 1.5));
    const double foldedInc = baseInc * (1.0 + gritTone * 1.5);
    const double folded =
        LayerWave(WaveType::Triangle, foldedPhase, foldedInc) * (0.52 - gritTone * 0.18) +
        square * (0.22 + gritTone * 0.18);

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
    const double alpha = OnePoleAlphaFromCutoff(cutoff, in.dt);
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
    const double p = StepLayerPhase(voices.leadPhase[i], baseInc);
    const double pd = StepLayerPhase(voices.leadDetunePhase[i], baseInc * detuneMul);
    const double body =
        LayerWave(WaveType::Sine, p, baseInc) * 0.62 +
        LayerWave(WaveType::Sine, pd, baseInc * detuneMul) * 0.38;
    double edge =
        LayerWave(WaveType::Triangle, WrapPhase(p * 2.0), baseInc * 2.0) * 0.36 +
        LayerWave(WaveType::Saw, WrapPhase(p * 3.0), baseInc * 3.0) * 0.18 +
        LayerWave(WaveType::Square, p, baseInc, 0.48) * 0.16;
    const double characterTone = std::clamp(layer.characterTone, 0.0, 1.0);
    const double character =
        LayerWave(WaveType::Sine, WrapPhase(p * 2.0), baseInc * 2.0) * (0.50 - characterTone * 0.10) +
        LayerWave(WaveType::Triangle, WrapPhase(p * 3.0), baseInc * 3.0) * (0.20 + characterTone * 0.12) +
        LayerWave(WaveType::Sine, WrapPhase(p * 5.0), baseInc * 5.0) * (0.08 + characterTone * 0.08);

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
        (LayerWave(WaveType::Sine, WrapPhase(p * 6.0), baseInc * 6.0) * 0.45 +
            LayerWave(WaveType::Square, p, baseInc, 0.47) * 0.28) *
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
        const double p = StepLayerPhase(voices.chordPhase[i][v], inc);
        const double tri = LayerWave(WaveType::Triangle, p, inc);
        const double saw = LayerWave(WaveType::Saw, p, inc);
        const double tone = LayerWave(WaveType::Sine, p, inc) * 0.58 + tri * 0.30 + saw * 0.08;
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
    const double alpha = OnePoleAlphaFromCutoff(cutoff, in.dt);
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
    const double inc0 = baseInc / detuneMul;
    const double inc1 = baseInc * detuneMul;
    const double p0 = StepLayerPhase(voices.padPhase[i], inc0);
    const double p1 = StepLayerPhase(voices.padDetunePhase[i], inc1);
    const double saw0 = LayerWave(WaveType::Saw, p0, inc0);
    const double saw1 = LayerWave(WaveType::Saw, p1, inc1);
    const double tri0 = LayerWave(WaveType::Triangle, p0, inc0);
    const double tri1 = LayerWave(WaveType::Triangle, p1, inc1);
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
    const double octaveTone = LayerWave(WaveType::Sine, WrapPhase(p0 * 2.0), inc0 * 2.0) * octaveLevel * (0.18 + brightness * 0.22);
    double sample = (baseTone + octaveTone) * fade * std::clamp(layer.level, 0.0, 1.0) * in.expressionPadMul;
    double left = (leftTone + octaveTone) * fade * std::clamp(layer.level, 0.0, 1.0) * in.expressionPadMul;
    double right = (rightTone + octaveTone) * fade * std::clamp(layer.level, 0.0, 1.0) * in.expressionPadMul;
    sample = AttackSoftClip(sample, std::clamp(layer.drive + in.expressionDriveAdd * 0.20, 0.0, 1.0));
    left = AttackSoftClip(left, std::clamp(layer.drive + in.expressionDriveAdd * 0.20, 0.0, 1.0));
    right = AttackSoftClip(right, std::clamp(layer.drive + in.expressionDriveAdd * 0.20, 0.0, 1.0));

    const double cutoff = std::clamp(layer.cutoffHz * (0.75 + brightness * 0.75), 80.0, 10000.0);
    const double alpha = OnePoleAlphaFromCutoff(cutoff, in.dt);
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
    const double p = StepLayerPhase(voices.pluckPhase[i], inc);
    const double tri = LayerWave(WaveType::Triangle, p, inc);
    const double saw = LayerWave(WaveType::Saw, p, inc);
    const double pulse = LayerWave(WaveType::Square, p, inc, 0.38 + bright * 0.18);
    const double noise = NextAttackNoise(voices.attackNoiseState[i]);
    const double clickEnv = std::exp(-age / (0.006 + bright * 0.018));
    double sample =
        tri * (0.45 - bright * 0.18) +
        saw * (0.25 + bright * 0.18) +
        pulse * (0.08 + bright * 0.08) +
        noise * std::clamp(layer.noiseMix, 0.0, 1.0) * clickEnv * 0.55;
    sample *= env * std::clamp(layer.level, 0.0, 1.0) * in.expressionPluckMul;
    sample = AttackSoftClip(sample, std::clamp(layer.drive + in.expressionDriveAdd * 0.35, 0.0, 1.0));

    const double cutoff = std::clamp(700.0 + bright * 7200.0, 80.0, 12000.0);
    const double alpha = OnePoleAlphaFromCutoff(cutoff, in.dt);
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
    const double incA = baseInc * std::exp2(-detune / 1200.0);
    const double incB = baseInc * std::exp2(detune / 1200.0);
    const double a = StepLayerPhase(voices.stringPhaseA[i], incA);
    const double b = StepLayerPhase(voices.stringPhaseB[i], incB);
    const double sawA = LayerWave(WaveType::Saw, a, incA);
    const double sawB = LayerWave(WaveType::Saw, b, incB);
    const double triA = LayerWave(WaveType::Triangle, a, incA);
    const double triB = LayerWave(WaveType::Triangle, b, incB);
    const double bowNoise = NextAttackNoise(voices.attackNoiseState[i]) * std::clamp(layer.bowLevel, 0.0, 1.0) * (0.045 + bright * 0.085);
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

StereoFrame RenderHarmonicLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const HarmonicLayerConfig& layer = voices.harmonicLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }

    constexpr std::array<double, 8> ratios{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0 };
    const double level = std::clamp(layer.level, 0.0, 1.0);
    const double brightness = std::clamp(layer.brightness, 0.0, 1.0);
    const double attackSec = std::clamp(layer.attackSec, 0.001, 0.25);
    const double attack = 1.0 - std::exp(-voices.ageSec[i] / attackSec);
    const double releaseDamp = voices.env[i].stage == ADSRStage::Release
        ? (1.0 - std::clamp(layer.releaseDamp, 0.0, 1.0) * 0.65)
        : 1.0;
    const double stereo = std::clamp(layer.stereo, 0.0, 1.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor;
    double left = 0.0;
    double right = 0.0;
    double weight = 0.0;

    for (size_t h = 0; h < ratios.size(); h++)
    {
        const double rawLevel = std::clamp(layer.harmonicLevels[h], 0.0, 1.0);
        if (rawLevel <= 0.0)
        {
            continue;
        }

        const double harmonicIndex = static_cast<double>(h) / static_cast<double>(ratios.size() - 1);
        const double brightScale = 0.55 + brightness * (0.55 + harmonicIndex * 0.70);
        const double harmonicLevel = rawLevel * brightScale;
        const double inc = baseInc * ratios[h];
        const double phaseL = StepLayerPhase(voices.harmonicPhase[i][h], inc);
        const double phaseR = StepLayerPhase(voices.harmonicPhaseR[i][h], inc * (1.0 + stereo * 0.0008 * static_cast<double>(h + 1)));
        left += LayerWave(WaveType::Sine, phaseL, inc) * harmonicLevel;
        right += LayerWave(WaveType::Sine, phaseR, inc) * harmonicLevel;
        weight += harmonicLevel;
    }

    if (weight <= 0.0)
    {
        return {};
    }

    left /= weight;
    right /= weight;

    const double keyClick = std::clamp(layer.keyClick, 0.0, 1.0);
    if (keyClick > 0.0)
    {
        const double clickEnv = std::exp(-voices.ageSec[i] / 0.010);
        const double clickPhase = voices.harmonicPhase[i][2];
        const double click = (
            LayerWave(WaveType::Sine, clickPhase, baseInc * 3.0) * 0.65 +
            LayerWave(WaveType::Triangle, clickPhase, baseInc * 3.0) * 0.35) *
            keyClick * clickEnv;
        left += click;
        right += click * (1.0 - stereo * 0.12);
    }

    const double gain = level * attack * releaseDamp;
    const double drive = std::clamp(layer.drive + in.expressionDriveAdd * 0.25, 0.0, 1.0);
    return StereoFrame{
        AttackSoftClip(left * gain, drive),
        AttackSoftClip(right * gain, drive)
    };
}

StereoFrame RenderPowerChordLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const PowerChordLayerConfig& layer = voices.powerChordLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }

    constexpr std::array<double, 3> semis{ 0.0, 7.0, 12.0 };
    const std::array<double, 3> levels{
        1.0,
        std::clamp(layer.fifthLevel, 0.0, 1.0),
        std::clamp(layer.octaveLevel, 0.0, 1.0)
    };
    const double level = std::clamp(layer.level, 0.0, 1.0);
    const double spread = std::clamp(layer.spread, 0.0, 1.0);
    const double tone = std::clamp(layer.tone, 0.0, 1.0);
    const double detune = std::clamp(layer.detuneCents, 0.0, 18.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor;
    double left = 0.0;
    double right = 0.0;
    double weight = 0.0;

    for (size_t v = 0; v < semis.size(); v++)
    {
        if (levels[v] <= 0.0)
        {
            continue;
        }
        const double centered = (static_cast<double>(v) - 1.0);
        const double incL = baseInc * std::exp2((semis[v] - detune * spread * 0.01 * centered) / 12.0);
        const double incR = baseInc * std::exp2((semis[v] + detune * spread * 0.01 * centered) / 12.0);
        const double pL = StepLayerPhase(voices.powerChordPhase[i][v], incL);
        const double pR = StepLayerPhase(voices.powerChordPhaseR[i][v], incR);
        const double toneL =
            LayerWave(WaveType::Triangle, pL, incL) * (0.50 - tone * 0.12) +
            LayerWave(WaveType::Saw, pL, incL) * (0.14 + tone * 0.18) +
            LayerWave(WaveType::Sine, pL, incL) * 0.18;
        const double toneR =
            LayerWave(WaveType::Triangle, pR, incR) * (0.50 - tone * 0.12) +
            LayerWave(WaveType::Saw, pR, incR) * (0.14 + tone * 0.18) +
            LayerWave(WaveType::Sine, pR, incR) * 0.18;
        const double pan = centered * spread * 0.18;
        left += toneL * levels[v] * (1.0 - pan);
        right += toneR * levels[v] * (1.0 + pan);
        weight += levels[v];
    }

    if (weight <= 0.0)
    {
        return {};
    }

    const double drive = std::clamp(layer.drive + in.expressionDriveAdd * 0.25, 0.0, 1.0);
    return StereoFrame{
        AttackSoftClip((left / weight) * level, drive),
        AttackSoftClip((right / weight) * level, drive)
    };
}

StereoFrame RenderChugLayer(Voice& voices, size_t i, const VoiceRenderInput& in)
{
    const ChugLayerConfig& layer = voices.chugLayer[i];
    if (!layer.enabled || layer.level <= 0.0)
    {
        return {};
    }

    const double age = voices.ageSec[i];
    const double decay = std::clamp(layer.decaySec, 0.025, 0.75);
    const double env = std::exp(-age / decay);
    if (env < 0.0002)
    {
        return {};
    }

    const double tight = std::clamp(layer.tightness, 0.0, 1.0);
    const double tone = std::clamp(layer.tone, 0.0, 1.0);
    const double baseInc = voices.phaseInc[i] * in.pitchFactor;
    const double p = StepLayerPhase(voices.chugPhase[i], baseInc);
    const double bodyInc = baseInc * 0.5;
    const double bp = StepLayerPhase(voices.chugBodyPhase[i], bodyInc);
    const double pickEnv = std::exp(-age / (0.006 + (1.0 - tight) * 0.025));
    const double body =
        LayerWave(WaveType::Sine, bp, bodyInc) * std::clamp(layer.lowPunch, 0.0, 1.0) +
        LayerWave(WaveType::Triangle, p, baseInc) * (0.35 + tone * 0.20);
    const double pick =
        (LayerWave(WaveType::Triangle, WrapPhase(p * 3.0), baseInc * 3.0) * 0.55 +
            LayerWave(WaveType::Square, p, baseInc, 0.42) * 0.25) *
        std::clamp(layer.pick, 0.0, 1.0) * pickEnv;
    double sample = (body + pick) * env * std::clamp(layer.level, 0.0, 1.0);
    sample = AttackSoftClip(sample, std::clamp(layer.drive + in.expressionDriveAdd * 0.3, 0.0, 1.0));

    const double cutoff = std::clamp(450.0 + tone * 3600.0 - tight * 260.0, 120.0, 7200.0);
    const double alpha = OnePoleAlphaFromCutoff(cutoff, in.dt);
    voices.chugLpState[i] += alpha * (sample - voices.chugLpState[i]);
    return StereoFrame{ voices.chugLpState[i], voices.chugLpState[i] };
}

void ApplyAmpCabLayer(Voice& voices, size_t i, const VoiceRenderInput& in, SourceRenderFrame& frame)
{
    const AmpCabLayerConfig& layer = voices.ampCabLayer[i];
    if (!layer.enabled)
    {
        return;
    }

    const double tone = std::clamp(layer.tone, 0.0, 1.0);
    const double drive = std::clamp(layer.drive + in.expressionDriveAdd * 0.35, 0.0, 1.0);
    const double output = std::clamp(layer.output, 0.0, 1.4);
    const double presence = std::clamp(layer.presence, 0.0, 1.0);
    const double lowCut = std::clamp(70.0 + std::clamp(layer.cabLow, 0.0, 1.0) * 260.0, 40.0, 520.0);
    const double highCut = std::clamp(1450.0 + std::clamp(layer.cabHigh, 0.0, 1.0) * 7600.0 + tone * 1200.0, 900.0, 11000.0);

    auto process = [&](double x, double& hpState, double& lpState) {
        const double hpAlpha = OnePoleAlphaFromCutoff(lowCut, in.dt);
        hpState += hpAlpha * (x - hpState);
        double y = x - hpState;
        y = AttackSoftClip(y * (1.0 + drive * 5.5), drive);
        const double lpAlpha = OnePoleAlphaFromCutoff(highCut, in.dt);
        lpState += lpAlpha * (y - lpState);
        const double low = lpState;
        const double edge = y - low;
        return (low + edge * presence * 0.55) * output;
    };

    const double left = process(frame.sample + frame.stereoOffsetL, voices.ampCabHpStateL[i], voices.ampCabLpStateL[i]);
    const double right = process(frame.sample + frame.stereoOffsetR, voices.ampCabHpStateR[i], voices.ampCabLpStateR[i]);
    frame.sample = (left + right) * 0.5;
    frame.stereoOffsetL = left - frame.sample;
    frame.stereoOffsetR = right - frame.sample;
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
    std::array<double, 5> ratios{ 1.00, 2.00, 3.00, 4.00, 5.00 };
    std::array<double, 5> weights{ 0.46, 0.26, 0.14, 0.08, 0.04 };
    switch (layer.mode)
    {
    case BodyLayerConfig::Mode::Metal:
        ratios = { 1.00, 1.41, 2.11, 2.86, 3.93 };
        weights = { 0.36, 0.27, 0.18, 0.11, 0.06 };
        break;
    case BodyLayerConfig::Mode::Box:
        ratios = { 1.00, 1.50, 2.00, 2.67, 3.33 };
        weights = { 0.42, 0.25, 0.16, 0.08, 0.04 };
        break;
    case BodyLayerConfig::Mode::Harmonic:
    default:
        break;
    }
    const double baseHz = std::clamp(95.0 + size * 330.0 + tone * 140.0, 70.0, 780.0);
    double bodyL = 0.0;
    double bodyR = 0.0;
    for (size_t r = 0; r < ratios.size(); r++)
    {
        const double freqL = std::clamp(baseHz * ratios[r] * (1.0 - stereo * 0.006 * static_cast<double>(r + 1)), 40.0, 6000.0);
        const double freqR = std::clamp(baseHz * ratios[r] * (1.0 + stereo * 0.007 * static_cast<double>(r + 1)), 40.0, 6000.0);
        voices.bodyPhase[i][r] = WrapPhase(voices.bodyPhase[i][r] + freqL * in.dt);
        const double band = LayerWave(WaveType::Sine, voices.bodyPhase[i][r], freqL * in.dt);
        const double decay = std::exp(-in.dt * (2.0 + damping * 28.0 + static_cast<double>(r) * 4.0));
        voices.bodyStateL[i][r] = voices.bodyStateL[i][r] * decay + frame.sample * weights[r] * (0.06 + tone * 0.075);
        voices.bodyStateR[i][r] = voices.bodyStateR[i][r] * decay + frame.sample * weights[r] * (0.06 + tone * 0.075);
        bodyL += voices.bodyStateL[i][r] * band;
        bodyR += voices.bodyStateR[i][r] * LayerWave(WaveType::Sine, WrapPhase(voices.bodyPhase[i][r] * freqR / freqL), freqR * in.dt);
    }
    bodyL = AttackSoftClip(bodyL, layer.drive);
    bodyR = AttackSoftClip(bodyR, layer.drive);
    frame.stereoOffsetL += bodyL * mix;
    frame.stereoOffsetR += bodyR * mix;
    frame.sample = frame.sample * (1.0 - mix * 0.10) + (bodyL + bodyR) * 0.5 * mix * 0.55;
}
