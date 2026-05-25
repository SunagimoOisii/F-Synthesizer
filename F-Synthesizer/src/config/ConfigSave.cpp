#include "ConfigFileInternal.h"

#include <fstream>
#include <sstream>

#include "io/PlatformPaths.h"
#include "third_party/nlohmann/json.hpp"

namespace config::internal
{
namespace
{
using Json = nlohmann::json;

Json ChannelSoundToJson(const InstrumentSoundConfig& config)
{
    std::ostringstream tmp;
    WriteInstrumentSoundConfig(tmp, 0, config, false);
    Json wrapped = Json::parse("{" + tmp.str() + "}", nullptr, false);
    if (wrapped.is_discarded() || !wrapped.contains("0") || !wrapped["0"].is_object())
    {
        return Json::object();
    }
    return wrapped["0"];
}

Json InstrumentToJson(const InstrumentConfig& instrument)
{
    Json macroHints = Json::array();
    for (const MacroHint& hint : instrument.macroHints)
    {
        macroHints.push_back(Json{
            {"id", hint.id},
            {"label", hint.label},
            {"description", hint.description},
        });
    }

    return Json{
        {"displayName", instrument.displayName},
        {"category", instrument.category},
        {"internal", instrument.internal},
        {"tags", instrument.tags},
        {"description", instrument.description},
        {"recommendedRange", Json{
            {"low", instrument.recommendedRange.low},
            {"high", instrument.recommendedRange.high},
            {"preview", instrument.recommendedRange.preview},
        }},
        {"macroHints", std::move(macroHints)},
        {"sound", ChannelSoundToJson(instrument.sound)},
    };
}

Json ProjectChannelToJson(const ProjectChannelAssignment& channel)
{
    return Json{
        {"instrumentId", channel.instrumentId},
        {"mix", Json{
            {"mute", channel.mix.mute},
            {"solo", channel.mix.solo},
            {"level", channel.mix.level},
            {"pan", channel.mix.pan},
            {"gain", channel.mix.gain},
        }},
    };
}

Json MasterEffectsToJson(const MasterEffectConfig& effects)
{
    return Json{
        {"reverb", Json{
            {"enabled", effects.reverb.enabled},
            {"mix", effects.reverb.mix},
            {"roomSize", effects.reverb.roomSize},
            {"damping", effects.reverb.damping},
        }},
        {"delay", Json{
            {"enabled", effects.delay.enabled},
            {"mix", effects.delay.mix},
            {"timeSec", effects.delay.timeSec},
            {"feedback", effects.delay.feedback},
            {"tempoSync", effects.delay.tempoSync},
            {"syncBeats", effects.delay.syncBeats},
        }},
        {"chorus", Json{
            {"enabled", effects.chorus.enabled},
            {"mix", effects.chorus.mix},
            {"baseDelayMs", effects.chorus.baseDelayMs},
            {"depthMs", effects.chorus.depthMs},
            {"rateHz", effects.chorus.rateHz},
            {"feedback", effects.chorus.feedback},
        }},
        {"flanger", Json{
            {"enabled", effects.flanger.enabled},
            {"mix", effects.flanger.mix},
            {"baseDelayMs", effects.flanger.baseDelayMs},
            {"depthMs", effects.flanger.depthMs},
            {"rateHz", effects.flanger.rateHz},
            {"feedback", effects.flanger.feedback},
        }},
        {"bitCrusher", Json{
            {"bits", effects.bitCrusher.bits},
        }},
        {"sampleRateReducer", Json{
            {"ratio", effects.sampleRateReducer.ratio},
        }},
    };
}
} // namespace

bool SaveProjectModelFileInternal(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err)
{
    std::error_code ec;
    if (configPath.has_parent_path())
    {
        std::filesystem::create_directories(configPath.parent_path(), ec);
    }

    const ProjectModel saveModel = model;

    Json instruments = Json::object();
    if (saveModel.instruments)
    {
        for (const auto& [id, instrument] : *saveModel.instruments)
        {
            instruments[id] = InstrumentToJson(instrument);
        }
    }

    Json channels = Json::object();
    if (saveModel.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const ProjectChannelAssignment& channel = (*saveModel.projectChannels)[ch];
            if (!channel.enabled)
            {
                continue;
            }
            channels[std::to_string(ch)] = ProjectChannelToJson(channel);
        }
    }

    Json root = Json{
        {"format", "projectModel.v3"},
        {"project", Json{
            {"midiPath", PathToUtf8(saveModel.midiPath)},
            {"wavPath", PathToUtf8(saveModel.wavPath)},
            {"targetChannel", saveModel.targetChannel},
            {"initialSeconds", saveModel.initialSeconds},
            {"bits", saveModel.bits},
            {"sampleRate", saveModel.sampleRate},
            {"extraReleaseSec", saveModel.extraReleaseSec},
            {"instruments", std::move(instruments)},
            {"channels", std::move(channels)},
            {"effects", MasterEffectsToJson(saveModel.masterEffects)},
        }},
    };

    std::ofstream out(configPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open output file";
        return false;
    }

    out << root.dump(2) << '\n';

    return true;
}

} // namespace config::internal
