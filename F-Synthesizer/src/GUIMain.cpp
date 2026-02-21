#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <regex>
#include <optional>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <type_traits>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppCore.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIPlatform.h"
#include "gui/GUIRunHelpers.h"
#include "gui/GUIStateStorage.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3dll.lib")
#ifdef _DEBUG
#pragma comment(lib, "imguid.lib")
#else
#pragma comment(lib, "imgui.lib")
#endif

namespace
{
struct GUIState
{
    struct GUIRunObserver : IRunObserver
    {
        std::mutex* logMutex = nullptr;
        std::vector<std::string>* logs = nullptr;
        std::atomic<bool>* cancelRequested = nullptr;

        void OnLogLine(const std::string& line) override
        {
            if (logMutex == nullptr || logs == nullptr)
            {
                return;
            }
            std::lock_guard<std::mutex> lock(*logMutex);
            logs->push_back(line);
        }

        bool ShouldCancel() override
        {
            return cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed);
        }
    };

    char midiPath[1024]{};
    char wavPath[1024]{};
    int targetChannel = -1;
    int sampleRate = 44100;
    int initialSeconds = 6;
    int bits = 16;
    float extraReleaseSec = 0.3f;
    int defaultWave = 2; // saw
    int uiScaleIndex = 1; // 0=100%, 1=125%, 2=150%
    float logPanelHeight = 240.0f;
    int presetIndex = 0;
    int lastRunExitCode = 0;
    bool hasRun = false;
    bool running = false;
    std::atomic<bool> stopRequested{ false };
    bool serialSave = false;
    int selectedChannel = 0;
    int selectedDrumNote = 36;
    char presetName[128]{ "custom" };
    bool presetDirty = false;
    std::vector<std::string> presetItems{};
    std::string lastOutputPath{};
    std::string lastPresetPath{};
    std::shared_ptr<std::array<ChannelConfig, 16>> channelConfigs{};
    std::shared_ptr<std::array<ChannelMixState, 16>> channelMixStates{};
    double lastPeak = 0.0;
    bool hasPeak = false;
    bool soloPreviewActive = false;
    bool restorePreviewOnRunComplete = false;
    int soloPreviewChannel = 0;
    std::array<ChannelMixState, 16> soloPreviewBackup{};
    bool previewLoop = false;
    bool previewAudioReady = false;
    bool runIsPreview = false;
    bool autoPlayPreviewOnRunComplete = false;
    std::shared_ptr<SoundData> previewRenderedSound{};
    std::shared_ptr<SoundData> runOutputBuffer{};
    PreviewPlaybackState playback{};
    std::future<int> runFuture{};
    std::mutex logMutex{};
    std::vector<std::string> logs{};
    GUIRunObserver observer{};
};

void EnsureChannelConfigs(GUIState& state);
void EnsureChannelMixStates(GUIState& state);
float UiScaleFromIndex(int idx);
const char* UiScaleLabelFromIndex(int idx);
void DrawStatusBadge(const GUIState& state);
using gui::ChannelConfigEquals;
using gui::ChannelMixStateEquals;
using gui::DefaultSourceByType;
using gui::DrumTypeToText;
using gui::NoiseFromIndex;
using gui::NoiseToIndex;
using gui::NoiseToText;
using gui::SourceTypeIndex;
using gui::WaveFromIndex;
using gui::WaveToIndex;
using gui::WaveToText;
using gui::WriteJsonEscaped;
using gui::WriteSourceJson;
using gui::BuildPreviewWavPath;
using gui::BuildSerialWavPath;

void SetupImGuiFont()
{
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\meiryo.ttc",
        "C:\\Windows\\Fonts\\YuGothM.ttc",
        "C:\\Windows\\Fonts\\msgothic.ttc"
    };

    for (const char* fontPath : candidates)
    {
        std::error_code ec;
        if (!std::filesystem::exists(fontPath, ec))
        {
            continue;
        }
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 20.0f, nullptr, ranges);
        if (font != nullptr)
        {
            io.FontDefault = font;
            return;
        }
    }
}

void AppendGUILog(GUIState& state, const std::string& line)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    state.logs.push_back(line);
}

bool SavePresetDiff(const GUIState& state, const std::filesystem::path& presetPath, std::string& err)
{
    AppConfig base = DefaultConfig();
    if (!state.channelConfigs || !base.channelConfigs || !state.channelMixStates || !base.channelMixStates)
    {
        err = "channel configs or mix states are not initialized";
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(presetPath.parent_path(), ec);
    std::ofstream out(presetPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open preset file";
        return false;
    }

    out << "{\n";
    out << "  \"midiPath\": \"";
    WriteJsonEscaped(out, state.midiPath);
    out << "\",\n";
    out << "  \"wavPath\": \"";
    WriteJsonEscaped(out, state.wavPath);
    out << "\",\n";
    out << "  \"channels\": {\n";

    bool first = true;
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelConfig& cur = (*state.channelConfigs)[ch];
        const ChannelConfig& def = (*base.channelConfigs)[ch];
        if (ChannelConfigEquals(cur, def))
        {
            continue;
        }
        if (!first) out << ",\n";
        first = false;
        out << "    \"" << ch << "\": {\n";
        out << "      \"amp\": " << cur.amp << ",\n";
        out << "      \"attackSec\": " << cur.attackSec << ",\n";
        out << "      \"decaySec\": " << cur.decaySec << ",\n";
        out << "      \"sustainLevel\": " << cur.sustainLevel << ",\n";
        out << "      \"releaseSec\": " << cur.releaseSec << ",\n";
        WriteSourceJson(out, cur.source, 6);
        out << "\n    }";
    }

    out << "\n  },\n";
    out << "  \"channelMix\": {\n";

    bool firstMix = true;
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelMixState& cur = (*state.channelMixStates)[ch];
        const ChannelMixState& def = (*base.channelMixStates)[ch];
        if (ChannelMixStateEquals(cur, def))
        {
            continue;
        }
        if (!firstMix) out << ",\n";
        firstMix = false;
        out << "    \"" << ch << "\": {\n";
        out << "      \"mute\": " << (cur.mute ? "true" : "false") << ",\n";
        out << "      \"solo\": " << (cur.solo ? "true" : "false") << ",\n";
        out << "      \"level\": " << cur.level << ",\n";
        out << "      \"pan\": " << cur.pan << ",\n";
        out << "      \"gain\": " << cur.gain << "\n";
        out << "    }";
    }

    out << "\n  }\n";
    out << "}\n";
    return true;
}

