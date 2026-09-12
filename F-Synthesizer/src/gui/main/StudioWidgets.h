#pragma once
#include <imgui.h>
#include <filesystem>

namespace studio
{
enum Icon
{
    MusicIcon,
    PianoIcon,
    GuitarIcon,
    LayersIcon,
    DrumIcon,
    StarIcon,
    PlayIcon,
    SaveIcon,
    PauseIcon,
    SearchIcon,
    DownIcon,
    SparklesIcon,
    ChipIcon
};
constexpr ImU32 color(int r, int g, int b, int a = 255)
{
    return IM_COL32(r, g, b, a);
}
inline constexpr ImU32 bg = color(26, 37, 44), panel = color(32, 47, 55), raised = color(53, 67, 76),
                       edge = color(69, 85, 95), fg = color(230, 233, 230), muted = color(159, 178, 187),
                       accent = color(175, 217, 208), pendingColor = color(225, 173, 117), scope = color(17, 28, 36);
struct Fonts
{
    ImFont *body = nullptr, *fontSmall = nullptr, *heading = nullptr, *title = nullptr;
};
const Fonts &GetFonts();
// Called with the application's OpenGL/ImGui contexts current, on the GUI thread.
void InitializeResources();
void ShutdownResources();
void capture(const std::filesystem::path &path, int width, int height);
void style();
ImVec4 vec(ImU32 color);
void box(float x, float y, float w, float h, ImU32 color);
void line(float x, float y, float x2, float y2, ImU32 color = edge, float thick = 1);
void text(float x, float y, const char *value, ImU32 color = fg, ImFont *font = nullptr);
void at(float x, float y);
void clipped(float x, float y, float width, const char *value, ImU32 color = fg, ImFont *font = nullptr);
void icon(Icon id, float x, float y, float size, ImU32 tint = accent);
bool button(const char *label, float x, float y, float w, float h, bool chosen = false, bool primary = false,
            int glyph = -1);
} // namespace studio
