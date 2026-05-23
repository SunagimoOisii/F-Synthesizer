#include "gui/GUIConfigUtils.h"

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <variant>

#include "config/SourceRegistry.h"

namespace
{
bool NearlyEq(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

bool DrumConfigEquals(const DrumConfig& a, const DrumConfig& b)
{
    return a.type == b.type &&
        NearlyEq(a.gain, b.gain) &&
        NearlyEq(a.bodyFreq, b.bodyFreq) &&
        NearlyEq(a.bodyLevel, b.bodyLevel) &&
        NearlyEq(a.bodyDecaySec, b.bodyDecaySec) &&
        NearlyEq(a.pitchStart, b.pitchStart) &&
        NearlyEq(a.pitchDecaySec, b.pitchDecaySec) &&
        NearlyEq(a.transientLevel, b.transientLevel) &&
        NearlyEq(a.transientDecaySec, b.transientDecaySec) &&
        NearlyEq(a.noiseLevel, b.noiseLevel) &&
        NearlyEq(a.snapLevel, b.snapLevel) &&
        NearlyEq(a.snapDecaySec, b.snapDecaySec) &&
        NearlyEq(a.metalLevel, b.metalLevel) &&
        NearlyEq(a.airLevel, b.airLevel) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.hpCut, b.hpCut) &&
        NearlyEq(a.lpCut, b.lpCut) &&
        NearlyEq(a.drive, b.drive) &&
        a.noiseColor == b.noiseColor &&
        NearlyEq(a.velocityToTone, b.velocityToTone) &&
        NearlyEq(a.velocityToDecay, b.velocityToDecay) &&
        NearlyEq(a.humanizePitchCents, b.humanizePitchCents) &&
        NearlyEq(a.humanizeDecayPct, b.humanizeDecayPct);
}

bool ModulationConfigEquals(const ModulationConfig& a, const ModulationConfig& b)
{
    if (a.lfo1.wave != b.lfo1.wave ||
        !NearlyEq(a.lfo1.rateHz, b.lfo1.rateHz) ||
        !NearlyEq(a.lfo1.depth, b.lfo1.depth) ||
        a.lfo1.bipolar != b.lfo1.bipolar ||
        a.lfo1.keySync != b.lfo1.keySync ||
        !NearlyEq(a.lfo1.delayMs, b.lfo1.delayMs) ||
        !NearlyEq(a.lfo1.fadeMs, b.lfo1.fadeMs))
    {
        return false;
    }
    if (!NearlyEq(a.env2.attackSec, b.env2.attackSec) ||
        !NearlyEq(a.env2.decaySec, b.env2.decaySec) ||
        !NearlyEq(a.env2.sustainLevel, b.env2.sustainLevel) ||
        !NearlyEq(a.env2.releaseSec, b.env2.releaseSec) ||
        !NearlyEq(a.env2.curve, b.env2.curve))
    {
        return false;
    }
    for (size_t i = 0; i < a.matrix.routes.size(); i++)
    {
        const ModRoute& ar = a.matrix.routes[i];
        const ModRoute& br = b.matrix.routes[i];
        if (ar.source != br.source ||
            ar.destination != br.destination ||
            !NearlyEq(ar.amount, br.amount) ||
            ar.enabled != br.enabled)
        {
            return false;
        }
    }
    return true;
}

bool ModEnvelopeConfigEquals(const ModEnvelopeConfig& a, const ModEnvelopeConfig& b)
{
    return NearlyEq(a.attackSec, b.attackSec) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.sustainLevel, b.sustainLevel) &&
        NearlyEq(a.releaseSec, b.releaseSec) &&
        NearlyEq(a.curve, b.curve);
}

bool AttackLayerConfigEquals(const AttackLayerConfig& a, const AttackLayerConfig& b)
{
    return a.enabled == b.enabled &&
        a.type == b.type &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.brightness, b.brightness) &&
        NearlyEq(a.bodyMix, b.bodyMix) &&
        NearlyEq(a.pitchOffsetSemis, b.pitchOffsetSemis) &&
        NearlyEq(a.drive, b.drive);
}

