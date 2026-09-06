void RenderFmSource(
    const FmConfig& src, Voice& voices, size_t i,
    const VoiceRenderInput& in, SourceRenderFrame& frame)
{
    auto& fs = std::get<FmVoiceState>(voices.sourceState[i]);
    const ModulationResult mod = EvaluateModulationSplitRate(
        fs.modulation, src.modulation, in.dt,
        ModulationInput{ in.expressionVelocity, in.modwheel, in.channelPressure, in.polyPressure }, 4);
    frame.sample = fs.chip->Sample(src,
        voices.phaseInc[i] / in.dt * in.pitchFactor * mod.pitchMul,
        mod.fmIndexMul * in.expressionFmIndexMul, voices.released[i] != 0) * mod.ampMul;
    SetFilterMode(fs.filter, src.filterMode);
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = src.filterCutoffHz;
    frame.shaperResonanceMul = mod.resonanceMul;
    frame.shaperDrive = src.drive;
    frame.shaperDriveNorm = src.drive > 0.0 ? 1.0 / std::tanh(src.drive * 20.0) : 1.0;
    frame.shaperFilterDrive = src.filterDrive;
}
