#include "StudioWidgets.h"
#include "AppCore.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <vector>
#include <cstring>
#include "io/PlatformPaths.h"

namespace studio
{
namespace
{
Fonts fonts_;
GLuint iconTexture = 0;
} // namespace
const Fonts &GetFonts()
{
    return fonts_;
}
void ShutdownResources()
{
    glDeleteTextures(1, &iconTexture);
    iconTexture = 0;
    fonts_ = {};
}
ImVec4 vec(ImU32 c)
{
    return ImGui::ColorConvertU32ToFloat4(c);
}
void box(float x, float y, float w, float h, ImU32 c)
{
    ImGui::GetWindowDrawList()->AddRectFilled({x, y}, {x + w, y + h}, c);
}
void line(float x, float y, float x2, float y2, ImU32 c, float thick)
{
    ImGui::GetWindowDrawList()->AddLine({x, y}, {x2, y2}, c, thick);
}
void text(float x, float y, const char *value, ImU32 c, ImFont *f)
{
    f = f ? f : fonts_.body;
    ImGui::GetWindowDrawList()->AddText(f, f->FontSize, {x, y}, c, value);
}
void at(float x, float y)
{
    ImGui::SetCursorScreenPos({x, y});
}
void clipped(float x, float y, float width, const char *value, ImU32 c, ImFont *f)
{
    f = f ? f : fonts_.body;
    std::string display = value;
    if (f->CalcTextSizeA(f->FontSize, 10000, 0, value).x > width)
    {
        do
        {
            size_t cut = display.size() - 1;
            while (cut > 0 && (static_cast<unsigned char>(display[cut]) & 0xc0) == 0x80)
                --cut;
            display.resize(cut);
        } while (!display.empty() && f->CalcTextSizeA(f->FontSize, 10000, 0, (display + "…").c_str()).x > width);
        display += "…";
    }
    ImGui::GetWindowDrawList()->PushClipRect({x, y}, {x + width, y + f->FontSize + 6}, true);
    text(x, y, display.c_str(), c, f);
    ImGui::GetWindowDrawList()->PopClipRect();
}
void icon(Icon id, float x, float y, float size, ImU32 tint)
{
    const float u = (static_cast<int>(id) * 64 + 4) / 832.f, v = 4 / 64.f;
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(iconTexture), {x, y}, {x + size, y + size}, {u, v},
                                         {u + 56 / 832.f, 60 / 64.f}, tint);
}
bool loadIcons()
{
    HBITMAP handle =
        static_cast<HBITMAP>(LoadImageW(nullptr, (FindProjectRootPath() / "assets" / "ui" / "icon-atlas.bmp").c_str(),
                                        IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
    if (!handle)
        return false;
    BITMAP bm{};
    GetObject(handle, sizeof(bm), &bm);
    if (!bm.bmBits || bm.bmWidth != 832 || bm.bmHeight < 64 || bm.bmBitsPixel < 24)
    {
        DeleteObject(handle);
        return false;
    }
    std::vector<unsigned char> rgba(832 * 64 * 4);
    auto *bytes = static_cast<unsigned char *>(bm.bmBits);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 832; ++x)
        {
            auto *p = bytes + (bm.bmHeight - 1 - y) * bm.bmWidthBytes + x * (bm.bmBitsPixel / 8);
            size_t dst = (y * 832 + x) * 4;
            rgba[dst] = rgba[dst + 1] = rgba[dst + 2] = 255;
            rgba[dst + 3] = p[0];
        }
    DeleteObject(handle);
    glGenTextures(1, &iconTexture);
    glBindTexture(GL_TEXTURE_2D, iconTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 832, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return true;
}
bool button(const char *label, float x, float y, float w, float h, bool chosen, bool primary, int glyph)
{
    at(x, y);
    ImGui::PushStyleColor(ImGuiCol_Button, vec(primary ? accent : chosen ? raised : panel));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, vec(primary ? fg : raised));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, vec(primary ? accent : edge));
    ImGui::PushStyleColor(ImGuiCol_Border, vec(chosen ? accent : edge));
    ImGui::PushStyleColor(ImGuiCol_Text, vec(primary ? scope : fg));
    std::string id = std::string("##") + label;
    bool clicked = ImGui::Button(id.c_str(), {w, h});
    ImGui::PopStyleColor(5);
    const float textWidth = fonts_.body->CalcTextSizeA(fonts_.body->FontSize, 1000, 0, label).x;
    const float groupWidth = textWidth + (glyph >= 0 ? 26 : 0), left = x + (w - groupWidth) / 2;
    auto tint = [](ImU32 value) {
        ImVec4 v = vec(value);
        v.w *= ImGui::GetStyle().Alpha;
        return ImGui::ColorConvertFloat4ToU32(v);
    };
    if (glyph >= 0)
        icon(static_cast<Icon>(glyph), left, y + (h - 18) / 2, 18, tint(primary ? scope : accent));
    text(left + (glyph >= 0 ? 26 : 0), y + (h - fonts_.body->FontSize) / 2, label, tint(primary ? scope : fg));
    return clicked;
}
void style()
{
    auto &s = ImGui::GetStyle();
    s.WindowPadding = {18, 18};
    s.FramePadding = {10, 7};
    s.ItemSpacing = {8, 8};
    s.WindowRounding = s.ChildRounding = s.FrameRounding = s.PopupRounding = 0;
    s.FrameBorderSize = 1;
    s.ScrollbarSize = 12;
    s.Colors[ImGuiCol_WindowBg] = vec(bg);
    s.Colors[ImGuiCol_ChildBg] = vec(bg);
    s.Colors[ImGuiCol_PopupBg] = vec(panel);
    s.Colors[ImGuiCol_Text] = vec(fg);
    s.Colors[ImGuiCol_TextDisabled] = vec(muted);
    s.Colors[ImGuiCol_Border] = vec(edge);
    s.Colors[ImGuiCol_FrameBg] = vec(scope);
    s.Colors[ImGuiCol_FrameBgHovered] = vec(raised);
    s.Colors[ImGuiCol_FrameBgActive] = vec(raised);
    s.Colors[ImGuiCol_Button] = vec(panel);
    s.Colors[ImGuiCol_ButtonHovered] = vec(raised);
    s.Colors[ImGuiCol_ButtonActive] = vec(edge);
    s.Colors[ImGuiCol_Header] = vec(raised);
    s.Colors[ImGuiCol_HeaderHovered] = vec(raised);
    s.Colors[ImGuiCol_HeaderActive] = vec(edge);
    s.Colors[ImGuiCol_CheckMark] = vec(accent);
    s.Colors[ImGuiCol_NavCursor] = vec(accent);
    s.Colors[ImGuiCol_ScrollbarBg] = vec(scope);
    s.Colors[ImGuiCol_ScrollbarGrab] = vec(edge);
    s.Colors[ImGuiCol_TitleBg] = vec(panel);
    s.Colors[ImGuiCol_TitleBgActive] = vec(raised);
    s.Colors[ImGuiCol_ModalWindowDimBg] = vec(color(7, 14, 19, 170));
    s.GrabMinSize = 7;
    s.Colors[ImGuiCol_SliderGrab] = vec(color(175, 217, 208, 90));
    s.Colors[ImGuiCol_SliderGrabActive] = vec(color(175, 217, 208, 120));
    s.Colors[ImGuiCol_TextSelectedBg] = vec(raised);
    s.Colors[ImGuiCol_TableHeaderBg] = vec(raised);
    s.Colors[ImGuiCol_TableBorderStrong] = vec(edge);
    s.Colors[ImGuiCol_TableBorderLight] = vec(edge);
}

