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

double RenderKickSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    double pitchFactor = 1.0;
    if (ds.pitchDecaySec > 0.0)
    {
        pitchFactor += (ds.pitchDrop - 1.0) * std::exp(-ds.time / ds.pitchDecaySec);
    }
    const double freq = ds.baseFreq * pitchFactor;
    voices.phase[i] += (freq / sampleRate);
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    return SampleWavePhase(WaveType::Sine, voices.phase[i]);
}

double RenderSnareSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.3;
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 0.7;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 1200.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 6000.0;
    const WaveType toneWave = static_cast<WaveType>(src.toneWave);
    const NoiseType noiseType = static_cast<NoiseType>(src.noiseType);
    EnsureDrumFilters(ds, hpCut, lpCut, sampleRate);
    voices.phase[i] += ds.baseFreq / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double toneInc = ds.baseFreq / sampleRate;
    const double tone = SampleWavePhase(toneWave, voices.phase[i], toneInc);
    const double noise = SampleNoise(noiseType);
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;
    return toneLevel * tone + noiseLevel * lp;
}

double RenderHatSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 1.0;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 6000.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 12000.0;
    const double toneFreq = (src.toneFreq > 0.0) ? src.toneFreq : 8000.0;
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.2;
    const WaveType toneWave = static_cast<WaveType>(src.toneWave);
    const NoiseType noiseType = static_cast<NoiseType>(src.noiseType);
    EnsureDrumFilters(ds, hpCut, lpCut, sampleRate);
    const double noise = SampleNoise(noiseType);
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;
    voices.phase[i] += toneFreq / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double toneInc = toneFreq / sampleRate;
    const double tone = SampleWavePhase(toneWave, voices.phase[i], toneInc);
    return noiseLevel * lp + toneLevel * tone;
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
