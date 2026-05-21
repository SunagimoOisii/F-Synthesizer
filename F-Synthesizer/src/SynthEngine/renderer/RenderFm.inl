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

double ShapeFmEnv(double value, double curve)
{
    const double clamped = std::clamp(value, 0.0, 1.0);
    const double c = std::clamp(curve, 0.0, 1.0);
    if (c <= 0.0)
    {
        return clamped;
    }
    return std::pow(clamped, 1.0 + c * 3.0);
}

double StepFmOperatorEnv(ADSRState& state, const ModEnvelopeConfig& env, double dt)
{
    const double value = StepADSR(
        state,
        dt,
        std::max(0.0, env.attackSec),
        std::max(0.0, env.decaySec),
        std::clamp(env.sustainLevel, 0.0, 1.0),
        std::max(0.0, env.releaseSec));
    return ShapeFmEnv(value, env.curve);
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
    const double ampScale = mod.ampMul;
    double opLevel[4]{};
    double opIndex[4]{};
    for (int op = 0; op < 4; op++)
    {
        opLevel[op] = src.ops[op].level * StepFmOperatorEnv(fs.opLevelEnv[op], src.ops[op].levelEnv, in.dt);
        opIndex[op] = src.ops[op].index * indexScale * StepFmOperatorEnv(fs.opIndexEnv[op], src.ops[op].indexEnv, in.dt);
    }
    const double op0 = SampleOp(src.ops[0].wave, fs.opPhase[0], feedbackSample, opIndex[0]);

    switch (src.algorithm)
    {
    case 1:
    {
        const double mod0 = op0 * opLevel[0];
        const double out0 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, opIndex[2]) * opLevel[2];
        const double out2 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, opIndex[3]) * opLevel[3];
        frame.sample = (out0 + out2) * 0.5 * ampScale;
        break;
    }
    case 2:
    {
        const double mod0 = op0 * opLevel[0];
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod0, opIndex[2]) * opLevel[2];
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod0, opIndex[3]) * opLevel[3];
        frame.sample = (out1 + out2 + out3) / 3.0 * ampScale;
        break;
    }
    case 3:
    {
        const double mod0 = op0 * opLevel[0];
        const double mod1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod1, opIndex[2]) * opLevel[2];
        const double out = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, opIndex[3]) * opLevel[3];
        frame.sample = out * ampScale;
        break;
    }
    case 4:
    {
        const double mod0 = op0 * opLevel[0];
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, opIndex[2]) * opLevel[2];
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, opIndex[3]) * opLevel[3];
        frame.sample = (out1 + out3) * 0.5 * ampScale;
        break;
    }
    case 5:
    {
        const double mod0 = op0 * opLevel[0];
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod0, opIndex[2]) * opLevel[2];
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod0, opIndex[3]) * opLevel[3];
        frame.sample = (out1 + out2 + out3) / 3.0 * ampScale;
        break;
    }
    case 6:
    {
        const double mod0 = op0 * opLevel[0];
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, opIndex[2]) * opLevel[2];
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], 0.0, opIndex[3]) * opLevel[3];
        frame.sample = (out1 + out2 + out3) / 3.0 * ampScale;
        break;
    }
    case 7:
    {
        const double out0 = op0 * opLevel[0];
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], 0.0, opIndex[1]) * opLevel[1];
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, opIndex[2]) * opLevel[2];
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], 0.0, opIndex[3]) * opLevel[3];
        frame.sample = (out0 + out1 + out2 + out3) * 0.25 * ampScale;
        break;
    }
    case 0:
    default:
    {
        const double mod0 = op0 * opLevel[0];
        const double out = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, opIndex[1]) * opLevel[1];
        frame.sample = out * ampScale;
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