bool BassLayerConfigEquals(const BassLayerConfig& a, const BassLayerConfig& b)
{
    return a.enabled == b.enabled &&
        a.type == b.type &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.subLevel, b.subLevel) &&
        NearlyEq(a.bodyLevel, b.bodyLevel) &&
        NearlyEq(a.gritLevel, b.gritLevel) &&
        NearlyEq(a.cutoffHz, b.cutoffHz) &&
        NearlyEq(a.drive, b.drive) &&
        NearlyEq(a.pitchOffsetSemis, b.pitchOffsetSemis) &&
        NearlyEq(a.velocityToDrive, b.velocityToDrive) &&
        NearlyEq(a.focusHz, b.focusHz) &&
        NearlyEq(a.focusLevel, b.focusLevel) &&
        NearlyEq(a.bodySaturation, b.bodySaturation) &&
        NearlyEq(a.gritTone, b.gritTone) &&
        NearlyEq(a.attackBoost, b.attackBoost) &&
        NearlyEq(a.attackDecaySec, b.attackDecaySec);
}

bool LeadLayerConfigEquals(const LeadLayerConfig& a, const LeadLayerConfig& b)
{
    return a.enabled == b.enabled &&
        a.type == b.type &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.edgeLevel, b.edgeLevel) &&
        NearlyEq(a.bodyLevel, b.bodyLevel) &&
        NearlyEq(a.detuneCents, b.detuneCents) &&
        NearlyEq(a.pitchBendSemis, b.pitchBendSemis) &&
        NearlyEq(a.bendDecaySec, b.bendDecaySec) &&
        NearlyEq(a.attackBoost, b.attackBoost) &&
        NearlyEq(a.attackDecaySec, b.attackDecaySec) &&
        NearlyEq(a.drive, b.drive) &&
        NearlyEq(a.characterLevel, b.characterLevel) &&
        NearlyEq(a.characterTone, b.characterTone) &&
        NearlyEq(a.biteLevel, b.biteLevel) &&
        NearlyEq(a.biteDecaySec, b.biteDecaySec) &&
        NearlyEq(a.wobbleDepthCents, b.wobbleDepthCents) &&
        NearlyEq(a.wobbleRateHz, b.wobbleRateHz);
}

bool ChordLayerConfigEquals(const ChordLayerConfig& a, const ChordLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        a.intervalsSemis == b.intervalsSemis &&
        std::equal(a.voiceLevels.begin(), a.voiceLevels.end(), b.voiceLevels.begin(), [](double av, double bv) {
            return NearlyEq(av, bv);
        }) &&
        NearlyEq(a.detuneCents, b.detuneCents) &&
        NearlyEq(a.spread, b.spread) &&
        NearlyEq(a.cutoffHz, b.cutoffHz) &&
        NearlyEq(a.drive, b.drive);
}

bool PadLayerConfigEquals(const PadLayerConfig& a, const PadLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.octaveLevel, b.octaveLevel) &&
        NearlyEq(a.detuneCents, b.detuneCents) &&
        NearlyEq(a.spread, b.spread) &&
        NearlyEq(a.fadeInSec, b.fadeInSec) &&
        NearlyEq(a.brightness, b.brightness) &&
        NearlyEq(a.motionDepth, b.motionDepth) &&
        NearlyEq(a.motionRateHz, b.motionRateHz) &&
        NearlyEq(a.cutoffHz, b.cutoffHz) &&
        NearlyEq(a.drive, b.drive);
}

bool PluckLayerConfigEquals(const PluckLayerConfig& a, const PluckLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.brightness, b.brightness) &&
        NearlyEq(a.noiseMix, b.noiseMix) &&
        NearlyEq(a.pitchOffsetSemis, b.pitchOffsetSemis) &&
        NearlyEq(a.bodySend, b.bodySend) &&
        NearlyEq(a.drive, b.drive);
}

bool StringLayerConfigEquals(const StringLayerConfig& a, const StringLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.bowLevel, b.bowLevel) &&
        NearlyEq(a.detuneCents, b.detuneCents) &&
        NearlyEq(a.spread, b.spread) &&
        NearlyEq(a.fadeInSec, b.fadeInSec) &&
        NearlyEq(a.brightness, b.brightness) &&
        NearlyEq(a.motionDepth, b.motionDepth) &&
        NearlyEq(a.motionRateHz, b.motionRateHz) &&
        NearlyEq(a.bodySend, b.bodySend) &&
        NearlyEq(a.drive, b.drive);
}

bool BodyLayerConfigEquals(const BodyLayerConfig& a, const BodyLayerConfig& b)
{
    return a.enabled == b.enabled &&
        a.mode == b.mode &&
        NearlyEq(a.mix, b.mix) &&
        NearlyEq(a.size, b.size) &&
        NearlyEq(a.tone, b.tone) &&
        NearlyEq(a.damping, b.damping) &&
        NearlyEq(a.stereo, b.stereo) &&
        NearlyEq(a.drive, b.drive);
}

