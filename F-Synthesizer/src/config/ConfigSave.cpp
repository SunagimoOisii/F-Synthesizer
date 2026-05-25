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

std::string DumpChannelSound(const ChannelConfig& config)
{
    std::ostringstream tmp;
    WriteChannelConfig(tmp, 0, config, false);
    Json wrapped = Json::parse("{" + tmp.str() + "}", nullptr, false);
    if (wrapped.is_discarded() || !wrapped.contains("0"))
    {
        return "{}";
    }
    return wrapped["0"].dump(6);
}

void WriteStringField(std::ostream& out, const char* key, const std::string& value, int indent, bool comma)
{
    WriteIndent(out, indent);
    out << "\"" << key << "\": \"" << EscapeJSON(value) << "\"";
    if (comma)
    {
        out << ",";
    }
    out << "\n";
}

void WriteInstrument(std::ostream& out, const std::string& id, const InstrumentConfig& instrument, bool withComma)
{
    WriteIndent(out, 6); out << "\"" << EscapeJSON(id) << "\": {\n";
    WriteStringField(out, "displayName", instrument.displayName, 8, true);
    WriteStringField(out, "category", instrument.category, 8, true);
    WriteIndent(out, 8); out << "\"internal\": " << (instrument.internal ? "true" : "false") << ",\n";
    WriteIndent(out, 8); out << "\"tags\": [";
    for (size_t i = 0; i < instrument.tags.size(); i++)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "\"" << EscapeJSON(instrument.tags[i]) << "\"";
    }
    out << "],\n";
    WriteStringField(out, "description", instrument.description, 8, true);
    WriteIndent(out, 8); out << "\"sound\": " << DumpChannelSound(instrument.sound) << "\n";
    WriteIndent(out, 6); out << "}";
    if (withComma)
    {
        out << ",";
    }
    out << "\n";
}

void WriteProjectChannel(std::ostream& out, int ch, const ProjectChannelConfig& channel, bool withComma)
{
    WriteIndent(out, 6); out << "\"" << ch << "\": {\n";
    WriteStringField(out, "instrumentId", channel.instrumentId, 8, true);
    WriteIndent(out, 8); out << "\"mix\": {\n";
    WriteIndent(out, 10); out << "\"mute\": " << (channel.mix.mute ? "true" : "false") << ",\n";
    WriteIndent(out, 10); out << "\"solo\": " << (channel.mix.solo ? "true" : "false") << ",\n";
    WriteIndent(out, 10); out << "\"level\": " << channel.mix.level << ",\n";
    WriteIndent(out, 10); out << "\"pan\": " << channel.mix.pan << ",\n";
    WriteIndent(out, 10); out << "\"gain\": " << channel.mix.gain << "\n";
    WriteIndent(out, 8); out << "}\n";
    WriteIndent(out, 6); out << "}";
    if (withComma)
    {
        out << ",";
    }
    out << "\n";
}
} // namespace

