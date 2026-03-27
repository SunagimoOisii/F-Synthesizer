void RenderSourceFrame(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    int sampleRate,
    SourceRenderFrame& frame)
{
    std::visit([&](const auto& source)
    {
        using T = std::decay_t<decltype(source)>;
        if constexpr (std::is_same_v<T, WaveformConfig>)
        {
            RenderWaveformSource(source, voices, i, in, frame);
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            RenderAnalogSource(source, voices, i, in, frame);
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            RenderNoiseSource(source, frame);
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            RenderFmSource(source, voices, i, in, frame);
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            RenderDrumSource(source, voices, i, in, sampleRate, frame);
        }
        else if constexpr (std::is_same_v<T, PsgConfig>)
        {
            RenderPsgSource(source, voices, i, in, frame);
        }
    }, src);
}

void ApplyCommonShaper(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    if (frame.shaperKind == CommonShaperKind::BiquadFilter)
    {
        // filter を持つ方式のみ処理する（NoteOffVoiceModulation と同パターン）。
        // filterCutoffSmoothing を持つ方式（現行: waveform）のみ smoothing を適用する。
        std::visit([&](auto& st)
        {
            if constexpr (requires { st.filter; })
            {
                double filterCutoffHz = frame.shaperCutoffHz;
                if constexpr (requires { st.filterCutoffSmoothing; })
                {
                    const auto* waveformSrc = std::get_if<WaveformConfig>(&src);
                    const auto* analogSrc = std::get_if<AnalogConfig>(&src);
                    const bool smoothingEnabled =
                        (waveformSrc && waveformSrc->smoothing.enabled) ||
                        (analogSrc && analogSrc->smoothing.enabled);
                    if (smoothingEnabled)
                    {
                        SetSmoothedTarget(st.filterCutoffSmoothing, filterCutoffHz);
                        filterCutoffHz = StepSmoothedParam(st.filterCutoffSmoothing);
                    }
                }
                filterCutoffHz *= CutoffScaleFromBrightness(in.brightness);
                SetFilterCutoffHz(st.filter, filterCutoffHz);
                const double baseResonance = SourceFilterResonance(src);
                SetFilterResonance(st.filter, baseResonance * ResonanceScaleFromCc(in.resonance) * frame.shaperResonanceMul);
                frame.sample = ProcessFilterSample(st.filter, frame.sample);
            }
        }, voices.sourceState[i]);
    }

    if (frame.shaperDrive > 0.0)
    {
        // tanh ソフトクリップ: tanh(k*x) / tanh(k), k = drive * 20.0
        const double k = frame.shaperDrive * 20.0;
        if (frame.shaperDriveNorm > 0.0)
        {
            frame.sample = std::tanh(k * frame.sample) * frame.shaperDriveNorm;
        }
    }
}

void ApplyModulationLayer(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    SourceRenderFrame& frame)
{
    std::visit([&](const auto& source)
    {
        using T = std::decay_t<decltype(source)>;
        if constexpr (std::is_same_v<T, WaveformConfig>)
        {
            auto& ws = std::get<WaveformVoiceState>(voices.sourceState[i]);
            double ampMul = frame.ampMul;
            if (source.smoothing.enabled)
            {
                SetSmoothedTarget(ws.ampSmoothing, ampMul);
                ampMul = StepSmoothedParam(ws.ampSmoothing);
            }
            frame.ampMul = ampMul;
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            auto& as = std::get<AnalogVoiceState>(voices.sourceState[i]);
            double ampMul = frame.ampMul;
            if (source.smoothing.enabled)
            {
                SetSmoothedTarget(as.ampSmoothing, ampMul);
                ampMul = StepSmoothedParam(as.ampSmoothing);
            }
            frame.ampMul = ampMul;
        }
    }, src);
}