std::vector<std::string> CollectPresetItems()
{
    std::vector<std::string> names;
    const std::filesystem::path root = FindProjectRootPath();
    const std::filesystem::path dir = root / "config" / "presets";

    std::error_code ec;
    if (std::filesystem::exists(dir, ec))
    {
        for (const auto& ent : std::filesystem::directory_iterator(dir, ec))
        {
            if (ec) break;
            if (!ent.is_regular_file()) continue;
            if (ent.path().extension() != ".json") continue;
            names.push_back(ent.path().stem().string());
        }
    }

    std::sort(names.begin(), names.end());
    const auto it = std::find(names.begin(), names.end(), "basic_wave");
    if (it != names.end() && it != names.begin())
    {
        std::rotate(names.begin(), it, it + 1);
    }
    return names;
}

int FindPresetIndex(const GUIState& state, const std::string& name)
{
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); i++)
    {
        if (state.presetItems[i] == name)
        {
            return i;
        }
    }
    return -1;
}

void RefreshPresetItems(GUIState& state, const std::string& preferName)
{
    state.presetItems = CollectPresetItems();
    if (state.presetItems.empty())
    {
        state.presetItems.push_back("basic_wave");
    }

    int idx = FindPresetIndex(state, preferName);
    if (idx < 0)
    {
        idx = FindPresetIndex(state, "basic_wave");
    }
    state.presetIndex = (idx >= 0) ? idx : 0;
}

bool ApplySelectedPresetPaths(GUIState& state, std::string& err)
{
    if (state.presetItems.empty())
    {
        err = "preset list is empty";
        return false;
    }
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        err = "invalid preset index";
        return false;
    }

    const std::string& presetName = state.presetItems[state.presetIndex];
    strncpy_s(state.presetName, sizeof(state.presetName), presetName.c_str(), _TRUNCATE);
    const std::filesystem::path root = FindProjectRootPath();
    const std::filesystem::path basePath = root / "config" / "base.json";
    const std::filesystem::path presetPath = root / "config" / "presets" / (presetName + ".json");

    AppConfig cfg = DefaultConfig();
    if (std::filesystem::exists(basePath))
    {
        if (!LoadConfigFile(basePath, cfg, err))
        {
            err = "failed to load base config: " + err;
            return false;
        }
    }
    if (!LoadConfigFile(presetPath, cfg, err))
    {
        err = "failed to load preset config: " + err;
        return false;
    }

    CopyPath(state.midiPath, sizeof(state.midiPath), cfg.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), cfg.wavPath);
    state.targetChannel = cfg.targetChannel;
    state.sampleRate = cfg.sampleRate;
    state.initialSeconds = cfg.initialSeconds;
    state.bits = cfg.bits;
    state.extraReleaseSec = static_cast<float>(cfg.extraReleaseSec);
    state.defaultWave = WaveToIndex(cfg.defaultWave);

    EnsureChannelConfigs(state);
    EnsureChannelMixStates(state);
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
    return true;
}

std::filesystem::path GUIStatePath()
{
    return FindProjectRootPath() / "config" / "gui_state.json";
}

GUIStateStorageData BuildStateStorageData(const GUIState& state)
{
    GUIStateStorageData data{};
    data.midiPath = state.midiPath;
    data.wavPath = state.wavPath;
    data.targetChannel = state.targetChannel;
    data.sampleRate = state.sampleRate;
    data.initialSeconds = state.initialSeconds;
    data.bits = state.bits;
    data.extraReleaseSec = state.extraReleaseSec;
    data.defaultWave = state.defaultWave;
    data.uiScaleIndex = state.uiScaleIndex;
    data.logPanelHeight = state.logPanelHeight;
    data.presetIndex = state.presetIndex;
    data.serialSave = state.serialSave;
    data.previewLoop = state.previewLoop;
    data.selectedChannel = state.selectedChannel;
    data.selectedDrumNote = state.selectedDrumNote;
    data.presetName = state.presetName;
    data.lastPresetPath = state.lastPresetPath;
    for (int ch = 0; ch < 16; ch++)
    {
        data.channelMixStates[ch] = (state.channelMixStates != nullptr)
            ? (*state.channelMixStates)[ch]
            : ChannelMixState{};
    }
    return data;
}