bool SaveProjectModelFileInternal(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err)
{
    std::error_code ec;
    if (configPath.has_parent_path())
    {
        std::filesystem::create_directories(configPath.parent_path(), ec);
    }

    std::ofstream out(configPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open output file";
        return false;
    }

    const ProjectModel saveModel = (model.instruments && model.projectChannels)
        ? model
        : ProjectModelFromAppConfig(ToAppConfig(model));
    const AppConfig config = ToAppConfig(saveModel);

    out << "{\n";
    out << "  \"format\": \"projectModel.v3\",\n";
    out << "  \"project\": {\n";
    out << "    \"midiPath\": \"" << EscapeJSON(PathToUtf8(config.midiPath)) << "\",\n";
    out << "    \"wavPath\": \"" << EscapeJSON(PathToUtf8(config.wavPath)) << "\",\n";
    out << "    \"targetChannel\": " << config.targetChannel << ",\n";
    out << "    \"initialSeconds\": " << config.initialSeconds << ",\n";
    out << "    \"bits\": " << config.bits << ",\n";
    out << "    \"sampleRate\": " << config.sampleRate << ",\n";
    out << "    \"extraReleaseSec\": " << config.extraReleaseSec << ",\n";
    out << "    \"instruments\": {\n";
    bool firstInstrument = true;
    if (saveModel.instruments)
    {
        for (const auto& [id, instrument] : *saveModel.instruments)
        {
            if (!firstInstrument)
            {
                out << ",\n";
            }
            firstInstrument = false;
            WriteInstrument(out, id, instrument, false);
        }
    }
    out << "    },\n";
    out << "    \"channels\": {\n";
    bool firstChannel = true;
    if (saveModel.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const ProjectChannelConfig& channel = (*saveModel.projectChannels)[ch];
            if (!channel.enabled)
            {
                continue;
            }
            if (!firstChannel)
            {
                out << ",\n";
            }
            firstChannel = false;
            WriteProjectChannel(out, ch, channel, false);
        }
    }
    out << "    },\n";
    out << "    \"effects\": {\n";
    out << "      \"reverb\": {\n";
    out << "        \"enabled\": " << (config.masterEffects.reverb.enabled ? "true" : "false") << ",\n";
    out << "        \"mix\": " << config.masterEffects.reverb.mix << ",\n";
    out << "        \"roomSize\": " << config.masterEffects.reverb.roomSize << ",\n";
    out << "        \"damping\": " << config.masterEffects.reverb.damping << "\n";
    out << "      },\n";
    out << "      \"delay\": {\n";
    out << "        \"enabled\": " << (config.masterEffects.delay.enabled ? "true" : "false") << ",\n";
    out << "        \"mix\": " << config.masterEffects.delay.mix << ",\n";
    out << "        \"timeSec\": " << config.masterEffects.delay.timeSec << ",\n";
    out << "        \"feedback\": " << config.masterEffects.delay.feedback << ",\n";
    out << "        \"tempoSync\": " << (config.masterEffects.delay.tempoSync ? "true" : "false") << ",\n";
    out << "        \"syncBeats\": " << config.masterEffects.delay.syncBeats << "\n";
    out << "      },\n";
    out << "      \"chorus\": {\n";
    out << "        \"enabled\": " << (config.masterEffects.chorus.enabled ? "true" : "false") << ",\n";
    out << "        \"mix\": " << config.masterEffects.chorus.mix << ",\n";
    out << "        \"baseDelayMs\": " << config.masterEffects.chorus.baseDelayMs << ",\n";
    out << "        \"depthMs\": " << config.masterEffects.chorus.depthMs << ",\n";
    out << "        \"rateHz\": " << config.masterEffects.chorus.rateHz << ",\n";
    out << "        \"feedback\": " << config.masterEffects.chorus.feedback << "\n";
    out << "      },\n";
    out << "      \"flanger\": {\n";
    out << "        \"enabled\": " << (config.masterEffects.flanger.enabled ? "true" : "false") << ",\n";
    out << "        \"mix\": " << config.masterEffects.flanger.mix << ",\n";
    out << "        \"baseDelayMs\": " << config.masterEffects.flanger.baseDelayMs << ",\n";
    out << "        \"depthMs\": " << config.masterEffects.flanger.depthMs << ",\n";
    out << "        \"rateHz\": " << config.masterEffects.flanger.rateHz << ",\n";
    out << "        \"feedback\": " << config.masterEffects.flanger.feedback << "\n";
    out << "      },\n";
    out << "      \"bitCrusher\": {\n";
    out << "        \"bits\": " << config.masterEffects.bitCrusher.bits << "\n";
    out << "      },\n";
    out << "      \"sampleRateReducer\": {\n";
    out << "        \"ratio\": " << config.masterEffects.sampleRateReducer.ratio << "\n";
    out << "      }\n";
    out << "    }\n";
    out << "  }\n";
    out << "}\n";

    return true;
}

bool SaveConfigFileInternal(const std::filesystem::path& configPath, const AppConfig& config, std::string& err)
{
    const ProjectModel model = ProjectModelFromAppConfig(config);
    return SaveProjectModelFileInternal(configPath, model, err);
}
} // namespace config::internal
