#include "gui/GUIStateStorage.h"

#include <fstream>
#include <optional>
#include <regex>
#include <sstream>

namespace
{
std::string EscapeJson(const std::string& src)
{
    std::string out;
    out.reserve(src.size() + 16);
    for (char c : src)
    {
        if (c == '\\')
        {
            out += "\\\\";
        }
        else if (c == '"')
        {
            out += "\\\"";
        }
        else if (c == '\n')
        {
            out += "\\n";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

std::optional<std::string> ReadJsonString(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        const std::string raw = m[1].str();
        std::string out;
        out.reserve(raw.size());
        for (size_t i = 0; i < raw.size(); i++)
        {
            const char c = raw[i];
            if (c == '\\' && i + 1 < raw.size())
            {
                const char n = raw[i + 1];
                if (n == '\\')
                {
                    out.push_back('\\');
                    i++;
                    continue;
                }
                if (n == '"')
                {
                    out.push_back('"');
                    i++;
                    continue;
                }
                if (n == 'n')
                {
                    out.push_back('\n');
                    i++;
                    continue;
                }
            }
            out.push_back(c);
        }
        return out;
    }
    return std::nullopt;
}

std::optional<int> ReadJsonInt(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stoi(m[1].str());
    }
    return std::nullopt;
}

std::optional<float> ReadJsonFloat(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stof(m[1].str());
    }
    return std::nullopt;
}

std::optional<bool> ReadJsonBool(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return m[1].str() == "true";
    }
    return std::nullopt;
}
} // namespace

bool LoadGUIStateStorageFile(const std::filesystem::path& path, GUIStateStorageData& data, std::string& err)
{
    if (!std::filesystem::exists(path))
    {
        // 初回起動はファイル未存在を正常系として扱う。
        return true;
    }

    std::ifstream fin(path, std::ios::binary);
    if (!fin)
    {
        err = "failed to open " + path.string();
        return false;
    }

    std::ostringstream oss;
    oss << fin.rdbuf();
    const std::string text = oss.str();

    if (auto v = ReadJsonString(text, "midiPath")) data.midiPath = *v;
    if (auto v = ReadJsonString(text, "wavPath")) data.wavPath = *v;
    if (auto v = ReadJsonInt(text, "targetChannel")) data.targetChannel = *v;
    if (auto v = ReadJsonInt(text, "sampleRate")) data.sampleRate = *v;
    if (auto v = ReadJsonInt(text, "initialSeconds")) data.initialSeconds = *v;
    if (auto v = ReadJsonInt(text, "bits")) data.bits = *v;
    if (auto v = ReadJsonFloat(text, "extraReleaseSec")) data.extraReleaseSec = *v;
    if (auto v = ReadJsonInt(text, "defaultWave")) data.defaultWave = *v;
    if (auto v = ReadJsonInt(text, "uiScaleIndex")) data.uiScaleIndex = *v;
    if (auto v = ReadJsonFloat(text, "logPanelHeight")) data.logPanelHeight = *v;
    if (auto v = ReadJsonInt(text, "presetIndex")) data.presetIndex = *v;
    if (auto v = ReadJsonBool(text, "serialSave")) data.serialSave = *v;
    if (auto v = ReadJsonBool(text, "previewLoop")) data.previewLoop = *v;
    if (auto v = ReadJsonInt(text, "selectedChannel")) data.selectedChannel = *v;
    if (auto v = ReadJsonInt(text, "selectedDrumNote")) data.selectedDrumNote = *v;
    if (auto v = ReadJsonString(text, "presetName")) data.presetName = *v;
    if (auto v = ReadJsonString(text, "lastPresetPath")) data.lastPresetPath = *v;

    for (int ch = 0; ch < 16; ch++)
    {
        // mix状態はキーごとに保持し、部分更新時も既存値を保持する。
        ChannelMixState& mix = data.channelMixStates[ch];
        const std::string kMute = "mixCh" + std::to_string(ch) + "Mute";
        const std::string kSolo = "mixCh" + std::to_string(ch) + "Solo";
        const std::string kLevel = "mixCh" + std::to_string(ch) + "Level";
        const std::string kPan = "mixCh" + std::to_string(ch) + "Pan";
        const std::string kGain = "mixCh" + std::to_string(ch) + "Gain";
        if (auto v = ReadJsonBool(text, kMute)) mix.mute = *v;
        if (auto v = ReadJsonBool(text, kSolo)) mix.solo = *v;
        if (auto v = ReadJsonFloat(text, kLevel)) mix.level = *v;
        if (auto v = ReadJsonFloat(text, kPan)) mix.pan = *v;
        if (auto v = ReadJsonFloat(text, kGain)) mix.gain = *v;
    }

    return true;
}

bool SaveGUIStateStorageFile(const std::filesystem::path& path, const GUIStateStorageData& data, std::string& err)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream fout(path, std::ios::binary | std::ios::trunc);
    if (!fout)
    {
        err = "failed to open " + path.string();
        return false;
    }

    // 読込側のregex実装と対応させるため、入れ子を使わないJSONキー構造で保存する。
    fout << "{\n";
    fout << "  \"midiPath\": \"" << EscapeJson(data.midiPath) << "\",\n";
    fout << "  \"wavPath\": \"" << EscapeJson(data.wavPath) << "\",\n";
    fout << "  \"targetChannel\": " << data.targetChannel << ",\n";
    fout << "  \"sampleRate\": " << data.sampleRate << ",\n";
    fout << "  \"initialSeconds\": " << data.initialSeconds << ",\n";
    fout << "  \"bits\": " << data.bits << ",\n";
    fout << "  \"extraReleaseSec\": " << data.extraReleaseSec << ",\n";
    fout << "  \"defaultWave\": " << data.defaultWave << ",\n";
    fout << "  \"uiScaleIndex\": " << data.uiScaleIndex << ",\n";
    fout << "  \"logPanelHeight\": " << data.logPanelHeight << ",\n";
    fout << "  \"presetIndex\": " << data.presetIndex << ",\n";
    fout << "  \"serialSave\": " << (data.serialSave ? "true" : "false") << ",\n";
    fout << "  \"previewLoop\": " << (data.previewLoop ? "true" : "false") << ",\n";
    fout << "  \"selectedChannel\": " << data.selectedChannel << ",\n";
    fout << "  \"selectedDrumNote\": " << data.selectedDrumNote << ",\n";
    fout << "  \"presetName\": \"" << EscapeJson(data.presetName) << "\",\n";
    fout << "  \"lastPresetPath\": \"" << EscapeJson(data.lastPresetPath) << "\",\n";
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelMixState& mix = data.channelMixStates[ch];
        fout << "  \"mixCh" << ch << "Mute\": " << (mix.mute ? "true" : "false") << ",\n";
        fout << "  \"mixCh" << ch << "Solo\": " << (mix.solo ? "true" : "false") << ",\n";
        fout << "  \"mixCh" << ch << "Level\": " << mix.level << ",\n";
        fout << "  \"mixCh" << ch << "Pan\": " << mix.pan << ",\n";
        fout << "  \"mixCh" << ch << "Gain\": " << mix.gain;
        fout << (ch == 15 ? "\n" : ",\n");
    }
    fout << "}\n";

    return true;
}
