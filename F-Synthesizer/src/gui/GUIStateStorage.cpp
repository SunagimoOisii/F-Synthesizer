#include "gui/GUIStateStorage.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace
{
std::string EscapeJSON(const std::string& src)
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

std::optional<std::string> ReadJSONString(const std::string& text, const std::string& key)
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

std::optional<int> ReadJSONInt(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stoi(m[1].str());
    }
    return std::nullopt;
}

std::optional<float> ReadJSONFloat(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stof(m[1].str());
    }
    return std::nullopt;
}

std::optional<bool> ReadJSONBool(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return m[1].str() == "true";
    }
    return std::nullopt;
}

std::string Trim(const std::string& s)
{
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])) != 0)
    {
        b++;
    }
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0)
    {
        e--;
    }
    return s.substr(b, e - b);
}

std::string UnescapeJSONString(const std::string& raw)
{
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

bool ParseFlatJSONLine(const std::string& line, std::string& outKey, std::string& outValue)
{
    const size_t k0 = line.find('"');
    if (k0 == std::string::npos)
    {
        return false;
    }
    const size_t k1 = line.find('"', k0 + 1);
    if (k1 == std::string::npos || k1 <= k0 + 1)
    {
        return false;
    }
    const size_t colon = line.find(':', k1 + 1);
    if (colon == std::string::npos)
    {
        return false;
    }

    outKey = line.substr(k0 + 1, k1 - k0 - 1);
    outValue = Trim(line.substr(colon + 1));
    if (!outValue.empty() && outValue.back() == ',')
    {
        outValue.pop_back();
        outValue = Trim(outValue);
    }
    return true;
}

std::optional<int> ParseIntValue(const std::string& value)
{
    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> ParseStringValue(const std::string& value)
{
    if (value.size() < 2 || value.front() != '"' || value.back() != '"')
    {
        return std::nullopt;
    }
    return UnescapeJSONString(value.substr(1, value.size() - 2));
}

bool StartsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
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

    if (auto v = ReadJSONString(text, "midiPath")) data.midiPath = *v;
    if (auto v = ReadJSONString(text, "wavPath")) data.wavPath = *v;
    if (auto v = ReadJSONInt(text, "targetChannel")) data.targetChannel = *v;
    if (auto v = ReadJSONInt(text, "sampleRate")) data.sampleRate = *v;
    if (auto v = ReadJSONInt(text, "initialSeconds")) data.initialSeconds = *v;
    if (auto v = ReadJSONInt(text, "bits")) data.bits = *v;
    if (auto v = ReadJSONFloat(text, "extraReleaseSec")) data.extraReleaseSec = *v;
    if (auto v = ReadJSONInt(text, "fxBitCrusherBits")) data.fxBitCrusherBits = std::clamp(*v, 1, 16);
    if (auto v = ReadJSONFloat(text, "fxSampleRateReducerRatio")) data.fxSampleRateReducerRatio = std::clamp(*v, 0.0f, 1.0f);
    if (auto v = ReadJSONBool(text, "fxChorusEnabled")) data.fxChorusEnabled = *v;
    if (auto v = ReadJSONFloat(text, "fxChorusMix")) data.fxChorusMix = std::clamp(*v, 0.0f, 1.0f);
    if (auto v = ReadJSONFloat(text, "fxChorusBaseDelayMs")) data.fxChorusBaseDelayMs = std::clamp(*v, 2.0f, 40.0f);
    if (auto v = ReadJSONFloat(text, "fxChorusDepthMs")) data.fxChorusDepthMs = std::clamp(*v, 0.0f, 20.0f);
    if (auto v = ReadJSONFloat(text, "fxChorusRateHz")) data.fxChorusRateHz = std::clamp(*v, 0.05f, 8.0f);
    if (auto v = ReadJSONFloat(text, "fxChorusFeedback")) data.fxChorusFeedback = std::clamp(*v, 0.0f, 0.9f);
    if (auto v = ReadJSONBool(text, "fxFlangerEnabled")) data.fxFlangerEnabled = *v;
    if (auto v = ReadJSONFloat(text, "fxFlangerMix")) data.fxFlangerMix = std::clamp(*v, 0.0f, 1.0f);
    if (auto v = ReadJSONFloat(text, "fxFlangerBaseDelayMs")) data.fxFlangerBaseDelayMs = std::clamp(*v, 0.1f, 8.0f);
    if (auto v = ReadJSONFloat(text, "fxFlangerDepthMs")) data.fxFlangerDepthMs = std::clamp(*v, 0.0f, 5.0f);
    if (auto v = ReadJSONFloat(text, "fxFlangerRateHz")) data.fxFlangerRateHz = std::clamp(*v, 0.05f, 8.0f);
    if (auto v = ReadJSONFloat(text, "fxFlangerFeedback")) data.fxFlangerFeedback = std::clamp(*v, 0.0f, 0.95f);
    if (auto v = ReadJSONBool(text, "fxDelayEnabled")) data.fxDelayEnabled = *v;
    if (auto v = ReadJSONFloat(text, "fxDelayMix")) data.fxDelayMix = std::clamp(*v, 0.0f, 1.0f);
    if (auto v = ReadJSONFloat(text, "fxDelayTimeSec")) data.fxDelayTimeSec = std::clamp(*v, 0.01f, 2.0f);
    if (auto v = ReadJSONFloat(text, "fxDelayFeedback")) data.fxDelayFeedback = std::clamp(*v, 0.0f, 0.95f);
    if (auto v = ReadJSONBool(text, "fxDelayTempoSync")) data.fxDelayTempoSync = *v;
    if (auto v = ReadJSONFloat(text, "fxDelaySyncBeats")) data.fxDelaySyncBeats = std::clamp(*v, 0.125f, 4.0f);
    if (auto v = ReadJSONBool(text, "fxReverbEnabled")) data.fxReverbEnabled = *v;
    if (auto v = ReadJSONFloat(text, "fxReverbMix")) data.fxReverbMix = std::clamp(*v, 0.0f, 1.0f);
    if (auto v = ReadJSONFloat(text, "fxReverbRoomSize")) data.fxReverbRoomSize = std::clamp(*v, 0.1f, 1.0f);
    if (auto v = ReadJSONFloat(text, "fxReverbDamping")) data.fxReverbDamping = std::clamp(*v, 0.0f, 1.0f);
    if (auto v = ReadJSONInt(text, "UIScaleIndex")) data.UIScaleIndex = *v;
    else if (auto v = ReadJSONInt(text, "uiScaleIndex")) data.UIScaleIndex = *v;
    if (auto v = ReadJSONInt(text, "UIModeTab")) data.UIModeTab = *v;
    else if (auto v = ReadJSONInt(text, "uiModeTab")) data.UIModeTab = *v;
    if (auto v = ReadJSONFloat(text, "logPanelHeight")) data.logPanelHeight = *v;
    if (auto v = ReadJSONInt(text, "presetIndex")) data.presetIndex = *v;
    if (auto v = ReadJSONBool(text, "serialSave")) data.serialSave = *v;
    if (auto v = ReadJSONBool(text, "previewLoop")) data.previewLoop = *v;
    if (auto v = ReadJSONBool(text, "autoTonePreviewEnabled")) data.autoTonePreviewEnabled = *v;
    if (auto v = ReadJSONInt(text, "selectedSoundSlot")) data.selectedSoundSlot = *v;
    if (auto v = ReadJSONInt(text, "selectedDrumNote")) data.selectedDrumNote = *v;
    if (auto v = ReadJSONInt(text, "tonePreviewNoteNumber")) data.tonePreviewNoteNumber = *v;
    if (auto v = ReadJSONBool(text, "chordModeEnabled")) data.chordModeEnabled = *v;
    if (auto v = ReadJSONInt(text, "chordType")) data.chordType = std::clamp(*v, 0, 4);
    if (auto v = ReadJSONString(text, "presetName")) data.presetName = *v;
    if (auto v = ReadJSONString(text, "lastPresetPath")) data.lastPresetPath = *v;
    if (auto v = ReadJSONInt(text, "prDisplayChannel")) data.prDisplayChannel = *v;
    if (auto v = ReadJSONInt(text, "prSnapIndex")) data.prSnapIndex = *v;
    if (auto v = ReadJSONFloat(text, "prPixelsPerQuarter")) data.prPixelsPerQuarter = *v;
    if (auto v = ReadJSONInt(text, "prTickOffset")) data.prTickOffset = *v;
    if (auto v = ReadJSONInt(text, "prNoteOffset")) data.prNoteOffset = *v;
    if (auto v = ReadJSONInt(text, "prVisibleNoteCount")) data.prVisibleNoteCount = *v;
    if (auto v = ReadJSONBool(text, "prDrumNameMode")) data.prDrumNameMode = *v;
    if (auto v = ReadJSONBool(text, "prFollowPreviewPlayback")) data.prFollowPreviewPlayback = *v;
    if (auto v = ReadJSONInt(text, "prPreviewStartTick")) data.prPreviewStartTick = *v;
    if (auto v = ReadJSONBool(text, "drumChannelSpecialHandling")) data.drumChannelSpecialHandling = *v;
    if (auto v = ReadJSONInt(text, "prSelectedCount"))
    {
        data.prSelectedIndices.clear();
        const int n = (std::max)(0, *v);
        data.prSelectedIndices.reserve(static_cast<size_t>(n));
        for (int i = 0; i < n; i++)
        {
            if (auto idx = ReadJSONInt(text, "prSelectedIndex" + std::to_string(i)))
            {
                data.prSelectedIndices.push_back(*idx);
            }
        }
    }

    for (int ch = 0; ch < 16; ch++)
    {
        // mix状態はキーごとに保持し、部分更新時も既存値を保持する。
        ChannelMixState& mix = data.channelMixStates[ch];
        const std::string kMute = "mixCh" + std::to_string(ch) + "Mute";
        const std::string kSolo = "mixCh" + std::to_string(ch) + "Solo";
        const std::string kLevel = "mixCh" + std::to_string(ch) + "Level";
        const std::string kPan = "mixCh" + std::to_string(ch) + "Pan";
        const std::string kGain = "mixCh" + std::to_string(ch) + "Gain";
        if (auto v = ReadJSONBool(text, kMute)) mix.mute = *v;
        if (auto v = ReadJSONBool(text, kSolo)) mix.solo = *v;
        if (auto v = ReadJSONFloat(text, kLevel)) mix.level = *v;
        if (auto v = ReadJSONFloat(text, kPan)) mix.pan = *v;
        if (auto v = ReadJSONFloat(text, kGain)) mix.gain = *v;

        const std::string kAssign = "assignCh" + std::to_string(ch);
        if (auto v = ReadJSONInt(text, kAssign))
        {
            data.channelAssignments[ch] = std::clamp(*v, 0, 15);
        }
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
    fout << "  \"midiPath\": \"" << EscapeJSON(data.midiPath) << "\",\n";
    fout << "  \"wavPath\": \"" << EscapeJSON(data.wavPath) << "\",\n";
    fout << "  \"targetChannel\": " << data.targetChannel << ",\n";
    fout << "  \"sampleRate\": " << data.sampleRate << ",\n";
    fout << "  \"initialSeconds\": " << data.initialSeconds << ",\n";
    fout << "  \"bits\": " << data.bits << ",\n";
    fout << "  \"extraReleaseSec\": " << data.extraReleaseSec << ",\n";
    fout << "  \"fxBitCrusherBits\": " << data.fxBitCrusherBits << ",\n";
    fout << "  \"fxSampleRateReducerRatio\": " << data.fxSampleRateReducerRatio << ",\n";
    fout << "  \"fxChorusEnabled\": " << (data.fxChorusEnabled ? "true" : "false") << ",\n";
    fout << "  \"fxChorusMix\": " << data.fxChorusMix << ",\n";
    fout << "  \"fxChorusBaseDelayMs\": " << data.fxChorusBaseDelayMs << ",\n";
    fout << "  \"fxChorusDepthMs\": " << data.fxChorusDepthMs << ",\n";
    fout << "  \"fxChorusRateHz\": " << data.fxChorusRateHz << ",\n";
    fout << "  \"fxChorusFeedback\": " << data.fxChorusFeedback << ",\n";
    fout << "  \"fxFlangerEnabled\": " << (data.fxFlangerEnabled ? "true" : "false") << ",\n";
    fout << "  \"fxFlangerMix\": " << data.fxFlangerMix << ",\n";
    fout << "  \"fxFlangerBaseDelayMs\": " << data.fxFlangerBaseDelayMs << ",\n";
    fout << "  \"fxFlangerDepthMs\": " << data.fxFlangerDepthMs << ",\n";
    fout << "  \"fxFlangerRateHz\": " << data.fxFlangerRateHz << ",\n";
    fout << "  \"fxFlangerFeedback\": " << data.fxFlangerFeedback << ",\n";
    fout << "  \"fxDelayEnabled\": " << (data.fxDelayEnabled ? "true" : "false") << ",\n";
    fout << "  \"fxDelayMix\": " << data.fxDelayMix << ",\n";
    fout << "  \"fxDelayTimeSec\": " << data.fxDelayTimeSec << ",\n";
    fout << "  \"fxDelayFeedback\": " << data.fxDelayFeedback << ",\n";
    fout << "  \"fxDelayTempoSync\": " << (data.fxDelayTempoSync ? "true" : "false") << ",\n";
    fout << "  \"fxDelaySyncBeats\": " << data.fxDelaySyncBeats << ",\n";
    fout << "  \"fxReverbEnabled\": " << (data.fxReverbEnabled ? "true" : "false") << ",\n";
    fout << "  \"fxReverbMix\": " << data.fxReverbMix << ",\n";
    fout << "  \"fxReverbRoomSize\": " << data.fxReverbRoomSize << ",\n";
    fout << "  \"fxReverbDamping\": " << data.fxReverbDamping << ",\n";
    // 既存stateファイル互換のため、保存キーは従来名を維持する。
    fout << "  \"uiScaleIndex\": " << data.UIScaleIndex << ",\n";
    fout << "  \"uiModeTab\": " << data.UIModeTab << ",\n";
    fout << "  \"logPanelHeight\": " << data.logPanelHeight << ",\n";
    fout << "  \"presetIndex\": " << data.presetIndex << ",\n";
    fout << "  \"serialSave\": " << (data.serialSave ? "true" : "false") << ",\n";
    fout << "  \"previewLoop\": " << (data.previewLoop ? "true" : "false") << ",\n";
    fout << "  \"autoTonePreviewEnabled\": " << (data.autoTonePreviewEnabled ? "true" : "false") << ",\n";
    fout << "  \"selectedSoundSlot\": " << data.selectedSoundSlot << ",\n";
    fout << "  \"selectedDrumNote\": " << data.selectedDrumNote << ",\n";
    fout << "  \"tonePreviewNoteNumber\": " << data.tonePreviewNoteNumber << ",\n";
    fout << "  \"chordModeEnabled\": " << (data.chordModeEnabled ? "true" : "false") << ",\n";
    fout << "  \"chordType\": " << data.chordType << ",\n";
    fout << "  \"presetName\": \"" << EscapeJSON(data.presetName) << "\",\n";
    fout << "  \"lastPresetPath\": \"" << EscapeJSON(data.lastPresetPath) << "\",\n";
    fout << "  \"prDisplayChannel\": " << data.prDisplayChannel << ",\n";
    fout << "  \"prSnapIndex\": " << data.prSnapIndex << ",\n";
    fout << "  \"prPixelsPerQuarter\": " << data.prPixelsPerQuarter << ",\n";
    fout << "  \"prTickOffset\": " << data.prTickOffset << ",\n";
    fout << "  \"prNoteOffset\": " << data.prNoteOffset << ",\n";
    fout << "  \"prVisibleNoteCount\": " << data.prVisibleNoteCount << ",\n";
    fout << "  \"prDrumNameMode\": " << (data.prDrumNameMode ? "true" : "false") << ",\n";
    fout << "  \"prFollowPreviewPlayback\": " << (data.prFollowPreviewPlayback ? "true" : "false") << ",\n";
    fout << "  \"prPreviewStartTick\": " << data.prPreviewStartTick << ",\n";
    fout << "  \"drumChannelSpecialHandling\": " << (data.drumChannelSpecialHandling ? "true" : "false") << ",\n";
    fout << "  \"prSelectedCount\": " << data.prSelectedIndices.size() << ",\n";
    for (size_t i = 0; i < data.prSelectedIndices.size(); i++)
    {
        fout << "  \"prSelectedIndex" << i << "\": " << data.prSelectedIndices[i] << ",\n";
    }
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelMixState& mix = data.channelMixStates[ch];
        fout << "  \"mixCh" << ch << "Mute\": " << (mix.mute ? "true" : "false") << ",\n";
        fout << "  \"mixCh" << ch << "Solo\": " << (mix.solo ? "true" : "false") << ",\n";
        fout << "  \"mixCh" << ch << "Level\": " << mix.level << ",\n";
        fout << "  \"mixCh" << ch << "Pan\": " << mix.pan << ",\n";
        fout << "  \"mixCh" << ch << "Gain\": " << mix.gain << ",\n";
        fout << "  \"assignCh" << ch << "\": " << data.channelAssignments[ch];
        fout << (ch == 15 ? "\n" : ",\n");
    }
    fout << "}\n";

    return true;
}

bool LoadPianoRollProjectStorageFile(const std::filesystem::path& path, PianoRollProjectStorageData& data, std::string& err)
{
    if (!std::filesystem::exists(path))
    {
        return true;
    }

    std::ifstream fin(path, std::ios::binary);
    if (!fin)
    {
        err = "failed to open " + path.string();
        return false;
    }

    data = PianoRollProjectStorageData{};

    std::unordered_map<int, PianoRollProjectStorageNote> noteMap;
    int noteCount = 0;
    std::string line;
    while (std::getline(fin, line))
    {
        std::string key;
        std::string value;
        if (!ParseFlatJSONLine(line, key, value))
        {
            continue;
        }

        if (key == "midiPath")
        {
            if (auto v = ParseStringValue(value))
            {
                data.midiPath = *v;
            }
            continue;
        }
        if (key == "ticksPerQuarter")
        {
            if (auto v = ParseIntValue(value))
            {
                data.ticksPerQuarter = *v;
            }
            continue;
        }
        if (key == "noteCount")
        {
            if (auto v = ParseIntValue(value))
            {
                noteCount = (std::max)(0, *v);
            }
            continue;
        }

        if (!StartsWith(key, "note"))
        {
            continue;
        }

        auto setField = [&](const std::string& suffix, auto setter) {
            if (key.size() <= 4 + suffix.size())
            {
                return false;
            }
            if (key.compare(key.size() - suffix.size(), suffix.size(), suffix) != 0)
            {
                return false;
            }
            const std::string idxStr = key.substr(4, key.size() - 4 - suffix.size());
            int idx = 0;
            try
            {
                idx = std::stoi(idxStr);
            }
            catch (...)
            {
                return false;
            }
            if (idx < 0)
            {
                return false;
            }
            auto it = noteMap.find(idx);
            if (it == noteMap.end())
            {
                it = noteMap.emplace(idx, PianoRollProjectStorageNote{}).first;
            }
            setter(it->second, value);
            return true;
        };

        if (setField("StartTick", [](PianoRollProjectStorageNote& n, const std::string& v) {
            if (auto iv = ParseIntValue(v)) n.startTick = *iv;
            }))
        {
            continue;
        }
        if (setField("EndTick", [](PianoRollProjectStorageNote& n, const std::string& v) {
            if (auto iv = ParseIntValue(v)) n.endTick = *iv;
            }))
        {
            continue;
        }
        if (setField("Note", [](PianoRollProjectStorageNote& n, const std::string& v) {
            if (auto iv = ParseIntValue(v)) n.note = *iv;
            }))
        {
            continue;
        }
        if (setField("Channel", [](PianoRollProjectStorageNote& n, const std::string& v) {
            if (auto iv = ParseIntValue(v)) n.channel = *iv;
            }))
        {
            continue;
        }
        setField("Velocity", [](PianoRollProjectStorageNote& n, const std::string& v) {
            if (auto iv = ParseIntValue(v)) n.velocity = *iv;
            });
    }

    if (noteCount <= 0)
    {
        return true;
    }
    data.notes.reserve(static_cast<size_t>(noteCount));
    for (int i = 0; i < noteCount; i++)
    {
        auto it = noteMap.find(i);
        if (it == noteMap.end())
        {
            // 欠番ノートは最小長ノートで補完し、配列インデックス整合を優先する。
            PianoRollProjectStorageNote n{};
            n.endTick = n.startTick + 1;
            data.notes.push_back(n);
            continue;
        }
        PianoRollProjectStorageNote n = it->second;
        if (n.endTick <= n.startTick)
        {
            n.endTick = n.startTick + 1;
        }
        data.notes.push_back(n);
    }

    return true;
}

bool SavePianoRollProjectStorageFile(const std::filesystem::path& path, const PianoRollProjectStorageData& data, std::string& err)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream fout(path, std::ios::binary | std::ios::trunc);
    if (!fout)
    {
        err = "failed to open " + path.string();
        return false;
    }

    fout << "{\n";
    fout << "  \"midiPath\": \"" << EscapeJSON(data.midiPath) << "\",\n";
    fout << "  \"ticksPerQuarter\": " << data.ticksPerQuarter << ",\n";
    fout << "  \"noteCount\": " << data.notes.size();
    if (!data.notes.empty())
    {
        fout << ",\n";
    }
    else
    {
        fout << "\n";
    }

    for (size_t i = 0; i < data.notes.size(); i++)
    {
        const auto& n = data.notes[i];
        const bool last = (i + 1 == data.notes.size());
        fout << "  \"note" << i << "StartTick\": " << n.startTick << ",\n";
        fout << "  \"note" << i << "EndTick\": " << n.endTick << ",\n";
        fout << "  \"note" << i << "Note\": " << n.note << ",\n";
        fout << "  \"note" << i << "Channel\": " << n.channel << ",\n";
        fout << "  \"note" << i << "Velocity\": " << n.velocity << (last ? "\n" : ",\n");
    }
    fout << "}\n";
    return true;
}
