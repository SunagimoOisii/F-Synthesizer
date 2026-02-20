#include <string>
#include <cstring>
#include <vector>
#include <mutex>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <regex>
#include <optional>
#include <ctime>

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
enum class PresetKind
{
    Solstice = 0,
    Frog = 1,
};

struct GuiState
{
    struct GuiRunObserver : IRunObserver
    {
        std::mutex* logMutex = nullptr;
        std::vector<std::string>* logs = nullptr;

        void OnLogLine(const std::string& line) override
        {
            if (logMutex == nullptr || logs == nullptr)
            {
                return;
            }
            std::lock_guard<std::mutex> lock(*logMutex);
            logs->push_back(line);
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
    int presetIndex = 0;
    int lastRunExitCode = 0;
    bool hasRun = false;
    bool running = false;
    bool stopRequested = false;
    bool serialSave = false;
    std::string lastOutputPath{};
    std::future<int> runFuture{};
    std::mutex logMutex{};
    std::vector<std::string> logs{};
    GuiRunObserver observer{};
};

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
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, nullptr, ranges);
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

void CopyPath(char* dst, size_t dstSize, const std::filesystem::path& p)
{
    std::string s = PathToUtf8(p);
    if (dstSize == 0)
    {
        return;
    }
    strncpy_s(dst, dstSize, s.c_str(), _TRUNCATE);
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

void ApplyPresetPath(GuiState& state)
{
    const std::filesystem::path root = FindProjectRootPath();
    if (state.presetIndex == static_cast<int>(PresetKind::Frog))
    {
        CopyPath(state.midiPath, sizeof(state.midiPath), root / "assets" / "midi" / "test_frog.mid");
        CopyPath(state.wavPath, sizeof(state.wavPath), root / "output" / "frog.wav");
        return;
    }

    CopyPath(state.midiPath, sizeof(state.midiPath), root / "assets" / "midi" / "solstice_intro.mid");
    CopyPath(state.wavPath, sizeof(state.wavPath), root / "output" / "solstice.wav");
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
    if (auto v = ReadJsonInt(text, "presetIndex")) state.presetIndex = *v;
    if (auto v = ReadJsonBool(text, "serialSave")) state.serialSave = *v;

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
    fout << "  \"presetIndex\": " << state.presetIndex << ",\n";
    fout << "  \"serialSave\": " << (state.serialSave ? "true" : "false") << "\n";
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
    AppConfig cfg = DefaultConfig();
    CopyPath(state.midiPath, sizeof(state.midiPath), cfg.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), cfg.wavPath);
    state.targetChannel = cfg.targetChannel;
    state.sampleRate = cfg.sampleRate;
    state.initialSeconds = cfg.initialSeconds;
    state.bits = cfg.bits;
    state.extraReleaseSec = static_cast<float>(cfg.extraReleaseSec);
    state.defaultWave = 2;
    state.presetIndex = 0;
    state.running = false;
    state.stopRequested = false;
    state.hasRun = false;
    state.lastRunExitCode = 0;
    state.serialSave = false;
    state.lastOutputPath.clear();
    state.logs.clear();
    state.observer.logMutex = &state.logMutex;
    state.observer.logs = &state.logs;
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
    if (repaired)
    {
        AppendGuiLog(state, "[GUI] Detected invalid saved paths. Recovered to default paths.");
    }
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
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("F-Synthesizer GUI");
        ImGui::Text("Phase 5: Release Ready");
        ImGui::Separator();

        ImGui::BeginDisabled(state.running);
        const char* presets[] = { "solstice", "frog" };
        if (ImGui::Combo("Preset", &state.presetIndex, presets, IM_ARRAYSIZE(presets)))
        {
            ApplyPresetPath(state);
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply Preset Paths"))
        {
            ApplyPresetPath(state);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults"))
        {
            InitGuiState(state);
        }

        ImGui::InputText("MIDI Path", state.midiPath, IM_ARRAYSIZE(state.midiPath));
        ImGui::InputText("Output Path", state.wavPath, IM_ARRAYSIZE(state.wavPath));
        ImGui::InputInt("Target Channel", &state.targetChannel);
        ImGui::InputInt("Sample Rate", &state.sampleRate);
        ImGui::InputInt("Initial Seconds", &state.initialSeconds);
        ImGui::InputInt("Bits", &state.bits);
        ImGui::InputFloat("Extra Release (sec)", &state.extraReleaseSec, 0.01f, 0.1f, "%.2f");
        const char* waves[] = { "sine", "square", "saw", "triangle" };
        ImGui::Combo("Default Wave", &state.defaultWave, waves, IM_ARRAYSIZE(waves));
        ImGui::Checkbox("Serial Save (timestamp suffix)", &state.serialSave);

        if (ImGui::Button("Run"))
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
                AppConfig cfg = BuildConfigFromGui(state);
                if (state.serialSave)
                {
                    cfg.wavPath = BuildSerialWavPath(cfg.wavPath);
                }
                state.lastOutputPath = cfg.wavPath.string();

                state.logs.clear();
                AppendGuiLog(state, "[GUI] Run started");
                AppendGuiLog(state, "[GUI] Effective Output: " + state.lastOutputPath);
                state.hasRun = false;
                state.stopRequested = false;
                state.running = true;
                state.runFuture = std::async(std::launch::async, [cfg, &state]() {
                    return Run(cfg, &state.observer);
                    });
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!state.running);
        if (ImGui::Button("Stop"))
        {
            state.stopRequested = true;
            AppendGuiLog(state, "[GUI] Stop requested (current engine does not support cancellation yet)");
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (state.running)
        {
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Status: Running...");
        }
        else if (state.hasRun)
        {
            if (state.lastRunExitCode == 0)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Last Run: Success");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Last Run: Failed (%d)", state.lastRunExitCode);
            }
        }
        if (!state.lastOutputPath.empty())
        {
            ImGui::Text("Last Output: %s", state.lastOutputPath.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Logs");
        ImGui::BeginChild("log_panel", ImVec2(0, 240), true);
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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
