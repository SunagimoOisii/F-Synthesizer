#include "config/SourceJSON.h"
#include "ConfigFileInternal.h"
#include "config/SourceRegistry.h"
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace config
{
namespace
{
using Json = nlohmann::json;
using namespace internal;

void CheckFinite(const Json &value)
{
    if (value.is_number_float() && !std::isfinite(value.get<double>()))
        throw std::runtime_error("sound contains a non-finite number");
    if (value.is_structured())
        for (const auto &child : value)
            CheckFinite(child);
}

Json DrumToJSON(const DrumConfig &d)
{
    return Json{{"gain", d.gain},
                {"bodyFreq", d.bodyFreq},
                {"bodyLevel", d.bodyLevel},
                {"bodyDecaySec", d.bodyDecaySec},
                {"pitchStart", d.pitchStart},
                {"pitchDecaySec", d.pitchDecaySec},
                {"transientLevel", d.transientLevel},
                {"transientDecaySec", d.transientDecaySec},
                {"noiseLevel", d.noiseLevel},
                {"snapLevel", d.snapLevel},
                {"snapDecaySec", d.snapDecaySec},
                {"metalLevel", d.metalLevel},
                {"airLevel", d.airLevel},
                {"decaySec", d.decaySec},
                {"hpCut", d.hpCut},
                {"lpCut", d.lpCut},
                {"drive", d.drive},
                {"velocityToTone", d.velocityToTone},
                {"velocityToDecay", d.velocityToDecay},
                {"humanizePitchCents", d.humanizePitchCents},
                {"humanizeDecayPct", d.humanizeDecayPct},
                {"drumType", DrumTypeToString(d.type)},
                {"noiseColor", NoiseTypeToString(static_cast<NoiseType>(d.noiseColor))}};
}

Json DrumBusToJSON(const DrumBusConfig &bus)
{
    return Json{{"enabled", bus.enabled},         {"level", bus.level},       {"attackTrim", bus.attackTrim},
                {"sustainLift", bus.sustainLift}, {"glue", bus.glue},         {"presenceCut", bus.presenceCut},
                {"lowTighten", bus.lowTighten},   {"roomSend", bus.roomSend}, {"driveTrim", bus.driveTrim}};
}

Json EnvelopeToJSON(const ModEnvelopeConfig &env)
{
    return Json{{"attackSec", env.attackSec},
                {"decaySec", env.decaySec},
                {"sustainLevel", env.sustainLevel},
                {"releaseSec", env.releaseSec},
                {"curve", env.curve}};
}

Json ModulationToJSON(const ModulationConfig &m)
{
    Json routes = Json::object();
    for (size_t i = 0; i < m.matrix.routes.size(); ++i)
    {
        const auto &r = m.matrix.routes[i];
        routes[std::to_string(i)] = {{"source", ModSourceToString(r.source)},
                                     {"destination", ModDestinationToString(r.destination)},
                                     {"amount", r.amount},
                                     {"enabled", r.enabled}};
    }
    return Json{{"lfo1", Json{{"rateHz", m.lfo1.rateHz},
                              {"depth", m.lfo1.depth},
                              {"bipolar", m.lfo1.bipolar},
                              {"keySync", m.lfo1.keySync},
                              {"delayMs", m.lfo1.delayMs},
                              {"fadeMs", m.lfo1.fadeMs},
                              {"wave", LfoWaveToString(m.lfo1.wave)}}},
                {"env2", EnvelopeToJSON(m.env2)},
                {"routes", std::move(routes)}};
}

template <typename T> Json FilterToJSON(const T &v)
{
    Json filter{{"mode", FilterModeToString(v.filterMode)},
                {"cutoffHz", v.filterCutoffHz},
                {"resonance", v.filterResonance},
                {"drive", v.filterDrive}};
    if constexpr (requires { v.filterKeytrack; })
        filter["keytrack"] = v.filterKeytrack;
    return filter;
}

template <typename T> Json WaveformFieldsToJSON(const T &v)
{
    return Json{{"unisonVoices", v.unisonVoices},
                {"unisonDetuneCents", v.unisonDetuneCents},
                {"unisonSpread", v.unisonSpread},
                {"subOscLevel", v.subOscLevel},
                {"pulseWidth", v.pulseWidth},
                {"hardSyncEnabled", v.hardSyncEnabled},
                {"hardSyncRatio", v.hardSyncRatio},
                {"ringModEnabled", v.ringModEnabled},
                {"ringModRatio", v.ringModRatio},
                {"ringModMix", v.ringModMix},
                {"drive", v.drive},
                {"wave", WaveTypeToString(v.wave)},
                {"filter", FilterToJSON(v)},
                {"arpeggio", Json{{"enabled", v.arpeggio.enabled},
                                  {"rateHz", v.arpeggio.rateHz},
                                  {"steps", v.arpeggio.steps},
                                  {"semitones", v.arpeggio.semitones}}},
                {"smoothing", Json{{"enabled", v.smoothing.enabled},
                                   {"pitchEnabled", v.smoothing.pitchEnabled},
                                   {"ampTimeMs", v.smoothing.ampTimeMs},
                                   {"pitchTimeMs", v.smoothing.pitchTimeMs},
                                   {"filterCutoffTimeMs", v.smoothing.filterCutoffTimeMs}}},
                {"modulation", ModulationToJSON(v.modulation)}};
}

Json SourceToJSON(const SourceConfig &source)
{
    return std::visit(
        [](const auto &v) -> Json {
            using T = std::decay_t<decltype(v)>;
            Json json = Json::object();
            if constexpr (std::is_same_v<T, WaveformConfig> || std::is_same_v<T, AnalogConfig>)
            {
                json = WaveformFieldsToJSON(v);
                if constexpr (std::is_same_v<T, AnalogConfig>)
                {
                    json["driftDepthCents"] = v.driftDepthCents;
                    json["driftRateHz"] = v.driftRateHz;
                }
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
                json = {{"noise", NoiseTypeToString(v.noise)}, {"filter", FilterToJSON(v)}};
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                Json operators = Json::array();
                for (const auto &op : v.ops)
                    operators.push_back(Json{{"ratio", op.ratio},
                                             {"level", op.level},
                                             {"index", op.index},
                                             {"wave", WaveTypeToString(op.wave)},
                                             {"levelEnv", EnvelopeToJSON(op.levelEnv)},
                                             {"indexEnv", EnvelopeToJSON(op.indexEnv)}});
                json = {{"chip", v.chip},
                        {"algorithm", v.algorithm},
                        {"feedback", v.feedback},
                        {"brightness", v.brightness},
                        {"drive", v.drive},
                        {"ops", std::move(operators)},
                        {"filter", FilterToJSON(v)},
                        {"modulation", ModulationToJSON(v.modulation)}};
            }
            else if constexpr (std::is_same_v<T, PsgConfig>)
                json = {{"duty", v.duty},
                        {"volumeSteps", v.volumeSteps},
                        {"maxVoices", v.maxVoices},
                        {"wave", PsgWaveTypeToString(v.wave)}};
            else if constexpr (std::is_same_v<T, DrumKitConfig>)
            {
                const auto &bus = v.drumBus;
                if (bus.enabled || bus.level != 1 || bus.attackTrim != 0 || bus.sustainLift != 0 || bus.glue != 0 ||
                    bus.presenceCut != 0 || bus.lowTighten != 0 || bus.roomSend != 0 || bus.driveTrim != 0)
                    json["drumBus"] = DrumBusToJSON(bus);
                if (v.velocityCeiling != 1 || v.velocityCurve != 1)
                {
                    json["velocityCeiling"] = v.velocityCeiling;
                    json["velocityCurve"] = v.velocityCurve;
                }
                json["map"] = Json::object();
                for (size_t note = 0; note < v.map.size(); ++note)
                    if (v.map[note].type != DrumType::None)
                        json["map"][std::to_string(note)] = DrumToJSON(v.map[note]);
            }
            // Standalone DrumConfig has never been a supported saved source type.
            if constexpr (!std::is_same_v<T, DrumConfig>)
                json["type"] = SourceKindToTypeName(SourceConfigKind(v));
            return json;
        },
        source);
}
} // namespace

nlohmann::json SoundToJSON(const InstrumentSoundConfig &cfg)
{
    using namespace internal;
    Json layers = Json::object();
    {
        const auto &layer = cfg.attackLayer;
        layers["attack"] = {{"enabled", layer.enabled},   {"level", layer.level},
                            {"decaySec", layer.decaySec}, {"brightness", layer.brightness},
                            {"bodyMix", layer.bodyMix},   {"pitchOffsetSemis", layer.pitchOffsetSemis},
                            {"drive", layer.drive},       {"type", AttackLayerTypeToString(layer.type)}};
    }
    {
        const auto &layer = cfg.bassLayer;
        layers["bass"] = {{"enabled", layer.enabled},
                          {"level", layer.level},
                          {"subLevel", layer.subLevel},
                          {"bodyLevel", layer.bodyLevel},
                          {"gritLevel", layer.gritLevel},
                          {"cutoffHz", layer.cutoffHz},
                          {"drive", layer.drive},
                          {"pitchOffsetSemis", layer.pitchOffsetSemis},
                          {"velocityToDrive", layer.velocityToDrive},
                          {"focusHz", layer.focusHz},
                          {"focusLevel", layer.focusLevel},
                          {"bodySaturation", layer.bodySaturation},
                          {"gritTone", layer.gritTone},
                          {"attackBoost", layer.attackBoost},
                          {"attackDecaySec", layer.attackDecaySec},
                          {"type", BassLayerTypeToString(layer.type)}};
    }
    {
        const auto &layer = cfg.leadLayer;
        layers["lead"] = {{"enabled", layer.enabled},
                          {"level", layer.level},
                          {"edgeLevel", layer.edgeLevel},
                          {"bodyLevel", layer.bodyLevel},
                          {"detuneCents", layer.detuneCents},
                          {"pitchBendSemis", layer.pitchBendSemis},
                          {"bendDecaySec", layer.bendDecaySec},
                          {"attackBoost", layer.attackBoost},
                          {"attackDecaySec", layer.attackDecaySec},
                          {"drive", layer.drive},
                          {"characterLevel", layer.characterLevel},
                          {"characterTone", layer.characterTone},
                          {"biteLevel", layer.biteLevel},
                          {"biteDecaySec", layer.biteDecaySec},
                          {"wobbleDepthCents", layer.wobbleDepthCents},
                          {"wobbleRateHz", layer.wobbleRateHz},
                          {"type", LeadLayerTypeToString(layer.type)}};
    }
    {
        const auto &layer = cfg.chordLayer;
        layers["chord"] = {{"enabled", layer.enabled},
                           {"level", layer.level},
                           {"intervalsSemis", layer.intervalsSemis},
                           {"voiceLevels", layer.voiceLevels},
                           {"detuneCents", layer.detuneCents},
                           {"spread", layer.spread},
                           {"cutoffHz", layer.cutoffHz},
                           {"drive", layer.drive}};
    }
    {
        const auto &layer = cfg.padLayer;
        layers["pad"] = {{"enabled", layer.enabled},
                         {"level", layer.level},
                         {"octaveLevel", layer.octaveLevel},
                         {"detuneCents", layer.detuneCents},
                         {"spread", layer.spread},
                         {"fadeInSec", layer.fadeInSec},
                         {"brightness", layer.brightness},
                         {"motionDepth", layer.motionDepth},
                         {"motionRateHz", layer.motionRateHz},
                         {"cutoffHz", layer.cutoffHz},
                         {"drive", layer.drive}};
    }
    {
        const auto &layer = cfg.pluckLayer;
        layers["pluck"] = {{"enabled", layer.enabled},   {"level", layer.level},
                           {"decaySec", layer.decaySec}, {"brightness", layer.brightness},
                           {"noiseMix", layer.noiseMix}, {"pitchOffsetSemis", layer.pitchOffsetSemis},
                           {"bodySend", layer.bodySend}, {"drive", layer.drive}};
    }
    {
        const auto &layer = cfg.stringLayer;
        layers["string"] = {{"enabled", layer.enabled},
                            {"level", layer.level},
                            {"bowLevel", layer.bowLevel},
                            {"detuneCents", layer.detuneCents},
                            {"spread", layer.spread},
                            {"fadeInSec", layer.fadeInSec},
                            {"brightness", layer.brightness},
                            {"motionDepth", layer.motionDepth},
                            {"motionRateHz", layer.motionRateHz},
                            {"bodySend", layer.bodySend},
                            {"drive", layer.drive}};
    }
    {
        const auto &layer = cfg.bodyLayer;
        layers["body"] = {{"enabled", layer.enabled}, {"mix", layer.mix},
                          {"size", layer.size},       {"tone", layer.tone},
                          {"damping", layer.damping}, {"stereo", layer.stereo},
                          {"drive", layer.drive},     {"mode", BodyLayerModeToString(layer.mode)}};
    }
    {
        const auto &layer = cfg.harmonicLayer;
        layers["harmonic"] = {
            {"enabled", layer.enabled},         {"level", layer.level},       {"harmonicLevels", layer.harmonicLevels},
            {"brightness", layer.brightness},   {"keyClick", layer.keyClick}, {"attackSec", layer.attackSec},
            {"releaseDamp", layer.releaseDamp}, {"drive", layer.drive},       {"stereo", layer.stereo}};
    }
    {
        const auto &layer = cfg.powerChordLayer;
        layers["powerChord"] = {{"enabled", layer.enabled},
                                {"level", layer.level},
                                {"fifthLevel", layer.fifthLevel},
                                {"octaveLevel", layer.octaveLevel},
                                {"detuneCents", layer.detuneCents},
                                {"spread", layer.spread},
                                {"tone", layer.tone},
                                {"drive", layer.drive}};
    }
    {
        const auto &layer = cfg.chugLayer;
        layers["chug"] = {{"enabled", layer.enabled},     {"level", layer.level}, {"decaySec", layer.decaySec},
                          {"lowPunch", layer.lowPunch},   {"pick", layer.pick},   {"tone", layer.tone},
                          {"tightness", layer.tightness}, {"drive", layer.drive}};
    }
    {
        const auto &layer = cfg.ampCabLayer;
        layers["ampCab"] = {{"enabled", layer.enabled}, {"drive", layer.drive},     {"tone", layer.tone},
                            {"cabLow", layer.cabLow},   {"cabHigh", layer.cabHigh}, {"presence", layer.presence},
                            {"output", layer.output}};
    }
    const auto &map = cfg.expressionMap;
    Json result{{"amp", cfg.amp},
                {"attackSec", cfg.attackSec},
                {"decaySec", cfg.decaySec},
                {"sustainLevel", cfg.sustainLevel},
                {"releaseSec", cfg.releaseSec},
                {"portamentoTimeSec", cfg.portamentoTimeSec},
                {"layers", std::move(layers)},
                {"source", SourceToJSON(cfg.source)},
                {"expressionMap", Json{{"enabled", map.enabled},
                                       {"velocityCurve", map.velocityCurve},
                                       {"velocityToAmp", map.velocityToAmp},
                                       {"velocityToBrightness", map.velocityToBrightness},
                                       {"velocityToFmIndex", map.velocityToFmIndex},
                                       {"velocityToAttack", map.velocityToAttack},
                                       {"velocityToBass", map.velocityToBass},
                                       {"velocityToLead", map.velocityToLead},
                                       {"velocityToChord", map.velocityToChord},
                                       {"velocityToPad", map.velocityToPad},
                                       {"velocityToPluck", map.velocityToPluck},
                                       {"velocityToString", map.velocityToString},
                                       {"velocityToBody", map.velocityToBody},
                                       {"modWheelToBrightness", map.modWheelToBrightness},
                                       {"modWheelToPad", map.modWheelToPad},
                                       {"modWheelToString", map.modWheelToString},
                                       {"pressureToDrive", map.pressureToDrive},
                                       {"pressureToFilterDrive", map.pressureToFilterDrive},
                                       {"cc74ToBrightness", map.cc74ToBrightness},
                                       {"cc74ToPadBrightness", map.cc74ToPadBrightness},
                                       {"cc74ToStringBrightness", map.cc74ToStringBrightness}}}};
    CheckFinite(result);
    return result;
}
} // namespace config