void InitializeResources()
{
    auto &io = ImGui::GetIO();
    static ImVector<ImWchar> glyphs;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    builder.AddText("…");
    builder.BuildRanges(&glyphs);
    const auto directory = FindProjectRootPath() / "assets" / "ui";
    // Read through filesystem paths: the user's workspace contains non-ASCII characters.
    auto add = [&](const char *name, float size) -> ImFont * {
        std::ifstream file(directory / name, std::ios::binary);
        std::vector<char> data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        if (data.empty())
            return nullptr;
        void *owned = IM_ALLOC(data.size());
        std::memcpy(owned, data.data(), data.size());
        return io.Fonts->AddFontFromMemoryTTF(owned, static_cast<int>(data.size()), size, nullptr, glyphs.Data);
    };
    fonts_.body = add("MPLUS1-Regular.ttf", 21);
    fonts_.fontSmall = add("MPLUS1-Regular.ttf", 18);
    fonts_.heading = add("MPLUS1-Medium.ttf", 24);
    fonts_.title = add("MPLUS1-Medium.ttf", 30);
    if (!fonts_.body)
        for (const char *font : {"C:/Windows/Fonts/meiryo.ttc", "C:/Windows/Fonts/YuGothM.ttc"})
            if (std::filesystem::exists(font))
            {
                fonts_.body = io.Fonts->AddFontFromFileTTF(font, 21, nullptr, glyphs.Data);
                if (fonts_.body)
                    break;
            }
    if (!fonts_.body)
        fonts_.body = io.Fonts->AddFontDefault();
    if (!fonts_.fontSmall)
        fonts_.fontSmall = fonts_.body;
    if (!fonts_.heading)
        fonts_.heading = fonts_.body;
    if (!fonts_.title)
        fonts_.title = fonts_.heading;
    io.FontDefault = fonts_.body;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    loadIcons();
}
void capture(const std::filesystem::path &path, int width, int height)
{
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
    glReadPixels(0, 0, width, height, 0x80E1 /* BGRA */, GL_UNSIGNED_BYTE, pixels.data());
    BITMAPFILEHEADER file{};
    file.bfType = 0x4D42;
    file.bfOffBits = sizeof(file) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + static_cast<DWORD>(pixels.size());
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = height;
    info.biPlanes = 1;
    info.biBitCount = 32;
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(&file), sizeof(file));
    output.write(reinterpret_cast<const char *>(&info), sizeof(info));
    output.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
}
} // namespace studio
