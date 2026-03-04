#include "ConfigFileInternal.h"

#include <fstream>

#include "io/PlatformPaths.h"

namespace config::internal
{
bool SaveConfigFileInternal(const std::filesystem::path& configPath, const AppConfig& config, std::string& err)
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

    out << "{\n";
    out << "  \"midiPath\": \"" << EscapeJSON(PathToUtf8(config.midiPath)) << "\",\n";
    out << "  \"wavPath\": \"" << EscapeJSON(PathToUtf8(config.wavPath)) << "\",\n";
    out << "  \"targetChannel\": " << config.targetChannel << ",\n";
    out << "  \"defaultWave\": \"" << WaveTypeToString(config.defaultWave) << "\",\n";
    out << "  \"initialSeconds\": " << config.initialSeconds << ",\n";
    out << "  \"bits\": " << config.bits << ",\n";
    out << "  \"sampleRate\": " << config.sampleRate << ",\n";
    out << "  \"extraReleaseSec\": " << config.extraReleaseSec << ",\n";
    out << "  \"channels\": {\n";

    // 出力時に常に16ch/16mixを書き出し、ロード側の差分適用と組み合わせて再現性を優先する。
    AppConfig base = DefaultConfig();
    const auto& channels = config.channelConfigs ? *config.channelConfigs : *base.channelConfigs;
    for (int ch = 0; ch < 16; ch++)
    {
        WriteChannelConfig(out, ch, channels[ch], ch != 15);
    }

    out << "  },\n";
    out << "  \"channelMix\": {\n";
    const auto& channelMix = config.channelMixStates ? *config.channelMixStates : *base.channelMixStates;
    for (int ch = 0; ch < 16; ch++)
    {
        WriteChannelMixState(out, ch, channelMix[ch], ch != 15);
    }
    out << "  }\n";
    out << "}\n";

    return true;
}
} // namespace config::internal