void ApplyStateStorageData(GUIState& state, const GUIStateStorageData& data)
{
    strncpy_s(state.midiPath, sizeof(state.midiPath), data.midiPath.c_str(), _TRUNCATE);
    strncpy_s(state.wavPath, sizeof(state.wavPath), data.wavPath.c_str(), _TRUNCATE);
    state.targetChannel = data.targetChannel;
    state.sampleRate = data.sampleRate;
    state.initialSeconds = data.initialSeconds;
    state.bits = data.bits;
    state.extraReleaseSec = data.extraReleaseSec;
    state.defaultWave = data.defaultWave;
    state.uiScaleIndex = data.uiScaleIndex;
    state.logPanelHeight = data.logPanelHeight;
    state.presetIndex = data.presetIndex;
    state.serialSave = data.serialSave;
    state.previewLoop = data.previewLoop;
    state.selectedChannel = data.selectedChannel;
    state.selectedDrumNote = data.selectedDrumNote;
    strncpy_s(state.presetName, sizeof(state.presetName), data.presetName.c_str(), _TRUNCATE);
    state.lastPresetPath = data.lastPresetPath;

    EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        (*state.channelMixStates)[ch] = data.channelMixStates[ch];
    }
}

bool LoadGUIStateFile(GUIState& state, std::string& err)
{
    GUIStateStorageData data = BuildStateStorageData(state);
    if (!LoadGUIStateStorageFile(GUIStatePath(), data, err))
    {
        return false;
    }
    ApplyStateStorageData(state, data);
    return true;
}

bool SaveGUIStateFile(const GUIState& state, std::string& err)
{
    const GUIStateStorageData data = BuildStateStorageData(state);
    return SaveGUIStateStorageFile(GUIStatePath(), data, err);
}

AppConfig BuildConfigFromGUI(const GUIState& state)
{
    AppConfig cfg = DefaultConfig();
    cfg.midiPath = Utf8ToPath(state.midiPath);
    cfg.wavPath = Utf8ToPath(state.wavPath);
    cfg.targetChannel = state.targetChannel;
    cfg.sampleRate = state.sampleRate;
    cfg.initialSeconds = state.initialSeconds;
    cfg.bits = state.bits;
    cfg.extraReleaseSec = state.extraReleaseSec;
    cfg.defaultWave = WaveFromIndex(state.defaultWave);
    if (state.channelConfigs)
    {
        cfg.channelConfigs = std::static_pointer_cast<const std::array<ChannelConfig, 16>>(state.channelConfigs);
    }
    if (state.channelMixStates)
    {
        cfg.channelMixStates = std::static_pointer_cast<const std::array<ChannelMixState, 16>>(state.channelMixStates);
    }
    return cfg;
}

bool ValidateBeforeRun(const GUIState& state, std::string& err)
{
    return gui::ValidateRunSettings(
        state.midiPath,
        state.wavPath,
        state.targetChannel,
        state.sampleRate,
        state.initialSeconds,
        state.bits,
        err);
}

void InitGUIState(GUIState& state)
{
    StopPreviewAudio(state.playback);
    AppConfig cfg = DefaultConfig();
    CopyPath(state.midiPath, sizeof(state.midiPath), cfg.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), cfg.wavPath);
    state.targetChannel = cfg.targetChannel;
    state.sampleRate = cfg.sampleRate;
    state.initialSeconds = cfg.initialSeconds;
    state.bits = cfg.bits;
    state.extraReleaseSec = static_cast<float>(cfg.extraReleaseSec);
    state.defaultWave = 2;
    state.uiScaleIndex = 1;
    state.logPanelHeight = 240.0f;
    state.presetIndex = 0;
    state.selectedChannel = 0;
    state.selectedDrumNote = 36;
    strncpy_s(state.presetName, sizeof(state.presetName), "basic_wave", _TRUNCATE);
    state.running = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.hasRun = false;
    state.lastRunExitCode = 0;
    state.serialSave = false;
    state.lastOutputPath.clear();
    state.lastPresetPath.clear();
    state.logs.clear();
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
    state.soloPreviewChannel = 0;
    state.previewLoop = false;
    state.previewAudioReady = false;
    state.runIsPreview = false;
    state.autoPlayPreviewOnRunComplete = false;
    state.previewRenderedSound.reset();
    state.runOutputBuffer.reset();
    state.observer.logMutex = &state.logMutex;
    state.observer.logs = &state.logs;
    state.observer.cancelRequested = &state.stopRequested;
    RefreshPresetItems(state, state.presetName);

    state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
    state.soloPreviewBackup = *state.channelMixStates;
}

