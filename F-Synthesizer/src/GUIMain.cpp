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
#include <cwchar>

#define MINIAUDIO_IMPLEMENTATION
#include "third_party/miniaudio.h"

#include <windows.h>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppCore.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3dll.lib")
#ifdef _DEBUG
#pragma comment(lib, "imguid.lib")
#else
#pragma comment(lib, "imgui.lib")
#endif

namespace
{
struct PreviewPlaybackState
{
    ma_device device{};
    bool deviceReady = false;
    std::mutex mutex{};
    std::vector<float> pcm{};
    std::atomic<uint64_t> frameCursor{ 0 };
    std::atomic<bool> playing{ false };
    std::atomic<bool> loop{ false };
    ma_uint32 channels = 1;
    ma_uint32 sampleRate = 44100;
};

struct GuiState
{
    struct GuiRunObserver : IRunObserver
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
    GuiRunObserver observer{};
};

std::optional<std::string> ReadJsonString(const std::string& text, const std::string& key);
void EnsureChannelConfigs(GuiState& state);
void EnsureChannelMixStates(GuiState& state);
float UiScaleFromIndex(int idx);
const char* UiScaleLabelFromIndex(int idx);
void DrawStatusBadge(const GuiState& state);

std::string PathToUtf8(const std::filesystem::path& p)
{
    const auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::filesystem::path Utf8ToPath(const std::string& s)
{
    std::u8string u8;
    u8.assign(reinterpret_cast<const char8_t*>(s.data()),
        reinterpret_cast<const char8_t*>(s.data() + s.size()));
    return std::filesystem::path(u8);
}

std::wstring Utf8ToWide(const std::string& s)
{
    return Utf8ToPath(s).wstring();
}

std::string WideToUtf8(const std::wstring& w)
{
    return PathToUtf8(std::filesystem::path(w));
}

bool BrowseOpenPath(const std::string& initialPathUtf8, const wchar_t* filter, std::string& outPathUtf8)
{
    wchar_t fileBuf[2048]{};
    if (!initialPathUtf8.empty())
    {
        std::wstring initial = Utf8ToWide(initialPathUtf8);
        wcsncpy_s(fileBuf, initial.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileBuf));
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    outPathUtf8 = WideToUtf8(fileBuf);
    return true;
}

bool BrowseSavePath(const std::string& initialPathUtf8, const wchar_t* filter, const wchar_t* defExt, std::string& outPathUtf8)
{
    wchar_t fileBuf[2048]{};
    if (!initialPathUtf8.empty())
    {
        std::wstring initial = Utf8ToWide(initialPathUtf8);
        wcsncpy_s(fileBuf, initial.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileBuf));
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = defExt;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn))
    {
        return false;
    }

    outPathUtf8 = WideToUtf8(fileBuf);
    return true;
}

std::string CompactPathForUi(const std::string& s, size_t maxChars = 72)
{
    if (s.size() <= maxChars)
    {
        return s;
    }
    const size_t head = maxChars / 2 - 3;
    const size_t tail = maxChars - head - 3;
    return s.substr(0, head) + "..." + s.substr(s.size() - tail);
}

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

void AppendGuiLog(GuiState& state, const std::string& line)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    state.logs.push_back(line);
}

void PreviewAudioCallback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount)
{
    auto* playback = reinterpret_cast<PreviewPlaybackState*>(device->pUserData);
    float* out = reinterpret_cast<float*>(output);
    if (out == nullptr || playback == nullptr)
    {
        return;
    }

    std::fill(out, out + frameCount, 0.0f);
    std::lock_guard<std::mutex> lock(playback->mutex);
    if (!playback->playing.load(std::memory_order_relaxed) || playback->pcm.empty())
    {
        return;
    }

    const uint64_t totalFrames = static_cast<uint64_t>(playback->pcm.size());
    uint64_t cursor = playback->frameCursor.load(std::memory_order_relaxed);
    ma_uint32 written = 0;
    while (written < frameCount)
    {
        if (cursor >= totalFrames)
        {
            if (playback->loop.load(std::memory_order_relaxed))
            {
                cursor = 0;
            }
            else
            {
                playback->playing.store(false, std::memory_order_relaxed);
                break;
            }
        }

        const uint64_t remain = totalFrames - cursor;
        const ma_uint32 chunk = static_cast<ma_uint32>((std::min<uint64_t>)(remain, frameCount - written));
        std::memcpy(out + written, playback->pcm.data() + cursor, sizeof(float) * chunk);
        written += chunk;
        cursor += chunk;
    }

    playback->frameCursor.store(cursor, std::memory_order_relaxed);
}

