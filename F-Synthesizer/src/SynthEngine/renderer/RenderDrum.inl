void EnsureDrumFilters(DrumVoiceState& ds, double hpCut, double lpCut, int sampleRate)
{
    if (ds.hpAlpha <= 0.0)
    {
        ds.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
    }
    if (ds.lpAlpha <= 0.0)
    {
        ds.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
}

void PrepareDrumRelease(Voice& voices, const DrumVoiceState& ds, size_t i)
{
    if (voices.released[i] == 0 && ds.time >= (voices.attackSec[i] + voices.decaySec[i]))
    {
        NoteOff(voices.env[i]);
        voices.released[i] = 1;
    }
}

double DrumParam(double value, double fallback)
{
    return (value > 0.0) ? value : fallback;
}

double DrumVelocityShape(const DrumConfig& src, const DrumVoiceState& ds)
{
    const double vel = std::clamp(ds.velocityNorm, 0.0, 1.0);
    return 1.0 + (vel - 0.7) * std::clamp(src.velocityToTone, 0.0, 1.0);
}

double DrumDecay(const DrumConfig& src, const DrumVoiceState& ds, double fallback)
{
    const double vel = std::clamp(ds.velocityNorm, 0.0, 1.0);
    const double velScale = 1.0 + (vel - 0.7) * std::clamp(src.velocityToDecay, -1.0, 1.0);
    return std::max(0.001, DrumParam(src.decaySec, fallback) * ds.decayScale * velScale);
}

double DrumEnv(double time, double decaySec)
{
    return std::exp(-time / DrumParam(decaySec, 0.05));
}

double DrumSoftClip(double x, double drive)
{
    const double amount = 1.0 + std::clamp(drive, 0.0, 1.0) * 10.0;
    return std::tanh(x * amount) / std::tanh(amount);
}

NoiseType DrumNoiseColor(const DrumConfig& src)
{
    return static_cast<NoiseType>(std::clamp(src.noiseColor, 0, 3));
}

double NextDrumWhite(DrumVoiceState& ds)
{
    ds.noiseState = ds.noiseState * 1664525u + 1013904223u;
    const double unit = static_cast<double>((ds.noiseState >> 8) & 0x00FFFFFFu) / 8388607.5;
    return unit - 1.0;
}

double NextDrumNoise(DrumVoiceState& ds, NoiseType color)
{
    const double white = NextDrumWhite(ds);
    if (color == NoiseType::Pink)
    {
        ds.lpPrev = 0.82 * ds.lpPrev + 0.18 * white;
        return ds.lpPrev;
    }
    if (color == NoiseType::Brown)
    {
        ds.lpPrev = std::clamp(ds.lpPrev + white * 0.08, -1.0, 1.0);
        return ds.lpPrev;
    }
    if (color == NoiseType::Blue)
    {
        const double blue = white - ds.noisePrev;
        ds.noisePrev = white;
        return std::clamp(blue, -1.0, 1.0);
    }
    return white;
}

double FilterDrumNoise(DrumVoiceState& ds, double noise)
{
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;
    return lp;
}

double StepBodyPhase(Voice& voices, DrumVoiceState& ds, size_t i, double freq, int sampleRate)
{
    voices.phase[i] = WrapPhase(voices.phase[i] + (freq * ds.pitchRatio / sampleRate));
    return voices.phase[i];
}

double RenderTransient(const DrumConfig& src, DrumVoiceState& ds, int sampleRate, double freq, double fallbackLevel, double fallbackDecay)
{
    const double level = DrumParam(src.transientLevel, fallbackLevel);
    const double decay = DrumParam(src.transientDecaySec, fallbackDecay);
    ds.transientPhase = WrapPhase(ds.transientPhase + freq / sampleRate);
    const double tone = SampleWavePhase(WaveType::Square, ds.transientPhase);
    const double noise = NextDrumNoise(ds, NoiseType::Blue);
    return (0.7 * tone + 0.3 * noise) * level * DrumEnv(ds.time, decay);
}

double RenderKickSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double toneShape = DrumVelocityShape(src, ds);
    const double pitchStart = DrumParam(src.pitchStart, 4.2) * toneShape;
    const double pitchDecay = DrumParam(src.pitchDecaySec, 0.05);
    const double pitchFactor = 1.0 + (pitchStart - 1.0) * std::exp(-ds.time / pitchDecay);
    const double bodyFreq = ds.bodyFreq * pitchFactor;
    const double body = SampleWavePhase(WaveType::Sine, StepBodyPhase(voices, ds, i, bodyFreq, sampleRate))
        * DrumParam(src.bodyLevel, 0.9)
        * DrumEnv(ds.time, DrumParam(src.bodyDecaySec, 0.18) * ds.decayScale);
    const double transient = RenderTransient(src, ds, sampleRate, 2600.0 + 700.0 * toneShape, 0.28, 0.007);
    return DrumSoftClip(body + transient, src.drive);
}

double RenderSnareSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double toneShape = DrumVelocityShape(src, ds);
    EnsureDrumFilters(ds, DrumParam(src.hpCut, 950.0), DrumParam(src.lpCut, 5600.0) * toneShape, sampleRate);
    const double body = SampleWavePhase(WaveType::Square, StepBodyPhase(voices, ds, i, ds.bodyFreq * toneShape, sampleRate), ds.bodyFreq / sampleRate)
        * DrumParam(src.bodyLevel, 0.48)
        * DrumEnv(ds.time, DrumParam(src.bodyDecaySec, 0.09) * ds.decayScale);
    const double snapNoise = FilterDrumNoise(ds, NextDrumNoise(ds, DrumNoiseColor(src)))
        * DrumParam(src.snapLevel, 0.82)
        * DrumEnv(ds.time, DrumParam(src.snapDecaySec, 0.055) * ds.decayScale);
    const double transient = RenderTransient(src, ds, sampleRate, 3600.0, 0.15, 0.006);
    return DrumSoftClip(body + snapNoise + transient, src.drive);
}

double RenderHatLikeSample(const DrumConfig& src, DrumVoiceState& ds, int sampleRate, bool ride, bool crash)
{
    const double toneShape = DrumVelocityShape(src, ds);
    const double defaultDecay = crash ? 0.45 : (ride ? 0.22 : 0.045);
    const double decay = DrumDecay(src, ds, defaultDecay);
    EnsureDrumFilters(ds, DrumParam(src.hpCut, ride ? 3600.0 : 5200.0), DrumParam(src.lpCut, crash ? 13500.0 : 11000.0), sampleRate);
    const double noise = FilterDrumNoise(ds, NextDrumNoise(ds, DrumNoiseColor(src)))
        * DrumParam(src.airLevel, crash ? 0.52 : 0.32);

    const double base = ride ? 3200.0 : (crash ? 4700.0 : 5600.0);
    constexpr double ratios[4] = { 1.0, 1.31, 1.73, 2.07 };
    double metal = 0.0;
    for (size_t phaseIndex = 0; phaseIndex < ds.metalPhase.size(); phaseIndex++)
    {
        ds.metalPhase[phaseIndex] = WrapPhase(ds.metalPhase[phaseIndex] + base * ratios[phaseIndex] * toneShape / sampleRate);
        const WaveType wave = ride ? WaveType::Sine : WaveType::Square;
        metal += SampleWavePhase(wave, ds.metalPhase[phaseIndex]);
    }
    metal *= 0.25 * DrumParam(src.metalLevel, ride ? 0.7 : 0.55);
    const double ping = ride ? std::sin(2.0 * kPi * ds.metalPhase[0]) * DrumParam(src.transientLevel, 0.22) * DrumEnv(ds.time, 0.035) : 0.0;
    return DrumSoftClip((metal + noise) * DrumEnv(ds.time, decay) + ping, src.drive);
}

double RenderTomSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double toneShape = DrumVelocityShape(src, ds);
    const double pitchStart = DrumParam(src.pitchStart, 2.3) * toneShape;
    const double pitchDecay = DrumParam(src.pitchDecaySec, 0.08);
    const double pitchFactor = 1.0 + (pitchStart - 1.0) * std::exp(-ds.time / pitchDecay);
    const double phase = StepBodyPhase(voices, ds, i, ds.bodyFreq * pitchFactor, sampleRate);
    const double body = SampleWavePhase(WaveType::Sine, phase)
        * DrumParam(src.bodyLevel, 0.78)
        * DrumEnv(ds.time, DrumParam(src.bodyDecaySec, 0.20) * ds.decayScale);
    const double transient = RenderTransient(src, ds, sampleRate, 1800.0, 0.12, 0.009);
    return DrumSoftClip(body + transient, src.drive);
}

double RenderRimSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double toneShape = DrumVelocityShape(src, ds);
    EnsureDrumFilters(ds, DrumParam(src.hpCut, 1800.0), DrumParam(src.lpCut, 7600.0), sampleRate);
    const double phase = StepBodyPhase(voices, ds, i, ds.bodyFreq * toneShape, sampleRate);
    const double body = SampleWavePhase(WaveType::Square, phase)
        * DrumParam(src.bodyLevel, 0.5)
        * DrumEnv(ds.time, DrumParam(src.bodyDecaySec, 0.045) * ds.decayScale);
    const double stick = FilterDrumNoise(ds, NextDrumNoise(ds, NoiseType::Blue))
        * DrumParam(src.transientLevel, 0.36)
        * DrumEnv(ds.time, DrumParam(src.transientDecaySec, 0.008));
    return DrumSoftClip(body + stick, src.drive);
}

double RenderClapSample(const DrumConfig& src, DrumVoiceState& ds, int sampleRate)
{
    EnsureDrumFilters(ds, DrumParam(src.hpCut, 900.0), DrumParam(src.lpCut, 5200.0), sampleRate);
    double bursts = 0.0;
    for (double delay : ds.burstDelaySec)
    {
        const double localTime = ds.time - delay;
        if (localTime >= 0.0)
        {
            bursts += DrumEnv(localTime, DrumParam(src.transientDecaySec, 0.018));
        }
    }
    const double noise = FilterDrumNoise(ds, NextDrumNoise(ds, DrumNoiseColor(src)))
        * DrumParam(src.noiseLevel, 0.85)
        * bursts;
    const double tail = FilterDrumNoise(ds, NextDrumNoise(ds, DrumNoiseColor(src)))
        * DrumParam(src.airLevel, 0.25)
        * DrumEnv(ds.time, DrumDecay(src, ds, 0.16));
    return DrumSoftClip(noise + tail, src.drive);
}

double RenderDrumSample(const DrumConfig& src, Voice& voices, size_t i, double dt, int sampleRate)
{
    auto& ds = std::get<DrumVoiceState>(voices.sourceState[i]);
    PrepareDrumRelease(voices, ds, i);

    double w = 0.0;
    if (src.type == DrumType::Kick)
    {
        w = RenderKickSample(src, voices, ds, i, sampleRate);
    }
    else if (src.type == DrumType::Snare)
    {
        w = RenderSnareSample(src, voices, ds, i, sampleRate);
    }
    else if (src.type == DrumType::Hat)
    {
        w = RenderHatLikeSample(src, ds, sampleRate, false, false);
    }
    else if (src.type == DrumType::Tom)
    {
        w = RenderTomSample(src, voices, ds, i, sampleRate);
    }
    else if (src.type == DrumType::Rim)
    {
        w = RenderRimSample(src, voices, ds, i, sampleRate);
    }
    else if (src.type == DrumType::Clap)
    {
        w = RenderClapSample(src, ds, sampleRate);
    }
    else if (src.type == DrumType::Crash)
    {
        w = RenderHatLikeSample(src, ds, sampleRate, false, true);
    }
    else if (src.type == DrumType::Ride)
    {
        w = RenderHatLikeSample(src, ds, sampleRate, true, false);
    }

    ds.time += dt;
    return w;
}

void RenderDrumSource(
    const DrumConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    int sampleRate,
    SourceRenderFrame& frame)
{
    const double drumGain = (src.gain > 0.0) ? src.gain : 1.0;
    frame.sample = RenderDrumSample(src, voices, i, in.dt, sampleRate);
    frame.sourceGain = drumGain;
}
