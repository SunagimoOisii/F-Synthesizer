void EnsureDrumFilters(DrumVoiceState& ds, double hpCut, double lpCut, int sampleRate)
{
    // 初回のみ係数を計算し、同一voice中は再利用する。
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
    // Drum は attack+decay 到達で自動 NoteOff へ移す。
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

double RenderKickSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    double pitchFactor = 1.0;
    if (ds.pitchDecaySec > 0.0)
    {
        pitchFactor += (ds.pitchStart - 1.0) * std::exp(-ds.time / ds.pitchDecaySec);
    }
    const double freq = ds.bodyFreq * pitchFactor;
    voices.phase[i] += (freq / sampleRate);
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;

    const double clickDecay = DrumParam(src.clickDecaySec, 0.008);
    const double bodyLevel = DrumParam(src.bodyLevel, 0.85);
    const double clickLevel = DrumParam(src.clickLevel, 0.25);
    const double body = SampleWavePhase(WaveType::Sine, voices.phase[i]) * bodyLevel * DrumEnv(ds.time, 0.18);

    ds.clickPhase += 2400.0 / sampleRate;
    if (ds.clickPhase >= 1.0) ds.clickPhase -= 1.0;
    const double clickTone = SampleWavePhase(WaveType::Square, ds.clickPhase);
    const double clickNoise = SampleNoise(NoiseType::Blue);
    const double click = (0.65 * clickTone + 0.35 * clickNoise) * clickLevel * DrumEnv(ds.time, clickDecay);
    return DrumSoftClip(body + click, src.drive);
}

double RenderSnareSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double bodyLevel = DrumParam(src.bodyLevel, 0.45);
    const double snapLevel = DrumParam(src.snapLevel, 0.75);
    const double snapDecay = DrumParam(src.snapDecaySec, 0.055);
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 1200.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 5200.0;
    EnsureDrumFilters(ds, hpCut, lpCut, sampleRate);

    voices.phase[i] += ds.bodyFreq / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double bodyInc = ds.bodyFreq / sampleRate;
    const double body = SampleWavePhase(WaveType::Square, voices.phase[i], bodyInc) * bodyLevel * DrumEnv(ds.time, 0.09);

    const double noise = SampleNoise(DrumNoiseColor(src));
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;

    const double snap = lp * snapLevel * DrumEnv(ds.time, snapDecay);
    return DrumSoftClip(body + snap, src.drive);
}

double RenderHatSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double airLevel = DrumParam(src.airLevel, 0.35);
    const double metalLevel = DrumParam(src.metalLevel, 0.55);
    const double decay = DrumParam(src.decaySec, 0.045);
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 5200.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 11000.0;
    EnsureDrumFilters(ds, hpCut, lpCut, sampleRate);

    const double noise = SampleNoise(DrumNoiseColor(src));
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;

    constexpr double metalFreqs[4] = { 5600.0, 7100.0, 8300.0, 9700.0 };
    double metal = 0.0;
    for (size_t phaseIndex = 0; phaseIndex < ds.metalPhase.size(); phaseIndex++)
    {
        ds.metalPhase[phaseIndex] += metalFreqs[phaseIndex] / sampleRate;
        if (ds.metalPhase[phaseIndex] >= 1.0) ds.metalPhase[phaseIndex] -= 1.0;
        metal += SampleWavePhase(WaveType::Square, ds.metalPhase[phaseIndex]);
    }
    metal *= 0.25;

    const double env = DrumEnv(ds.time, decay);
    return DrumSoftClip((metal * metalLevel + lp * airLevel) * env, src.drive);
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
        w = RenderHatSample(src, voices, ds, i, sampleRate);
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