bool HarmonicLayerConfigEquals(const HarmonicLayerConfig& a, const HarmonicLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        std::equal(a.harmonicLevels.begin(), a.harmonicLevels.end(), b.harmonicLevels.begin(), [](double av, double bv) {
            return NearlyEq(av, bv);
        }) &&
        NearlyEq(a.brightness, b.brightness) &&
        NearlyEq(a.keyClick, b.keyClick) &&
        NearlyEq(a.attackSec, b.attackSec) &&
        NearlyEq(a.releaseDamp, b.releaseDamp) &&
        NearlyEq(a.drive, b.drive) &&
        NearlyEq(a.stereo, b.stereo);
}

bool PowerChordLayerConfigEquals(const PowerChordLayerConfig& a, const PowerChordLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.fifthLevel, b.fifthLevel) &&
        NearlyEq(a.octaveLevel, b.octaveLevel) &&
        NearlyEq(a.detuneCents, b.detuneCents) &&
        NearlyEq(a.spread, b.spread) &&
        NearlyEq(a.tone, b.tone) &&
        NearlyEq(a.drive, b.drive);
}

bool ChugLayerConfigEquals(const ChugLayerConfig& a, const ChugLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.lowPunch, b.lowPunch) &&
        NearlyEq(a.pick, b.pick) &&
        NearlyEq(a.tone, b.tone) &&
        NearlyEq(a.tightness, b.tightness) &&
        NearlyEq(a.drive, b.drive);
}

bool AmpCabLayerConfigEquals(const AmpCabLayerConfig& a, const AmpCabLayerConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.drive, b.drive) &&
        NearlyEq(a.tone, b.tone) &&
        NearlyEq(a.cabLow, b.cabLow) &&
        NearlyEq(a.cabHigh, b.cabHigh) &&
        NearlyEq(a.presence, b.presence) &&
        NearlyEq(a.output, b.output);
}

bool ExpressionMapConfigEquals(const ExpressionMapConfig& a, const ExpressionMapConfig& b)
{
    return a.enabled == b.enabled &&
        NearlyEq(a.velocityCurve, b.velocityCurve) &&
        NearlyEq(a.velocityToAmp, b.velocityToAmp) &&
        NearlyEq(a.velocityToBrightness, b.velocityToBrightness) &&
        NearlyEq(a.velocityToFmIndex, b.velocityToFmIndex) &&
        NearlyEq(a.velocityToAttack, b.velocityToAttack) &&
        NearlyEq(a.velocityToBass, b.velocityToBass) &&
        NearlyEq(a.velocityToLead, b.velocityToLead) &&
        NearlyEq(a.velocityToChord, b.velocityToChord) &&
        NearlyEq(a.velocityToPad, b.velocityToPad) &&
        NearlyEq(a.velocityToPluck, b.velocityToPluck) &&
        NearlyEq(a.velocityToString, b.velocityToString) &&
        NearlyEq(a.velocityToBody, b.velocityToBody) &&
        NearlyEq(a.modWheelToBrightness, b.modWheelToBrightness) &&
        NearlyEq(a.modWheelToPad, b.modWheelToPad) &&
        NearlyEq(a.modWheelToString, b.modWheelToString) &&
        NearlyEq(a.pressureToDrive, b.pressureToDrive) &&
        NearlyEq(a.pressureToFilterDrive, b.pressureToFilterDrive) &&
        NearlyEq(a.cc74ToBrightness, b.cc74ToBrightness) &&
        NearlyEq(a.cc74ToPadBrightness, b.cc74ToPadBrightness) &&
        NearlyEq(a.cc74ToStringBrightness, b.cc74ToStringBrightness);
}

template <typename SmoothingT>
bool WaveformLikeSmoothingConfigEquals(const SmoothingT& a, const SmoothingT& b)
{
    return a.enabled == b.enabled &&
        a.pitchEnabled == b.pitchEnabled &&
        NearlyEq(a.ampTimeMs, b.ampTimeMs) &&
        NearlyEq(a.pitchTimeMs, b.pitchTimeMs) &&
        NearlyEq(a.filterCutoffTimeMs, b.filterCutoffTimeMs);
}

bool WaveformSmoothingConfigEquals(const WaveformConfig::SmoothingConfig& a, const WaveformConfig::SmoothingConfig& b)
{
    return WaveformLikeSmoothingConfigEquals(a, b);
}