void RepairGUIStatePathsIfNeeded(GUIState& state)
{
    const AppConfig def = DefaultConfig();
    const std::filesystem::path midi = Utf8ToPath(state.midiPath);
    const std::filesystem::path wav = Utf8ToPath(state.wavPath);

    bool repaired = false;
    if (!std::filesystem::exists(midi))
    {
        CopyPath(state.midiPath, sizeof(state.midiPath), def.midiPath);
        repaired = true;
    }
    if (wav.extension().empty() || std::filesystem::is_directory(wav))
    {
        CopyPath(state.wavPath, sizeof(state.wavPath), def.wavPath);
        repaired = true;
    }
    if (state.targetChannel < -1 || state.targetChannel > 15)
    {
        state.targetChannel = def.targetChannel;
        repaired = true;
    }
    if (state.presetItems.empty())
    {
        RefreshPresetItems(state, state.presetName);
    }
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        RefreshPresetItems(state, state.presetName);
        repaired = true;
    }
    if (state.sampleRate <= 0)
    {
        state.sampleRate = def.sampleRate;
        repaired = true;
    }
    if (state.initialSeconds <= 0)
    {
        state.initialSeconds = def.initialSeconds;
        repaired = true;
    }
    if (state.bits != 16)
    {
        state.bits = 16;
        repaired = true;
    }
    if (state.uiScaleIndex < 0 || state.uiScaleIndex > 2)
    {
        state.uiScaleIndex = 1;
        repaired = true;
    }
    if (state.logPanelHeight < 140.0f || state.logPanelHeight > 520.0f)
    {
        state.logPanelHeight = std::clamp(state.logPanelHeight, 140.0f, 520.0f);
        repaired = true;
    }
    EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
        bool mixRepaired = false;
        if (mix.level < 0.0 || mix.level > 2.0)
        {
            mix.level = std::clamp(mix.level, 0.0, 2.0);
            mixRepaired = true;
        }
        if (mix.pan < -1.0 || mix.pan > 1.0)
        {
            mix.pan = std::clamp(mix.pan, -1.0, 1.0);
            mixRepaired = true;
        }
        if (mix.gain < 0.0 || mix.gain > 4.0)
        {
            mix.gain = std::clamp(mix.gain, 0.0, 4.0);
            mixRepaired = true;
        }
        if (mixRepaired)
        {
            repaired = true;
            AppendGUILog(state, "[GUI] Invalid mix state detected and clamped: ch" + std::to_string(ch));
        }
    }
    if (repaired)
    {
        AppendGUILog(state, "[GUI] Detected invalid saved state. Recovered to safe defaults.");
    }
}