bool EnsurePreviewAudioDevice(PreviewPlaybackState& playback, int sampleRate, std::string& err)
{
    std::lock_guard<std::mutex> lock(playback.mutex);

    if (playback.deviceReady && playback.sampleRate != static_cast<ma_uint32>(sampleRate))
    {
        ma_device_uninit(&playback.device);
        playback.deviceReady = false;
    }

    if (playback.deviceReady)
    {
        return true;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate = static_cast<ma_uint32>(sampleRate);
    config.dataCallback = PreviewAudioCallback;
    config.pUserData = &playback;

    if (ma_device_init(nullptr, &config, &playback.device) != MA_SUCCESS)
    {
        err = "Failed to initialize preview audio device.";
        return false;
    }
    if (ma_device_start(&playback.device) != MA_SUCCESS)
    {
        ma_device_uninit(&playback.device);
        err = "Failed to start preview audio device.";
        return false;
    }

    playback.deviceReady = true;
    playback.sampleRate = static_cast<ma_uint32>(sampleRate);
    return true;
}

void StopPreviewAudio(PreviewPlaybackState& playback)
{
    playback.playing.store(false, std::memory_order_relaxed);
    playback.frameCursor.store(0, std::memory_order_relaxed);
}

void ShutdownPreviewAudio(PreviewPlaybackState& playback)
{
    std::lock_guard<std::mutex> lock(playback.mutex);
    playback.playing.store(false, std::memory_order_relaxed);
    if (playback.deviceReady)
    {
        ma_device_uninit(&playback.device);
        playback.deviceReady = false;
    }
}

bool PlayPreviewAudio(PreviewPlaybackState& playback, const SoundData& sound, bool loop, std::string& err)
{
    if (sound.data.empty())
    {
        err = "Preview buffer is empty.";
        return false;
    }
    if (!EnsurePreviewAudioDevice(playback, sound.fs, err))
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(playback.mutex);
    playback.pcm.resize(sound.data.size());
    for (size_t i = 0; i < sound.data.size(); i++)
    {
        const double clamped = (std::max)(-1.0, (std::min)(1.0, sound.data[i]));
        playback.pcm[i] = static_cast<float>(clamped);
    }
    playback.frameCursor.store(0, std::memory_order_relaxed);
    playback.loop.store(loop, std::memory_order_relaxed);
    playback.playing.store(true, std::memory_order_relaxed);
    return true;
}

void CopyPath(char* dst, size_t dstSize, const std::filesystem::path& p)
{
    std::string s = PathToUtf8(p);
    if (dstSize == 0)
    {
        return;
    }
    strncpy_s(dst, dstSize, s.c_str(), _TRUNCATE);
}

bool NearlyEq(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

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
    if (std::holds_alternative<WaveformConfig>(src)) return 0;
    if (std::holds_alternative<NoiseConfig>(src)) return 1;
    if (std::holds_alternative<FmConfig>(src)) return 2;
    if (std::holds_alternative<DrumConfig>(src)) return 3;
    if (std::holds_alternative<DrumKitConfig>(src)) return 4;
    return 0;
}

SourceConfig DefaultSourceByType(int idx)
{
    switch (idx)
    {
    case 0: return WaveformConfig{ WaveType::Saw };
    case 1: return NoiseConfig{ NoiseType::White };
    case 2: return FmConfig{ WaveType::Sine, WaveType::Sine, 1.0, 2.0, 1.0, 1.0 };
    case 3: return DrumConfig{ DrumType::Kick };
    case 4:
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map) d.type = DrumType::None;
        kit.map[36] = DrumConfig{ DrumType::Kick };
        return kit;
    }
    default: return WaveformConfig{ WaveType::Saw };
    }
}