bool AnalogSmoothingConfigEquals(const AnalogConfig::SmoothingConfig& a, const AnalogConfig::SmoothingConfig& b)
{
    return WaveformLikeSmoothingConfigEquals(a, b);
}

template <typename ArpeggioT>
bool ArpeggioConfigEquals(const ArpeggioT& a, const ArpeggioT& b)
{
    if (a.enabled != b.enabled ||
        !NearlyEq(a.rateHz, b.rateHz) ||
        a.steps != b.steps)
    {
        return false;
    }
    for (size_t i = 0; i < a.semitones.size(); i++)
    {
        if (a.semitones[i] != b.semitones[i])
        {
            return false;
        }
    }
    return true;
}

template <typename SourceT, typename SmoothingEqFn>
bool WaveformLikeSourceConfigEqualsCommon(
    const SourceT& a,
    const SourceT& b,
    bool compareDrive,
    const SmoothingEqFn& smoothingEq)
{
    if (a.wave != b.wave ||
        a.unisonVoices != b.unisonVoices ||
        !NearlyEq(a.unisonDetuneCents, b.unisonDetuneCents) ||
        !NearlyEq(a.unisonSpread, b.unisonSpread) ||
        !NearlyEq(a.subOscLevel, b.subOscLevel) ||
        !NearlyEq(a.pulseWidth, b.pulseWidth) ||
        a.hardSyncEnabled != b.hardSyncEnabled ||
        !NearlyEq(a.hardSyncRatio, b.hardSyncRatio) ||
        a.ringModEnabled != b.ringModEnabled ||
        !NearlyEq(a.ringModRatio, b.ringModRatio) ||
        !NearlyEq(a.ringModMix, b.ringModMix) ||
        a.filterMode != b.filterMode ||
        !NearlyEq(a.filterCutoffHz, b.filterCutoffHz) ||
        !NearlyEq(a.filterResonance, b.filterResonance) ||
        !NearlyEq(a.filterKeytrack, b.filterKeytrack) ||
        !NearlyEq(a.filterDrive, b.filterDrive) ||
        !ArpeggioConfigEquals(a.arpeggio, b.arpeggio) ||
        !smoothingEq(a.smoothing, b.smoothing) ||
        !ModulationConfigEquals(a.modulation, b.modulation))
    {
        return false;
    }
    if (compareDrive && !NearlyEq(a.drive, b.drive))
    {
        return false;
    }
    return true;
}

bool SourceConfigEquals(const SourceConfig& a, const SourceConfig& b)
{
    if (a.index() != b.index())
    {
        return false;
    }
    // variant型比較は型一致を先に確認してから分岐し、比較漏れを防ぐ。
    return std::visit([&](const auto& av) -> bool
        {
            using T = std::decay_t<decltype(av)>;
            const auto* bv = std::get_if<T>(&b);
            if (bv == nullptr) return false;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                return WaveformLikeSourceConfigEqualsCommon(
                    av,
                    *bv,
                    false,
                    [](const auto& lhs, const auto& rhs) { return WaveformSmoothingConfigEquals(lhs, rhs); });
            }
            else if constexpr (std::is_same_v<T, AnalogConfig>)
            {
                return WaveformLikeSourceConfigEqualsCommon(
                    av,
                    *bv,
                    true,
                    [](const auto& lhs, const auto& rhs) { return AnalogSmoothingConfigEquals(lhs, rhs); }) &&
                    NearlyEq(av.driftDepthCents, bv->driftDepthCents) &&
                    NearlyEq(av.driftRateHz, bv->driftRateHz);
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                return av.noise == bv->noise &&
                    av.filterMode == bv->filterMode &&
                    NearlyEq(av.filterCutoffHz, bv->filterCutoffHz) &&
                    NearlyEq(av.filterResonance, bv->filterResonance) &&
                    NearlyEq(av.filterDrive, bv->filterDrive);
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                if (av.algorithm != bv->algorithm ||
                    !NearlyEq(av.feedback, bv->feedback) ||
                    av.filterMode != bv->filterMode ||
                    !NearlyEq(av.filterCutoffHz, bv->filterCutoffHz) ||
                    !NearlyEq(av.filterResonance, bv->filterResonance) ||
                    !NearlyEq(av.filterDrive, bv->filterDrive) ||
                    !NearlyEq(av.drive, bv->drive) ||
                    !ModulationConfigEquals(av.modulation, bv->modulation))
                {
                    return false;
                }
                for (size_t i = 0; i < 4; i++)
                {
                    const FmOperator& ao = av.ops[i];
                    const FmOperator& bo = bv->ops[i];
                    if (ao.wave != bo.wave ||
                        !NearlyEq(ao.ratio, bo.ratio) ||
                        !NearlyEq(ao.level, bo.level) ||
                        !NearlyEq(ao.index, bo.index) ||
                        !ModEnvelopeConfigEquals(ao.levelEnv, bo.levelEnv) ||
                        !ModEnvelopeConfigEquals(ao.indexEnv, bo.indexEnv))
                    {
                        return false;
                    }
                }
                return true;
            }
            else if constexpr (std::is_same_v<T, PsgConfig>)
            {
                return av.wave == bv->wave &&
                    av.duty == bv->duty &&
                    av.volumeSteps == bv->volumeSteps &&
                    av.maxVoices == bv->maxVoices;
            }
            else if constexpr (std::is_same_v<T, DrumKitConfig>)
            {
                for (int i = 0; i < 128; i++)
                {
                    if (!DrumConfigEquals(av.map[i], bv->map[i])) return false;
                }
                return true;
            }
            return false;
        }, a);
}
} // namespace