bool DrawDrumConfigEditor(const char* idPrefix, DrumConfig& d)
{
    bool changed = false;
    int drumType = static_cast<int>(d.type);
    const char* drumTypes[] = { "none", "kick", "snare", "hat" };
    std::string key = std::string("Drum Type##") + idPrefix;
    changed |= ImGui::Combo(key.c_str(), &drumType, drumTypes, IM_ARRAYSIZE(drumTypes));
    d.type = static_cast<DrumType>(drumType);

    key = std::string("Gain##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.gain, 0.01, 0.1, "%.3f");
    key = std::string("Base Freq##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.baseFreq, 1.0, 10.0, "%.2f");
    key = std::string("Pitch Drop##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDrop, 0.1, 1.0, "%.3f");
    key = std::string("Pitch Decay##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
    key = std::string("Tone Freq##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.toneFreq, 10.0, 100.0, "%.2f");
    key = std::string("Tone Level##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.toneLevel, 0.01, 0.1, "%.3f");
    key = std::string("Noise Level##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.noiseLevel, 0.01, 0.1, "%.3f");
    key = std::string("HP Cut##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
    key = std::string("LP Cut##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");

    int toneWave = d.toneWave >= 0 ? d.toneWave : 0;
    const char* waves[] = { "sine", "square", "saw", "triangle" };
    key = std::string("Tone Wave##") + idPrefix;
    changed |= ImGui::Combo(key.c_str(), &toneWave, waves, IM_ARRAYSIZE(waves));
    d.toneWave = toneWave;

    int noiseType = d.noiseType >= 0 ? d.noiseType : 0;
    const char* noises[] = { "white", "pink", "brown", "blue" };
    key = std::string("Noise Type##") + idPrefix;
    changed |= ImGui::Combo(key.c_str(), &noiseType, noises, IM_ARRAYSIZE(noises));
    d.noiseType = noiseType;
    return changed;
}

void EnsureChannelConfigs(GUIState& state)
{
    if (state.channelConfigs)
    {
        return;
    }
    AppConfig cfg = DefaultConfig();
    state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
}

void EnsureChannelMixStates(GUIState& state)
{
    if (state.channelMixStates)
    {
        return;
    }
    AppConfig cfg = DefaultConfig();
    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
}

void AnalyzeRenderPeakFromLogs(GUIState& state)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    for (auto it = state.logs.rbegin(); it != state.logs.rend(); ++it)
    {
        const std::string& line = *it;
        const std::string key = "[RenderStats] peak=";
        const size_t pos = line.find(key);
        if (pos == std::string::npos)
        {
            continue;
        }
        const size_t start = pos + key.size();
        size_t end = start;
        while (end < line.size() && (std::isdigit(static_cast<unsigned char>(line[end])) || line[end] == '.' || line[end] == '-'))
        {
            end++;
        }
        if (end <= start)
        {
            break;
        }
        try
        {
            state.lastPeak = std::stod(line.substr(start, end - start));
            state.hasPeak = true;
        }
        catch (...)
        {
            state.hasPeak = false;
        }
        return;
    }
}

void ActivateSoloPreview(GUIState& state, int channel)
{
    EnsureChannelMixStates(state);
    channel = std::clamp(channel, 0, 15);
    if (!state.soloPreviewActive)
    {
        state.soloPreviewBackup = *state.channelMixStates;
    }
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
        mix.solo = (ch == channel);
        if (ch == channel)
        {
            mix.mute = false;
        }
    }
    state.soloPreviewChannel = channel;
    state.soloPreviewActive = true;
    AppendGUILog(state, "[GUI] Solo Preview ON: ch" + std::to_string(channel));
}

void DeactivateSoloPreview(GUIState& state)
{
    if (!state.soloPreviewActive || !state.channelMixStates)
    {
        return;
    }
    *state.channelMixStates = state.soloPreviewBackup;
    AppendGUILog(state, "[GUI] Solo Preview OFF: restore previous mix state");
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
}

bool DrawChannelEditor(GUIState& state)
{
    bool changed = false;
    EnsureChannelConfigs(state);
    EnsureChannelMixStates(state);
    state.selectedChannel = std::clamp(state.selectedChannel, 0, 15);

    ImGui::Text("Channel");
    changed |= ImGui::InputInt("Selected Channel (0-15)", &state.selectedChannel);
    state.selectedChannel = std::clamp(state.selectedChannel, 0, 15);

    auto sliderMix = [&](const char* label, double& value, float minV, float maxV) -> bool
    {
        float v = static_cast<float>(value);
        bool edited = ImGui::SliderFloat(label, &v, minV, maxV, "%.2f");
        if (edited)
        {
            value = static_cast<double>(v);
        }
        return edited;
    };

    ImGui::Text("Mix Summary (M/S/L)");
    ImGui::BeginChild("mix_summary", ImVec2(0, 210), true);
    if (ImGui::BeginTable("mix_compact_table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("ch", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableSetupColumn("sel", ImGuiTableColumnFlags_WidthFixed, 56.0f);
        ImGui::TableSetupColumn("M", ImGuiTableColumnFlags_WidthFixed, 36.0f);
        ImGui::TableSetupColumn("S", ImGuiTableColumnFlags_WidthFixed, 36.0f);
        ImGui::TableSetupColumn("L");
        ImGui::TableHeadersRow();

        for (int ch = 0; ch < 16; ch++)
        {
            ChannelMixState& mix = (*state.channelMixStates)[ch];
            ImGui::TableNextRow();
            ImGui::PushID(ch);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ch%d", ch);

            ImGui::TableSetColumnIndex(1);
            bool selected = (state.selectedChannel == ch);
            if (ImGui::Selectable("Edit", selected))
            {
                state.selectedChannel = ch;
            }

            ImGui::TableSetColumnIndex(2);
            changed |= ImGui::Checkbox("##mute", &mix.mute);

            ImGui::TableSetColumnIndex(3);
            changed |= ImGui::Checkbox("##solo", &mix.solo);

            ImGui::TableSetColumnIndex(4);
            changed |= sliderMix("##level", mix.level, 0.0f, 2.0f);

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::Separator();
    ChannelConfig& chCfg = (*state.channelConfigs)[state.selectedChannel];
    ChannelMixState& chMix = (*state.channelMixStates)[state.selectedChannel];
    ImGui::Text("Selected ch%d", state.selectedChannel);
    ImGui::TextDisabled("Audition target: selected channel (use Play Preview)");

    if (ImGui::CollapsingHeader("Mix Details", ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::Checkbox("Mute", &chMix.mute);
        ImGui::SameLine();
        changed |= ImGui::Checkbox("Solo", &chMix.solo);
        changed |= sliderMix("Level", chMix.level, 0.0f, 2.0f);
        changed |= sliderMix("Pan", chMix.pan, -1.0f, 1.0f);
        changed |= sliderMix("Gain", chMix.gain, 0.0f, 4.0f);
    }

    if (ImGui::CollapsingHeader("Envelope / Gain", ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::InputDouble("Ch Amp", &chCfg.amp, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Attack", &chCfg.attackSec, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Decay", &chCfg.decaySec, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Sustain", &chCfg.sustainLevel, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");
    }

    if (ImGui::CollapsingHeader("Source Details", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int srcType = SourceTypeIndex(chCfg.source);
        const char* sourceTypes[] = { "waveform", "noise", "fm", "drum", "drumkit" };
        if (ImGui::Combo("Source Type", &srcType, sourceTypes, IM_ARRAYSIZE(sourceTypes)))
        {
            changed = true;
            chCfg.source = DefaultSourceByType(srcType);
        }

        if (auto* wf = std::get_if<WaveformConfig>(&chCfg.source))
        {
            int idx = WaveToIndex(wf->wave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Wave", &idx, waves, IM_ARRAYSIZE(waves));
            wf->wave = WaveFromIndex(idx);
        }
        else if (auto* nz = std::get_if<NoiseConfig>(&chCfg.source))
        {
            int idx = NoiseToIndex(nz->noise);
            const char* noises[] = { "white", "pink", "brown", "blue" };
            changed |= ImGui::Combo("Noise", &idx, noises, IM_ARRAYSIZE(noises));
            nz->noise = NoiseFromIndex(idx);
        }
        else if (auto* fm = std::get_if<FmConfig>(&chCfg.source))
        {
            int cIdx = WaveToIndex(fm->carrierWave);
            int mIdx = WaveToIndex(fm->modWave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Carrier Wave", &cIdx, waves, IM_ARRAYSIZE(waves));
            changed |= ImGui::Combo("Mod Wave", &mIdx, waves, IM_ARRAYSIZE(waves));
            fm->carrierWave = WaveFromIndex(cIdx);
            fm->modWave = WaveFromIndex(mIdx);
            changed |= ImGui::InputDouble("Carrier Ratio", &fm->carrierRatio, 0.01, 0.1, "%.3f");
            changed |= ImGui::InputDouble("Mod Ratio", &fm->modRatio, 0.01, 0.1, "%.3f");
            changed |= ImGui::InputDouble("FM Index", &fm->index, 0.01, 0.1, "%.3f");
            changed |= ImGui::InputDouble("FM OutLevel", &fm->outLevel, 0.01, 0.1, "%.3f");
        }
        else if (auto* drum = std::get_if<DrumConfig>(&chCfg.source))
        {
            changed |= DrawDrumConfigEditor("drum_single", *drum);
        }
        else if (auto* kit = std::get_if<DrumKitConfig>(&chCfg.source))
        {
            changed |= ImGui::InputInt("DrumKit Note (0-127)", &state.selectedDrumNote);
            state.selectedDrumNote = std::clamp(state.selectedDrumNote, 0, 127);
            DrumConfig& d = kit->map[state.selectedDrumNote];
            changed |= DrawDrumConfigEditor("drum_kit", d);
        }
    }
    return changed;
}

float UiScaleFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return 1.0f;
    case 1: return 1.25f;
    case 2: return 1.5f;
    default: return 1.25f;
    }
}

const char* UiScaleLabelFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return "100%";
    case 1: return "125%";
    case 2: return "150%";
    default: return "125%";
    }
}

void DrawStatusBadge(const GUIState& state)
{
    ImVec4 color = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    const char* label = "Idle";

    if (state.running)
    {
        color = ImVec4(0.95f, 0.78f, 0.2f, 1.0f);
        label = "Running";
    }
    else if (state.playback.playing.load(std::memory_order_relaxed))
    {
        color = ImVec4(0.28f, 0.82f, 0.95f, 1.0f);
        label = state.previewLoop ? "Preview (Loop)" : "Preview";
    }
    else if (state.hasRun && state.lastRunExitCode == 2)
    {
        color = ImVec4(0.95f, 0.70f, 0.25f, 1.0f);
        label = "Canceled";
    }
    else if (state.hasRun && state.lastRunExitCode == 0)
    {
        color = ImVec4(0.30f, 0.82f, 0.40f, 1.0f);
        label = "Success";
    }
    else if (state.hasRun)
    {
        color = ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
        label = "Failed";
    }

    ImGui::TextUnformatted("Status:");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}
} // namespace

int RunGUIApp()
{
    if (!glfwInit())
    {
        return 1;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "F-Synthesizer GUI (Preview)", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    SetupImGuiFont();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    GUIState state{};
    InitGUIState(state);

    {
        std::string err;
        if (!LoadGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state load failed: " + err);
        }
        else
        {
            AppendGUILog(state, "[GUI] gui_state loaded: " + GUIStatePath().string());
        }
        RepairGUIStatePathsIfNeeded(state);
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (state.running &&
            state.runFuture.valid() &&
            state.runFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            state.lastRunExitCode = state.runFuture.get();
            state.hasRun = true;
            state.running = false;
            AppendGUILog(state, std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
            if (state.runIsPreview)
            {
                if (state.lastRunExitCode == 0 &&
                    state.runOutputBuffer != nullptr &&
                    !state.runOutputBuffer->data.empty())
                {
                    state.previewRenderedSound = state.runOutputBuffer;
                    state.previewAudioReady = true;
                    AppendGUILog(state, "[GUI] Preview audio buffer ready");
                    if (state.autoPlayPreviewOnRunComplete)
                    {
                        std::string playErr;
                        if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, playErr))
                        {
                            AppendGUILog(state, "[GUI] Preview playback started");
                        }
                        else
                        {
                            AppendGUILog(state, "[GUI] Preview playback failed: " + playErr);
                        }
                    }
                }
                else
                {
                    state.previewAudioReady = false;
                    state.previewRenderedSound.reset();
                }
                state.runOutputBuffer.reset();
                state.runIsPreview = false;
                state.autoPlayPreviewOnRunComplete = false;
            }
            if (state.restorePreviewOnRunComplete)
            {
                DeactivateSoloPreview(state);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().FontGlobalScale = UiScaleFromIndex(state.uiScaleIndex);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        const ImGuiWindowFlags rootFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("F-SynthesizerRoot", nullptr, rootFlags);

        ImGui::TextUnformatted("F-Synthesizer GUI");
        ImGui::Separator();
        DrawStatusBadge(state);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16.0f);
        ImGui::TextUnformatted("UI Scale");
        ImGui::SameLine();
        const char* uiScales[] = { "100%", "125%", "150%" };
        if (ImGui::Combo("##ui_scale", &state.uiScaleIndex, uiScales, IM_ARRAYSIZE(uiScales)))
        {
            AppendGUILog(state, std::string("[GUI] UI scale changed: ") + UiScaleLabelFromIndex(state.uiScaleIndex));
        }
        ImGui::Separator();
        auto startRun = [&](bool previewSelected)
        {
            std::string validationError;
            if (!ValidateBeforeRun(state, validationError))
            {
                state.hasRun = true;
                state.lastRunExitCode = 1;
                AppendGUILog(state, "[GUI] Validation failed: " + validationError);
            }
            else
            {
                if (previewSelected)
                {
                    ActivateSoloPreview(state, state.selectedChannel);
                }
                if (state.playback.playing.load(std::memory_order_relaxed))
                {
                    StopPreviewAudio(state.playback);
                    AppendGUILog(state, "[GUI] Previous preview playback stopped for new run");
                }
                AppConfig cfg = BuildConfigFromGUI(state);
                RenderOptions options = previewSelected ? DefaultPreviewRenderOptions() : DefaultRenderOptions();
                if (!previewSelected && state.serialSave)
                {
                    cfg.wavPath = BuildSerialWavPath(cfg.wavPath);
                }
                if (previewSelected)
                {
                    state.restorePreviewOnRunComplete = true;
                    options = DefaultPreviewRenderOptions();
                    options.writeWav = false;
                }
                state.lastOutputPath = previewSelected ? "[memory preview]" : PathToUtf8(cfg.wavPath);

                state.logs.clear();
                state.lastPeak = 0.0;
                state.hasPeak = false;
                state.runOutputBuffer = previewSelected ? std::make_shared<SoundData>() : nullptr;
                state.runIsPreview = previewSelected;
                state.autoPlayPreviewOnRunComplete = previewSelected;
                AppendGUILog(state, previewSelected ? "[GUI] Preview Play started" : "[GUI] Play started");
                AppendGUILog(state, "[GUI] Effective Output: " + state.lastOutputPath);
                state.hasRun = false;
                state.stopRequested.store(false, std::memory_order_relaxed);
                state.running = true;
                state.runFuture = std::async(std::launch::async, [cfg, options, outBuffer = state.runOutputBuffer, &state]() {
                    return Run(cfg, options, &state.observer, outBuffer.get());
                    });
            }
        };

        ImGui::BeginDisabled(state.running);
        if (ImGui::Button("Play"))
        {
            startRun(false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Preview (Selected ch)"))
        {
            startRun(true);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Loop Preview", &state.previewLoop);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.running || !state.previewAudioReady || !state.previewRenderedSound);
        if (ImGui::Button("Replay Preview"))
        {
            std::string err;
            if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, err))
            {
                AppendGUILog(state, "[GUI] Preview replay started");
            }
            else
            {
                AppendGUILog(state, "[GUI] Preview replay failed: " + err);
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        const bool canStop = state.running || state.playback.playing.load(std::memory_order_relaxed);
        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button("Stop"))
        {
            if (state.playback.playing.load(std::memory_order_relaxed))
            {
                StopPreviewAudio(state.playback);
                AppendGUILog(state, "[GUI] Preview playback stopped");
            }
            if (state.running)
            {
                state.stopRequested.store(true, std::memory_order_relaxed);
                AppendGUILog(state, "[GUI] Stop requested (render cancellation signal sent)");
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.running);
        if (ImGui::Button("Close"))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Play: export full run / Play Preview: selected channel memory preview");
        ImGui::Separator();

        const float availY = ImGui::GetContentRegionAvail().y;
        const float reserveForLog = state.logPanelHeight + ImGui::GetFrameHeightWithSpacing() + 12.0f;
        const float bodyHeight = (std::max)(180.0f, availY - reserveForLog);
        ImGui::BeginChild("body_panel", ImVec2(0, bodyHeight), true);
        if (ImGui::BeginTable("layout_split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.56f);
            ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.44f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (state.presetDirty)
            {
                ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), "Preset: modified (unsaved)");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Preset: saved");
            }
            ImGui::BeginDisabled(state.running);
            auto presetGetter = [](void* data, int idx, const char** outText) -> bool
            {
                auto* items = static_cast<std::vector<std::string>*>(data);
                if (items == nullptr || idx < 0 || idx >= static_cast<int>(items->size()))
                {
                    return false;
                }
                *outText = (*items)[idx].c_str();
                return true;
            };
            if (ImGui::Combo("Preset", &state.presetIndex, presetGetter, &state.presetItems, static_cast<int>(state.presetItems.size())))
            {
                std::string err;
                if (ApplySelectedPresetPaths(state, err))
                {
                    state.presetDirty = true;
                }
                else
                {
                    AppendGUILog(state, "[GUI] Apply preset failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply Preset Paths"))
            {
                std::string err;
                if (ApplySelectedPresetPaths(state, err))
                {
                    state.presetDirty = true;
                }
                else
                {
                    AppendGUILog(state, "[GUI] Apply preset failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Defaults"))
            {
                InitGUIState(state);
                state.presetDirty = false;
            }

            ImGui::InputText("Preset Name", state.presetName, IM_ARRAYSIZE(state.presetName));
            ImGui::SameLine();
            if (ImGui::Button("Save Preset As"))
            {
                const std::filesystem::path p = FindProjectRootPath() / "config" / "presets" / (std::string(state.presetName) + ".json");
                std::string err;
                if (SavePresetDiff(state, p, err))
                {
                    state.lastPresetPath = PathToUtf8(p);
                    state.presetDirty = false;
                    RefreshPresetItems(state, state.presetName);
                    AppendGUILog(state, "[GUI] Preset saved: " + state.lastPresetPath);
                }
                else
                {
                    AppendGUILog(state, "[GUI] Preset save failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate Preset"))
            {
                std::string copyName = std::string(state.presetName) + "_copy";
                strncpy_s(state.presetName, sizeof(state.presetName), copyName.c_str(), _TRUNCATE);
                const std::filesystem::path p = FindProjectRootPath() / "config" / "presets" / (std::string(state.presetName) + ".json");
                std::string err;
                if (SavePresetDiff(state, p, err))
                {
                    state.lastPresetPath = PathToUtf8(p);
                    state.presetDirty = false;
                    RefreshPresetItems(state, state.presetName);
                    AppendGUILog(state, "[GUI] Preset duplicated: " + state.lastPresetPath);
                }
                else
                {
                    AppendGUILog(state, "[GUI] Preset duplicate failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Channel"))
            {
                EnsureChannelConfigs(state);
                EnsureChannelMixStates(state);
                AppConfig def = DefaultConfig();
                if (def.channelConfigs)
                {
                    (*state.channelConfigs)[state.selectedChannel] = (*def.channelConfigs)[state.selectedChannel];
                    if (def.channelMixStates)
                    {
                        (*state.channelMixStates)[state.selectedChannel] = (*def.channelMixStates)[state.selectedChannel];
                    }
                    state.presetDirty = true;
                    AppendGUILog(state, "[GUI] Channel reset: ch" + std::to_string(state.selectedChannel));
                }
            }
            if (!state.lastPresetPath.empty())
            {
                ImGui::Text("Last Preset: %s", state.lastPresetPath.c_str());
            }

            state.presetDirty |= ImGui::InputText("MIDI Path", state.midiPath, IM_ARRAYSIZE(state.midiPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse MIDI..."))
            {
                std::string selected;
                const wchar_t* midiFilter = L"MIDI Files (*.mid;*.midi)\0*.mid;*.midi\0All Files (*.*)\0*.*\0";
                if (BrowseOpenPath(state.midiPath, midiFilter, selected))
                {
                    strncpy_s(state.midiPath, sizeof(state.midiPath), selected.c_str(), _TRUNCATE);
                    state.presetDirty = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy MIDI"))
            {
                ImGui::SetClipboardText(state.midiPath);
            }
            {
                const std::string compact = CompactPathForUi(state.midiPath);
                ImGui::TextDisabled("%s", compact.c_str());
                if (ImGui::IsItemHovered() && std::strlen(state.midiPath) > 0)
                {
                    ImGui::SetTooltip("%s", state.midiPath);
                }
            }

            state.presetDirty |= ImGui::InputText("Output Path", state.wavPath, IM_ARRAYSIZE(state.wavPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse Output..."))
            {
                std::string selected;
                const wchar_t* wavFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
                if (BrowseSavePath(state.wavPath, wavFilter, L"wav", selected))
                {
                    strncpy_s(state.wavPath, sizeof(state.wavPath), selected.c_str(), _TRUNCATE);
                    state.presetDirty = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy Output"))
            {
                ImGui::SetClipboardText(state.wavPath);
            }
            {
                const std::string compact = CompactPathForUi(state.wavPath);
                ImGui::TextDisabled("%s", compact.c_str());
                if (ImGui::IsItemHovered() && std::strlen(state.wavPath) > 0)
                {
                    ImGui::SetTooltip("%s", state.wavPath);
                }
            }
            state.presetDirty |= ImGui::InputInt("Target Channel", &state.targetChannel);
            state.presetDirty |= ImGui::InputInt("Sample Rate", &state.sampleRate);
            state.presetDirty |= ImGui::InputInt("Initial Seconds", &state.initialSeconds);
            state.presetDirty |= ImGui::InputInt("Bits", &state.bits);
            state.presetDirty |= ImGui::InputFloat("Extra Release (sec)", &state.extraReleaseSec, 0.01f, 0.1f, "%.2f");
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            state.presetDirty |= ImGui::Combo("Default Wave", &state.defaultWave, waves, IM_ARRAYSIZE(waves));
            state.presetDirty |= ImGui::Checkbox("Serial Save (timestamp suffix)", &state.serialSave);
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(1);
            state.presetDirty |= DrawChannelEditor(state);
            if (!state.lastOutputPath.empty())
            {
                ImGui::Text("Last Output: %s", state.lastOutputPath.c_str());
            }
            AnalyzeRenderPeakFromLogs(state);
            if (state.hasPeak)
            {
                const float meter = static_cast<float>(std::clamp(state.lastPeak, 0.0, 1.0));
                ImGui::Text("Peak: %.4f", state.lastPeak);
                ImGui::ProgressBar(meter, ImVec2(-1, 0));
                if (state.lastPeak > 1.0)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f), "CLIP");
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::SetNextItemWidth(260.0f);
        ImGui::SliderFloat("Log Height", &state.logPanelHeight, 140.0f, 520.0f, "%.0f");
        ImGui::Text("Logs");
        ImGui::BeginChild("log_panel", ImVec2(0, state.logPanelHeight), true);
        {
            std::lock_guard<std::mutex> lock(state.logMutex);
            for (const std::string& line : state.logs)
            {
                ImGui::TextUnformatted(line.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
        ImGui::End();

        ImGui::Render();
        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (state.running && state.runFuture.valid())
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        state.lastRunExitCode = state.runFuture.get();
        state.hasRun = true;
        state.running = false;
    }

    {
        std::string err;
        if (!SaveGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state save failed: " + err);
        }
    }

    ShutdownPreviewAudio(state.playback);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

