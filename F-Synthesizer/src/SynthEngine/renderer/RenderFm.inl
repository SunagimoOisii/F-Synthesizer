// オペレータ1個分の位相サンプリング（FM変調込み）。
// modSample: 前段オペレータの出力。0.0 = 変調なし。
double SampleOp(
    WaveType wave,
    double phase,
    double modSample,
    double index)
{
    return SampleWavePhase(wave, phase + modSample * index);
}

void RenderFmSource(
    const FmConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& fs = std::get<FmVoiceState>(voices.sourceState[i]);
    const ModulationResult mod = EvaluateModulationSplitRate(
        fs.modulation,
        src.modulation,
        in.dt,
        ModulationInput{ in.velGain, in.modwheel, in.channelPressure, in.polyPressure },
        4);
    const double indexScale = mod.fmIndexMul;
    const double feedbackSample = fs.op0FeedbackSample * src.feedback;
    const double outLevelScale = mod.ampMul;
    const double op0 = SampleOp(src.ops[0].wave, fs.opPhase[0], feedbackSample, src.ops[0].index * indexScale);

    switch (src.algorithm)
    {
    case 1:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out0 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out2 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out0 + out2) * 0.5 * outLevelScale;
        break;
    }
    case 2:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out2 + out3) / 3.0 * outLevelScale;
        break;
    }
    case 3:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double mod1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod1, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = out * outLevelScale;
        break;
    }
    case 4:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out3) * 0.5 * outLevelScale;
        break;
    }
    case 5:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out2 + out3) / 3.0 * outLevelScale;
        break;
    }
    case 6:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], 0.0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out2 + out3) / 3.0 * outLevelScale;
        break;
    }
    case 7:
    {
        const double out0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], 0.0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], 0.0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out0 + out1 + out2 + out3) * 0.25 * outLevelScale;
        break;
    }
    case 0:
    default:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        frame.sample = out * outLevelScale;
        break;
    }
    }

    fs.op0FeedbackSample = op0;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = src.filterCutoffHz;
    frame.shaperResonanceMul = mod.resonanceMul;
    frame.shaperDrive = src.drive;
    frame.shaperDriveNorm = fs.driveNorm;

    for (int k = 0; k < 4; k++)
    {
        fs.opPhase[k] += voices.phaseInc[i] * in.pitchFactor * mod.pitchMul * src.ops[k].ratio;
        if (fs.opPhase[k] >= 1.0)
        {
            fs.opPhase[k] -= 1.0;
        }
    }
}