namespace gui
{
WaveType WaveFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return WaveType::Sine;
    case 1: return WaveType::Square;
    case 2: return WaveType::Saw;
    case 3: return WaveType::Triangle;
    default: return WaveType::Saw;
    }
}

int WaveToIndex(WaveType w)
{
    switch (w)
    {
    case WaveType::Sine: return 0;
    case WaveType::Square: return 1;
    case WaveType::Saw: return 2;
    case WaveType::Triangle: return 3;
    }
    return 2;
}

NoiseType NoiseFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return NoiseType::White;
    case 1: return NoiseType::Pink;
    case 2: return NoiseType::Brown;
    case 3: return NoiseType::Blue;
    default: return NoiseType::White;
    }
}

int NoiseToIndex(NoiseType n)
{
    switch (n)
    {
    case NoiseType::White: return 0;
    case NoiseType::Pink: return 1;
    case NoiseType::Brown: return 2;
    case NoiseType::Blue: return 3;
    }
    return 0;
}

int SourceTypeIndex(const SourceConfig& src)
{
    return config::SourceKindToIndex(config::SourceConfigKind(src));
}

bool ChannelConfigEquals(const ChannelConfig& a, const ChannelConfig& b)
{
    return NearlyEq(a.amp, b.amp) &&
        NearlyEq(a.attackSec, b.attackSec) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.sustainLevel, b.sustainLevel) &&
        NearlyEq(a.releaseSec, b.releaseSec) &&
        NearlyEq(a.portamentoTimeSec, b.portamentoTimeSec) &&
        AttackLayerConfigEquals(a.attackLayer, b.attackLayer) &&
        BassLayerConfigEquals(a.bassLayer, b.bassLayer) &&
        LeadLayerConfigEquals(a.leadLayer, b.leadLayer) &&
        ChordLayerConfigEquals(a.chordLayer, b.chordLayer) &&
        PadLayerConfigEquals(a.padLayer, b.padLayer) &&
        PluckLayerConfigEquals(a.pluckLayer, b.pluckLayer) &&
        StringLayerConfigEquals(a.stringLayer, b.stringLayer) &&
        BodyLayerConfigEquals(a.bodyLayer, b.bodyLayer) &&
        HarmonicLayerConfigEquals(a.harmonicLayer, b.harmonicLayer) &&
        PowerChordLayerConfigEquals(a.powerChordLayer, b.powerChordLayer) &&
        ChugLayerConfigEquals(a.chugLayer, b.chugLayer) &&
        AmpCabLayerConfigEquals(a.ampCabLayer, b.ampCabLayer) &&
        ExpressionMapConfigEquals(a.expressionMap, b.expressionMap) &&
        SourceConfigEquals(a.source, b.source);
}

bool ChannelMixStateEquals(const ChannelMixState& a, const ChannelMixState& b)
{
    return a.mute == b.mute &&
        a.solo == b.solo &&
        NearlyEq(a.level, b.level) &&
        NearlyEq(a.pan, b.pan) &&
        NearlyEq(a.gain, b.gain);
}

void WriteJSONEscaped(std::ostream& out, const std::string& s)
{
    for (char c : s)
    {
        if (c == '\\') out << "\\\\";
        else if (c == '"') out << "\\\"";
        else if (c == '\n') out << "\\n";
        else out << c;
    }
}

} // namespace gui