bool DrumConfigEquals(const DrumConfig& a, const DrumConfig& b)
{
    return a.type == b.type &&
        NearlyEq(a.gain, b.gain) &&
        NearlyEq(a.baseFreq, b.baseFreq) &&
        NearlyEq(a.pitchDrop, b.pitchDrop) &&
        NearlyEq(a.pitchDecaySec, b.pitchDecaySec) &&
        NearlyEq(a.toneFreq, b.toneFreq) &&
        NearlyEq(a.toneLevel, b.toneLevel) &&
        NearlyEq(a.noiseLevel, b.noiseLevel) &&
        NearlyEq(a.hpCut, b.hpCut) &&
        NearlyEq(a.lpCut, b.lpCut) &&
        a.toneWave == b.toneWave &&
        a.noiseType == b.noiseType;
}

bool SourceConfigEquals(const SourceConfig& a, const SourceConfig& b)
{
    if (a.index() != b.index())
    {
        return false;
    }
    return std::visit([&](const auto& av) -> bool
        {
            using T = std::decay_t<decltype(av)>;
            const auto* bv = std::get_if<T>(&b);
            if (bv == nullptr) return false;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                return av.wave == bv->wave;
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                return av.noise == bv->noise;
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                return av.carrierWave == bv->carrierWave &&
                    av.modWave == bv->modWave &&
                    NearlyEq(av.carrierRatio, bv->carrierRatio) &&
                    NearlyEq(av.modRatio, bv->modRatio) &&
                    NearlyEq(av.index, bv->index) &&
                    NearlyEq(av.outLevel, bv->outLevel);
            }
            else if constexpr (std::is_same_v<T, DrumConfig>)
            {
                return DrumConfigEquals(av, *bv);
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

bool ChannelConfigEquals(const ChannelConfig& a, const ChannelConfig& b)
{
    return NearlyEq(a.amp, b.amp) &&
        NearlyEq(a.attackSec, b.attackSec) &&
        NearlyEq(a.decaySec, b.decaySec) &&
        NearlyEq(a.sustainLevel, b.sustainLevel) &&
        NearlyEq(a.releaseSec, b.releaseSec) &&
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

void WriteJsonEscaped(std::ostream& out, const std::string& s)
{
    for (char c : s)
    {
        if (c == '\\') out << "\\\\";
        else if (c == '"') out << "\\\"";
        else if (c == '\n') out << "\\n";
        else out << c;
    }
}

std::string WaveToText(WaveType w)
{
    switch (w)
    {
    case WaveType::Sine: return "sine";
    case WaveType::Square: return "square";
    case WaveType::Saw: return "saw";
    case WaveType::Triangle: return "triangle";
    }
    return "saw";
}

std::string NoiseToText(NoiseType n)
{
    switch (n)
    {
    case NoiseType::White: return "white";
    case NoiseType::Pink: return "pink";
    case NoiseType::Brown: return "brown";
    case NoiseType::Blue: return "blue";
    }
    return "white";
}

std::string DrumTypeToText(DrumType d)
{
    switch (d)
    {
    case DrumType::None: return "none";
    case DrumType::Kick: return "kick";
    case DrumType::Snare: return "snare";
    case DrumType::Hat: return "hat";
    }
    return "none";
}

void WriteSourceJson(std::ostream& out, const SourceConfig& src, int indent)
{
    const std::string sp(indent, ' ');
    out << sp << "\"source\": {\n";
    std::visit([&](const auto& v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                out << sp << "  \"type\": \"waveform\",\n";
                out << sp << "  \"wave\": \"" << WaveToText(v.wave) << "\"\n";
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                out << sp << "  \"type\": \"noise\",\n";
                out << sp << "  \"noise\": \"" << NoiseToText(v.noise) << "\"\n";
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                out << sp << "  \"type\": \"fm\",\n";
                out << sp << "  \"carrierWave\": \"" << WaveToText(v.carrierWave) << "\",\n";
                out << sp << "  \"modWave\": \"" << WaveToText(v.modWave) << "\",\n";
                out << sp << "  \"carrierRatio\": " << v.carrierRatio << ",\n";
                out << sp << "  \"modRatio\": " << v.modRatio << ",\n";
                out << sp << "  \"index\": " << v.index << ",\n";
                out << sp << "  \"outLevel\": " << v.outLevel << "\n";
            }
            else if constexpr (std::is_same_v<T, DrumConfig>)
            {
                out << sp << "  \"type\": \"drum\",\n";
                out << sp << "  \"drumType\": \"" << DrumTypeToText(v.type) << "\",\n";
                out << sp << "  \"gain\": " << v.gain << ",\n";
                out << sp << "  \"baseFreq\": " << v.baseFreq << ",\n";
                out << sp << "  \"pitchDrop\": " << v.pitchDrop << ",\n";
                out << sp << "  \"pitchDecaySec\": " << v.pitchDecaySec << ",\n";
                out << sp << "  \"toneFreq\": " << v.toneFreq << ",\n";
                out << sp << "  \"toneLevel\": " << v.toneLevel << ",\n";
                out << sp << "  \"noiseLevel\": " << v.noiseLevel << ",\n";
                out << sp << "  \"hpCut\": " << v.hpCut << ",\n";
                out << sp << "  \"lpCut\": " << v.lpCut << ",\n";
                out << sp << "  \"toneWave\": \"" << WaveToText((WaveType)v.toneWave) << "\",\n";
                out << sp << "  \"noiseType\": \"" << NoiseToText((NoiseType)v.noiseType) << "\"\n";
            }
            else if constexpr (std::is_same_v<T, DrumKitConfig>)
            {
                out << sp << "  \"type\": \"drumkit\",\n";
                out << sp << "  \"map\": {\n";
                bool first = true;
                for (int note = 0; note < 128; note++)
                {
                    const auto& d = v.map[note];
                    if (d.type == DrumType::None) continue;
                    if (!first) out << ",\n";
                    first = false;
                    out << sp << "    \"" << note << "\": {\n";
                    out << sp << "      \"drumType\": \"" << DrumTypeToText(d.type) << "\",\n";
                    out << sp << "      \"gain\": " << d.gain << ",\n";
                    out << sp << "      \"baseFreq\": " << d.baseFreq << "\n";
                    out << sp << "    }";
                }
                out << "\n" << sp << "  }\n";
            }
        }, src);
    out << sp << "}";
}

bool SavePresetDiff(const GuiState& state, const std::filesystem::path& presetPath, std::string& err)
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

int FindPresetIndex(const GuiState& state, const std::string& name)
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

void RefreshPresetItems(GuiState& state, const std::string& preferName)
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

bool ApplySelectedPresetPaths(GuiState& state, std::string& err)
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

std::filesystem::path GuiStatePath()
{
    return FindProjectRootPath() / "config" / "gui_state.json";
}

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

bool LoadGuiStateFile(GuiState& state, std::string& err)
{
    const std::filesystem::path path = GuiStatePath();
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

    std::ostringstream oss;
    oss << fin.rdbuf();
    const std::string text = oss.str();

    if (auto v = ReadJsonString(text, "midiPath")) strncpy_s(state.midiPath, sizeof(state.midiPath), v->c_str(), _TRUNCATE);
    if (auto v = ReadJsonString(text, "wavPath")) strncpy_s(state.wavPath, sizeof(state.wavPath), v->c_str(), _TRUNCATE);
    if (auto v = ReadJsonInt(text, "targetChannel")) state.targetChannel = *v;
    if (auto v = ReadJsonInt(text, "sampleRate")) state.sampleRate = *v;
    if (auto v = ReadJsonInt(text, "initialSeconds")) state.initialSeconds = *v;
    if (auto v = ReadJsonInt(text, "bits")) state.bits = *v;
    if (auto v = ReadJsonFloat(text, "extraReleaseSec")) state.extraReleaseSec = *v;
    if (auto v = ReadJsonInt(text, "defaultWave")) state.defaultWave = *v;
    if (auto v = ReadJsonInt(text, "uiScaleIndex")) state.uiScaleIndex = *v;
    if (auto v = ReadJsonFloat(text, "logPanelHeight")) state.logPanelHeight = *v;
    if (auto v = ReadJsonInt(text, "presetIndex")) state.presetIndex = *v;
    if (auto v = ReadJsonBool(text, "serialSave")) state.serialSave = *v;
    if (auto v = ReadJsonBool(text, "previewLoop")) state.previewLoop = *v;
    if (auto v = ReadJsonInt(text, "selectedChannel")) state.selectedChannel = *v;
    if (auto v = ReadJsonInt(text, "selectedDrumNote")) state.selectedDrumNote = *v;
    if (auto v = ReadJsonString(text, "presetName")) strncpy_s(state.presetName, sizeof(state.presetName), v->c_str(), _TRUNCATE);
    if (auto v = ReadJsonString(text, "lastPresetPath")) state.lastPresetPath = *v;
    EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
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

bool SaveGuiStateFile(const GuiState& state, std::string& err)
{
    const std::filesystem::path path = GuiStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream fout(path, std::ios::binary | std::ios::trunc);
    if (!fout)
    {
        err = "failed to open " + path.string();
        return false;
    }

    fout << "{\n";
    fout << "  \"midiPath\": \"" << EscapeJson(state.midiPath) << "\",\n";
    fout << "  \"wavPath\": \"" << EscapeJson(state.wavPath) << "\",\n";
    fout << "  \"targetChannel\": " << state.targetChannel << ",\n";
    fout << "  \"sampleRate\": " << state.sampleRate << ",\n";
    fout << "  \"initialSeconds\": " << state.initialSeconds << ",\n";
    fout << "  \"bits\": " << state.bits << ",\n";
    fout << "  \"extraReleaseSec\": " << state.extraReleaseSec << ",\n";
    fout << "  \"defaultWave\": " << state.defaultWave << ",\n";
    fout << "  \"uiScaleIndex\": " << state.uiScaleIndex << ",\n";
    fout << "  \"logPanelHeight\": " << state.logPanelHeight << ",\n";
    fout << "  \"presetIndex\": " << state.presetIndex << ",\n";
    fout << "  \"serialSave\": " << (state.serialSave ? "true" : "false") << ",\n";
    fout << "  \"previewLoop\": " << (state.previewLoop ? "true" : "false") << ",\n";
    fout << "  \"selectedChannel\": " << state.selectedChannel << ",\n";
    fout << "  \"selectedDrumNote\": " << state.selectedDrumNote << ",\n";
    fout << "  \"presetName\": \"" << EscapeJson(state.presetName) << "\",\n";
    fout << "  \"lastPresetPath\": \"" << EscapeJson(state.lastPresetPath) << "\",\n";
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelMixState mix = (state.channelMixStates != nullptr)
            ? (*state.channelMixStates)[ch]
            : ChannelMixState{};
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

std::filesystem::path BuildSerialWavPath(const std::filesystem::path& basePath)
{
    std::error_code ec;
    std::filesystem::create_directories(basePath.parent_path(), ec);

    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmLocal{};
    localtime_s(&tmLocal, &tt);

    char stamp[32]{};
    std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmLocal);

    const std::string stem = basePath.stem().string();
    const std::string ext = basePath.extension().string().empty() ? ".wav" : basePath.extension().string();
    std::filesystem::path candidate = basePath.parent_path() / (stem + "_" + stamp + ext);
    for (int i = 1; i <= 99 && std::filesystem::exists(candidate); i++)
    {
        candidate = basePath.parent_path() / (stem + "_" + stamp + "_" + std::to_string(i) + ext);
    }
    return candidate;
}

std::filesystem::path BuildPreviewWavPath(const std::filesystem::path& basePath, int channel)
{
    const std::string stem = basePath.stem().string();
    const std::string ext = basePath.extension().string().empty() ? ".wav" : basePath.extension().string();
    return basePath.parent_path() / (stem + "_preview_ch" + std::to_string(channel) + ext);
}

AppConfig BuildConfigFromGui(const GuiState& state)
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

bool ValidateBeforeRun(const GuiState& state, std::string& err)
{
    const std::string midi = state.midiPath;
    const std::string wav = state.wavPath;
    if (midi.empty())
    {
        err = "MIDI Path is empty.";
        return false;
    }
    if (wav.empty())
    {
        err = "Output Path is empty.";
        return false;
    }
    const std::filesystem::path wavPath = Utf8ToPath(wav);
    if (wavPath.has_filename() && !wavPath.extension().empty())
    {
        if (std::filesystem::is_directory(wavPath))
        {
            err = "Output Path points to a directory, not a .wav file.";
            return false;
        }
    }
    else
    {
        err = "Output Path must include a .wav filename.";
        return false;
    }
    if (!std::filesystem::exists(Utf8ToPath(midi)))
    {
        err = "MIDI file not found: " + midi;
        return false;
    }
    if (state.targetChannel < -1 || state.targetChannel > 15)
    {
        err = "Target Channel must be -1 or 0..15.";
        return false;
    }
    if (state.sampleRate <= 0)
    {
        err = "Sample Rate must be positive.";
        return false;
    }
    if (state.initialSeconds <= 0)
    {
        err = "Initial Seconds must be positive.";
        return false;
    }
    if (state.bits != 16)
    {
        err = "Bits must be 16 in current implementation.";
        return false;
    }
    return true;
}

void InitGuiState(GuiState& state)
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

void RepairGuiStatePathsIfNeeded(GuiState& state)
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
            AppendGuiLog(state, "[GUI] Invalid mix state detected and clamped: ch" + std::to_string(ch));
        }
    }
    if (repaired)
    {
        AppendGuiLog(state, "[GUI] Detected invalid saved state. Recovered to safe defaults.");
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

void EnsureChannelConfigs(GuiState& state)
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

void EnsureChannelMixStates(GuiState& state)
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

void AnalyzeRenderPeakFromLogs(GuiState& state)
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

void ActivateSoloPreview(GuiState& state, int channel)
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
    AppendGuiLog(state, "[GUI] Solo Preview ON: ch" + std::to_string(channel));
}

void DeactivateSoloPreview(GuiState& state)
{
    if (!state.soloPreviewActive || !state.channelMixStates)
    {
        return;
    }
    *state.channelMixStates = state.soloPreviewBackup;
    AppendGuiLog(state, "[GUI] Solo Preview OFF: restore previous mix state");
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
}

bool DrawChannelEditor(GuiState& state)
{
    bool changed = false;
    EnsureChannelConfigs(state);
    EnsureChannelMixStates(state);
    state.selectedChannel = std::clamp(state.selectedChannel, 0, 15);
    ChannelConfig& chCfg = (*state.channelConfigs)[state.selectedChannel];
    ChannelMixState& chMix = (*state.channelMixStates)[state.selectedChannel];

    ImGui::Separator();
    ImGui::Text("Channel Editor");
    changed |= ImGui::InputInt("Edit Channel (0-15)", &state.selectedChannel);
    state.selectedChannel = std::clamp(state.selectedChannel, 0, 15);
    chMix = (*state.channelMixStates)[state.selectedChannel];

    ImGui::Separator();
    ImGui::Text("Channel Mix Monitor (0-15)");
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
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
        ImGui::PushID(ch);
        if ((ch % 4) != 0)
        {
            ImGui::SameLine();
        }
        ImGui::BeginGroup();
        ImGui::Text("ch%d", ch);
        changed |= ImGui::Checkbox("M", &mix.mute);
        ImGui::SameLine();
        changed |= ImGui::Checkbox("S", &mix.solo);
        changed |= sliderMix("L", mix.level, 0.0f, 2.0f);
        changed |= sliderMix("P", mix.pan, -1.0f, 1.0f);
        changed |= sliderMix("G", mix.gain, 0.0f, 4.0f);
        ImGui::EndGroup();
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("Selected Channel Mix (ch%d)", state.selectedChannel);
    changed |= ImGui::Checkbox("Mute", &chMix.mute);
    ImGui::SameLine();
    changed |= ImGui::Checkbox("Solo", &chMix.solo);
    changed |= sliderMix("Level", chMix.level, 0.0f, 2.0f);
    changed |= sliderMix("Pan", chMix.pan, -1.0f, 1.0f);
    changed |= sliderMix("Gain", chMix.gain, 0.0f, 4.0f);

    changed |= ImGui::InputDouble("Ch Amp", &chCfg.amp, 0.01, 0.1, "%.3f");
    changed |= ImGui::InputDouble("Ch Attack", &chCfg.attackSec, 0.01, 0.1, "%.3f");
    changed |= ImGui::InputDouble("Ch Decay", &chCfg.decaySec, 0.01, 0.1, "%.3f");
    changed |= ImGui::InputDouble("Ch Sustain", &chCfg.sustainLevel, 0.01, 0.1, "%.3f");
    changed |= ImGui::InputDouble("Ch Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");

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

void DrawStatusBadge(const GuiState& state)
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

int RunGuiApp()
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
    GuiState state{};
    InitGuiState(state);

    {
        std::string err;
        if (!LoadGuiStateFile(state, err))
        {
            AppendGuiLog(state, "[GUI] gui_state load failed: " + err);
        }
        else
        {
            AppendGuiLog(state, "[GUI] gui_state loaded: " + GuiStatePath().string());
        }
        RepairGuiStatePathsIfNeeded(state);
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
            AppendGuiLog(state, std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
            if (state.runIsPreview)
            {
                if (state.lastRunExitCode == 0 &&
                    state.runOutputBuffer != nullptr &&
                    !state.runOutputBuffer->data.empty())
                {
                    state.previewRenderedSound = state.runOutputBuffer;
                    state.previewAudioReady = true;
                    AppendGuiLog(state, "[GUI] Preview audio buffer ready");
                    if (state.autoPlayPreviewOnRunComplete)
                    {
                        std::string playErr;
                        if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, playErr))
                        {
                            AppendGuiLog(state, "[GUI] Preview playback started");
                        }
                        else
                        {
                            AppendGuiLog(state, "[GUI] Preview playback failed: " + playErr);
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

        ImGui::Begin("F-Synthesizer GUI");
        DrawStatusBadge(state);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16.0f);
        ImGui::TextUnformatted("UI Scale");
        ImGui::SameLine();
        const char* uiScales[] = { "100%", "125%", "150%" };
        if (ImGui::Combo("##ui_scale", &state.uiScaleIndex, uiScales, IM_ARRAYSIZE(uiScales)))
        {
            AppendGuiLog(state, std::string("[GUI] UI scale changed: ") + UiScaleLabelFromIndex(state.uiScaleIndex));
        }
        ImGui::Separator();
        auto startRun = [&](bool previewSelected)
        {
            std::string validationError;
            if (!ValidateBeforeRun(state, validationError))
            {
                state.hasRun = true;
                state.lastRunExitCode = 1;
                AppendGuiLog(state, "[GUI] Validation failed: " + validationError);
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
                    AppendGuiLog(state, "[GUI] Previous preview playback stopped for new run");
                }
                AppConfig cfg = BuildConfigFromGui(state);
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
                AppendGuiLog(state, previewSelected ? "[GUI] Preview Play started" : "[GUI] Play started");
                AppendGuiLog(state, "[GUI] Effective Output: " + state.lastOutputPath);
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
                AppendGuiLog(state, "[GUI] Preview replay started");
            }
            else
            {
                AppendGuiLog(state, "[GUI] Preview replay failed: " + err);
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
                AppendGuiLog(state, "[GUI] Preview playback stopped");
            }
            if (state.running)
            {
                state.stopRequested.store(true, std::memory_order_relaxed);
                AppendGuiLog(state, "[GUI] Stop requested (render cancellation signal sent)");
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
                    AppendGuiLog(state, "[GUI] Apply preset failed: " + err);
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
                    AppendGuiLog(state, "[GUI] Apply preset failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Defaults"))
            {
                InitGuiState(state);
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
                    AppendGuiLog(state, "[GUI] Preset saved: " + state.lastPresetPath);
                }
                else
                {
                    AppendGuiLog(state, "[GUI] Preset save failed: " + err);
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
                    AppendGuiLog(state, "[GUI] Preset duplicated: " + state.lastPresetPath);
                }
                else
                {
                    AppendGuiLog(state, "[GUI] Preset duplicate failed: " + err);
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
                    AppendGuiLog(state, "[GUI] Channel reset: ch" + std::to_string(state.selectedChannel));
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
            ImGui::BeginDisabled(state.running);
            if (state.soloPreviewActive)
            {
                if (ImGui::Button("Solo Preview Off"))
                {
                    DeactivateSoloPreview(state);
                    state.presetDirty = true;
                }
                ImGui::SameLine();
                ImGui::Text("Solo Preview: ch%d", state.soloPreviewChannel);
            }
            else
            {
                if (ImGui::Button("Solo Preview On (Selected ch)"))
                {
                    ActivateSoloPreview(state, state.selectedChannel);
                    state.presetDirty = true;
                }
            }
            ImGui::EndDisabled();
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
        if (!SaveGuiStateFile(state, err))
        {
            AppendGuiLog(state, "[GUI] gui_state save failed: " + err);
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
