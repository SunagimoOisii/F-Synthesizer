#include "StudioPanels.h"
#include "gui/GUIActions.h"
#include "gui/GUIPresetIO.h"
#include "io/PlatformPaths.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
namespace studio
{
int categoryIndex(const std::string &name)
{
    for (int i = 0; i < 8; ++i)
        if (name == categories[i])
            return i;
    return 7;
}
std::string toneName(const InstrumentConfig &instrument)
{
    return instrument.displayName.empty() ? "名前のない音色" : instrument.displayName;
}
namespace
{
bool matches(std::string name, std::string query)
{
    auto lower = [](unsigned char c) { return static_cast<char>(c < 128 ? std::tolower(c) : c); };
    std::transform(name.begin(), name.end(), name.begin(), lower);
    std::transform(query.begin(), query.end(), query.begin(), lower);
    return name.find(query) != std::string::npos;
}
std::string pitchName(int note)
{
    const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return std::string(names[std::clamp(note, 0, 127) % 12]) + std::to_string(note / 12 - 1);
}
void dial(GUIState &s, int index, const char *label, float x, float y, float scale = .88f)
{
    auto &part = s.tones[s.pianoRoll.displayChannel];
    if (!gui::ToneControlSupported(gui::AudibleInstrument(s, s.pianoRoll.displayChannel).sound, index))
        return;
    auto edit = [&](float value) {
        part.draft.values[index] = std::clamp(value, -1.f, 1.f);
        gui::UpdateToneControls(s);
    };
    ImGui::PushID(index + 40);
    // 比較表示は採用前の値。試聴中へ戻るまで編集を受け付けず、別の音を誤って調整しない。
    ImGui::BeginDisabled(part.compare);
    at(x, y);
    ImGui::InvisibleButton("dial", {100 * scale, 101 * scale}, ImGuiButtonFlags_EnableNav);
    const bool active = ImGui::IsItemActive(), hover = ImGui::IsItemHovered(), focus = ImGui::IsItemFocused();
    if (hover || active)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    const auto &io = ImGui::GetIO();
    const float fine = io.KeyShift ? .1f : 1.f;
    if (ImGui::IsItemActivated())
        gui::BeginToneEdit(s);
    if (active && ImGui::IsMouseDragging(0, 2))
        edit(part.draft.values[index] - io.MouseDelta.y / 130.f * fine);
    if (ImGui::IsItemDeactivated())
        gui::FinishToneEdit(s);
    float delta = hover ? io.MouseWheel * .04f : 0;
    if (focus)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            delta += .02f;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
            delta -= .02f;
    }
    if (delta)
    {
        gui::BeginToneEdit(s);
        edit(part.draft.values[index] + delta * fine);
        gui::FinishToneEdit(s);
    }
    if (hover && ImGui::IsMouseDoubleClicked(0))
    {
        gui::BeginToneEdit(s);
        edit(0);
        gui::FinishToneEdit(s);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(part.compare ? "採用前の音色を表示中\n「試聴中へ戻る」で調整を再開" :
                          "上下にドラッグ（上で増やす / 下で減らす）\n"
                          "0は音色の基準 / ダブルクリックで基準に戻す\n"
                          "Shiftで微調整 / 右クリックで数値入力");
    if (!part.compare && ImGui::BeginPopupContextItem("value"))
    {
        ImGui::TextUnformatted(label);
        float value = part.draft.values[index] * 100;
        ImGui::SetNextItemWidth(180);
        const bool changed = ImGui::InputFloat("##value", &value, 1, 5, "%+.0f");
        if (ImGui::IsItemActivated())
            gui::BeginToneEdit(s);
        if (changed)
            edit(value / 100);
        if (ImGui::IsItemDeactivated())
            gui::FinishToneEdit(s);
        ImGui::EndPopup();
    }
    const float value = (part.compare ? part.adopted : part.draft).values[index];
    const float normalized = (value + 1) * .5f;
    const ImVec2 center{x + 50 * scale, y + 50 * scale};
    const ImU32 tint = part.compare ? muted : accent;
    constexpr float startAngle = 2.35f, sweepAngle = 4.72f;
    for (int i = 0; i <= 4; ++i)
    {
        const float angle = startAngle + i / 4.f * sweepAngle;
        const bool reference = i == 2;
        const float innerRadius = (reference ? 36.f : 40.f) * scale;
        line(center.x + std::cos(angle) * innerRadius, center.y + std::sin(angle) * innerRadius,
             center.x + std::cos(angle) * 45 * scale, center.y + std::sin(angle) * 45 * scale,
             reference ? fg : muted, reference ? 2.f : 1.f);
    }
    ImGui::GetWindowDrawList()->AddCircleFilled(center, 31 * scale, panel, 48);
    ImGui::GetWindowDrawList()->AddCircle(center, 31 * scale, hover || active || focus ? tint : edge, 48);
    const float angle = startAngle + normalized * sweepAngle;
    line(center.x + std::cos(angle) * 21 * scale, center.y + std::sin(angle) * 21 * scale,
         center.x + std::cos(angle) * 29 * scale, center.y + std::sin(angle) * 29 * scale, tint, 2);
    char number[16];
    std::snprintf(number, sizeof(number), "%+d", static_cast<int>(std::round(value * 100)));
    text(center.x - GetFonts().fontSmall->CalcTextSizeA(GetFonts().fontSmall->FontSize, 100, 0, number).x / 2,
         center.y - GetFonts().fontSmall->FontSize / 2, number, fg, GetFonts().fontSmall);
    ImFont *labelFont = scale < .8f ? GetFonts().fontSmall : GetFonts().body;
    text(center.x - labelFont->CalcTextSizeA(labelFont->FontSize, 200, 0, label).x / 2, y + 102 * scale, label, fg,
         labelFont);
    ImGui::PopID();
}

void waveform(GUIState &s, float x, float y, float w, float h)
{
    static bool freeze = false;
    static std::array<float, 1024> samples{};
    static float gain = 1;
    const bool playing = s.playback.playing.load() || (s.running && s.runIsPreview && !s.stopRequested.load());
    if (!freeze)
    {
        auto &tap = *s.audioScope;
        const auto end = tap.cursor.load(std::memory_order_acquire);
        const auto &source = s.scopeWholeMix ? tap.mix : tap.part;
        uint64_t start = end > 2048 ? end - 2048 : 0;
        if (playing && end > 2048)
            for (uint64_t pos = start; pos + 1024 < end; ++pos)
                if (source[pos % AudioScope::capacity].load() <= 0 &&
                    source[(pos + 1) % AudioScope::capacity].load() > 0)
                {
                    start = pos;
                    break;
                }
        float peak = .04f;
        for (size_t i = 0; i < samples.size(); ++i)
        {
            samples[i] = playing && start + i < end
                             ? source[(start + i) % AudioScope::capacity].load(std::memory_order_relaxed)
                             : 0;
            peak = std::max(peak, std::abs(samples[i]));
        }
        gain += (std::min(10.f, .8f / peak) - gain) * .12f;
    }
    box(x, y, w, h, scope);
    ImGui::GetWindowDrawList()->AddRect({x, y}, {x + w, y + h}, edge);
    if (tab("選択ch", x + 18, y + 9, 96, 32, !s.scopeWholeMix))
        s.scopeWholeMix = false;
    if (tab("曲全体", x + 122, y + 9, 96, 32, s.scopeWholeMix))
        s.scopeWholeMix = true;
    at(x + w - 103, y + 14);
    ImGui::PushFont(GetFonts().fontSmall);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
    ImGui::Checkbox("固定", &freeze);
    ImGui::PopStyleVar();
    ImGui::PopFont();
    const float top = y + 56, bottom = y + h - 16, cy = (top + bottom) * .5f;
    line(x + 18, cy, x + w - 18, cy, edge);
    ImGui::GetWindowDrawList()->PushClipRect({x + 16, top}, {x + w - 16, bottom}, true);
    for (size_t i = 1; i < samples.size(); ++i)
        line(x + 18 + (w - 36) * (i - 1) / 1023, cy - samples[i - 1] * gain * (bottom - top) * .45f,
             x + 18 + (w - 36) * i / 1023, cy - samples[i] * gain * (bottom - top) * .45f, accent, 1.6f);
    ImGui::GetWindowDrawList()->PopClipRect();
}

void toneControls(GUIState &s, float x, float y, float w)
{
    dial(s, 0, "明るさ", x + 6, y);
    dial(s, 1, "ざらつき", x + 116, y);
    dial(s, 2, "余韻", x + 226, y);
    auto &part = s.tones[s.pianoRoll.displayChannel];
    if (disclosure("鳴り方・揺れ", x + 342, y + 28, 160, 36, s.toneExtraOpen))
        s.toneExtraOpen = !s.toneExtraOpen;
    ImGui::BeginDisabled(part.undo.empty());
    if (button("戻す", x + w - 193, y + 30, 85, 33))
        gui::UndoToneEdit(s);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(part.redo.empty());
    if (button("やり直す", x + w - 100, y + 30, 100, 33))
        gui::UndoToneEdit(s, true);
    ImGui::EndDisabled();
}

void extraControls(GUIState &s, float x, float y, float w)
{
    line(x, y, x + w, y);
    // 通常ノブと同じ中心位置（50 / 160 / 270）にそろえる。
    dial(s, 3, "立ち上がり", x + 19, y + 7, .62f);
    dial(s, 4, "減衰", x + 129, y + 7, .62f);
    dial(s, 5, "揺れ", x + 239, y + 7, .62f);
    const auto &sound = gui::AudibleInstrument(s, s.pianoRoll.displayChannel).sound;
    const float gx = x + 356, gy = y + 62, gw = std::max(65.f, w - 486);
    const double total = std::max(.2, sound.attackSec + sound.decaySec + sound.releaseSec + .3);
    const float a = static_cast<float>(sound.attackSec / total) * gw;
    const float d = static_cast<float>(sound.decaySec / total) * gw;
    line(gx, gy, gx + a, y + 16, accent);
    line(gx + a, y + 16, gx + a + d, gy - 40 * static_cast<float>(sound.sustainLevel), accent);
    line(gx + a + d, gy - 40 * static_cast<float>(sound.sustainLevel), gx + gw * .8f,
         gy - 40 * static_cast<float>(sound.sustainLevel), accent);
    line(gx + gw * .8f, gy - 40 * static_cast<float>(sound.sustainLevel), gx + gw, gy, accent);
    if (button("詳細", x + w - 104, y + 26, 104, 36))
    {
        s.tones[s.pianoRoll.displayChannel].compare = false;
        ImGui::OpenPopup("音色の詳細");
    }
}

} // namespace

float toneEditor(GUIState &s, float x, float y, float w, float controlsW, float availableH)
{
    // 開閉したフレームも同じ状態で高さを決め、下部操作まで一つのまとまりにする。
    const bool extraOpen = s.toneExtraOpen;
    constexpr float controlsH = 124, extraControlsH = 94;
    const float extraH = extraOpen ? extraControlsH : 0;
    const float waveH = std::min(240.f, availableH - controlsH - extraH);
    const float controlsBottom = y + waveH + controlsH;
    waveform(s, x, y, w, waveH);
    toneControls(s, x, y + waveH + 8, controlsW);
    if (extraOpen)
        extraControls(s, x, controlsBottom + 2, controlsW);
    return controlsBottom + extraH + 12;
}

void toneActions(GUIState &s, float x, float fy, float w)
{
    const int ch = s.pianoRoll.displayChannel;
    auto &part = s.tones[ch];
    const auto &audible = gui::AudibleInstrument(s, ch);
    const bool exporting = s.running && !s.runIsPreview;
    line(x, fy, x + w, fy);
    ImGui::BeginDisabled(exporting || s.toneAuditionActive || s.transportAction == gui::TransportAction::Audition);
    const bool drum = std::holds_alternative<DrumKitConfig>(audible.sound.source);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    if (button(s.toneAuditionActive ? "試聴中" : drum ? "ビートを試聴" : "一音鳴らす", x, fy + 12, 155, 40))
        gui::RequestToneAudition(s);
    if (button("", x + 155, fy + 12, 42, 40, ImGui::IsPopupOpen("preview_settings"), false, DownIcon))
        ImGui::OpenPopup("preview_settings");
    ImGui::PopStyleVar();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip(drum ? "試聴ビートの内容を表示" : "試聴する音の高さ・長さを設定");
    const ImU32 previewBorder = ImGui::GetColorU32(vec(edge));
    ImGui::GetWindowDrawList()->AddRect({x, fy + 12}, {x + 197, fy + 52}, previewBorder);
    line(x + 155.5f, fy + 12.5f, x + 155.5f, fy + 51.5f, previewBorder);
    ImGui::EndDisabled();
    if (ImGui::BeginPopup("preview_settings"))
    {
        if (drum)
            ImGui::TextUnformatted("キック・スネア・ハイハット\n2小節 / 120 BPM");
        else
        {
            bool automatic = part.auditionNote < 0;
            if (ImGui::Checkbox("曲に合う高さを自動選択", &automatic))
                part.auditionNote = automatic ? -1 : gui::ChooseAuditionNote(s, ch);
            int pitch = gui::ChooseAuditionNote(s, ch);
            ImGui::BeginDisabled(automatic);
            ImGui::SetNextItemWidth(260);
            if (ImGui::SliderInt("高さ", &pitch, 0, 127, pitchName(pitch).c_str()))
                part.auditionNote = pitch;
            ImGui::EndDisabled();
            ImGui::SetNextItemWidth(260);
            ImGui::SliderFloat("長さ", &s.auditionLengthSec, .2f, 3.f, "%.1f 秒");
        }
        ImGui::EndPopup();
    }
    ImGui::BeginDisabled(!gui::TonePending(s, ch) || exporting);
    if (button(part.compare ? "試聴中へ戻る" : "採用前と比較", x + 213, fy + 12, 160, 40, part.compare))
        part.compare = !part.compare;
    if (button("取り消す", x + w - 286, fy + 12, 112, 40))
        gui::CancelTone(s, ch);
    ImGui::BeginDisabled(part.compare);
    if (button("このchに採用", x + w - 162, fy + 12, 162, 40, false, true))
        gui::AdoptTone(s, ch);
    ImGui::EndDisabled();
    ImGui::EndDisabled();
}

void channelStrip(GUIState &s, float x, float w)
{
    at(x, 124);
    ImGui::BeginChild("channels", {w, 84}, false, ImGuiWindowFlags_HorizontalScrollbar);

    static int previousChannel = -1;
    const bool scrollToSelected = previousChannel != s.pianoRoll.displayChannel;
    std::vector<int> used;
    for (int ch = 0; ch < 16; ++ch)
        if (s.pianoRoll.noteCountByChannel[ch] || ch == s.pianoRoll.displayChannel)
            used.push_back(ch);
    for (size_t i = 0; i < used.size(); ++i)
    {
        const int ch = used[i];
        ImGui::PushID(ch);
        const auto p = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("channel", {174, 63}))
            gui::SelectToneChannel(s, ch);
        if (scrollToSelected && ch == s.pianoRoll.displayChannel)
            ImGui::SetScrollHereX(.5f);
        const bool selected = ch == s.pianoRoll.displayChannel;
        if (ImGui::IsItemHovered())
            box(p.x, p.y, 174, 63, panel);
        if (selected)
            box(p.x, p.y + 61, 174, 2, accent);
        const auto &part = s.tones[ch];
        const int cat = categoryIndex(part.category);
        icon(categoryGlyphs[cat], p.x + 12, p.y + 17, 25, selected ? accent : muted);
        const std::string label = "ch " + std::to_string(ch + 1) + " / " + categoryLabels[cat];
        text(p.x + 46, p.y + 7, label.c_str(), muted, GetFonts().fontSmall);
        clipped(p.x + 46, p.y + 31, 116, toneName(gui::AudibleInstrument(s, ch)).c_str(), fg, GetFonts().fontSmall);
        if (gui::TonePending(s, ch))
            ImGui::GetWindowDrawList()->AddCircleFilled({p.x + 162, p.y + 12}, 3, pendingColor);
        if (s.channelMixStates[ch].mute)
            line(p.x + 12, p.y + 46, p.x + 36, p.y + 18, muted, 2);
        ImGui::PopID();
        if (i + 1 < used.size())
            ImGui::SameLine(0, 8);
    }
    previousChannel = s.pianoRoll.displayChannel;
    ImGui::EndChild();
}

void mixControls(GUIState &s, float x, float y)
{
    auto &mix = s.channelMixStates[s.pianoRoll.displayChannel];
    text(x, y + (35 - GetFonts().fontSmall->FontSize) / 2, "音量", muted, GetFonts().fontSmall);
    at(x + 46, y);
    ImGui::SetNextItemWidth(160);
    float volume = static_cast<float>(mix.level * 100);
    if (ImGui::SliderFloat("##part_volume", &volume, 0, 200, "%.0f %%"))
    {
        mix.level = volume / 100;
        s.presetDirty = true;
    }
    if (button("消音", x + 220, y, 76, 35, mix.mute))
    {
        mix.mute = !mix.mute;
        s.presetDirty = true;
    }
    if (button("このパートだけ聴く", x + 304, y, 188, 35, mix.solo))
    {
        mix.solo = !mix.solo;
        s.presetDirty = true;
    }
    if (button("左右", x + 500, y, 76, 35))
        ImGui::OpenPopup("左右の位置");
    if (ImGui::BeginPopup("左右の位置"))
    {
        float pan = static_cast<float>(mix.pan);
        ImGui::SetNextItemWidth(240);
        if (ImGui::SliderFloat("左 / 右", &pan, -1, 1, "%.2f"))
        {
            mix.pan = pan;
            s.presetDirty = true;
        }
        if (ImGui::Button("中央へ"))
        {
            mix.pan = 0;
            s.presetDirty = true;
        }
        ImGui::EndPopup();
    }
}

void presetList(GUIState &s, float x, float y, float w, float h)
{
    static char query[128]{};
    static std::array<int, 16> filters = [] {
        std::array<int, 16> result;
        result.fill(-2);
        return result;
    }();
    const int ch = s.pianoRoll.displayChannel;
    auto &part = s.tones[ch];
    int &filter = filters[ch];
    text(x, y + 4, "プリセット", fg, GetFonts().heading);
    const int category = filter == -2 ? categoryIndex(part.category) : filter;
    if (category >= 0)
        icon(categoryGlyphs[category], x + w - 192, y + 4, 24, muted);
    at(x + w - 162, y - 2);
    ImGui::SetNextItemWidth(162);
    if (ImGui::BeginCombo("##filter", category < 0 ? "すべて" : categoryLabels[category]))
    {
        if (ImGui::Selectable("このパート", filter == -2))
            filter = -2;
        if (ImGui::Selectable("すべて", filter == -1))
            filter = -1;
        for (int i = 0; i < 8; ++i)
            if (ImGui::Selectable(categoryLabels[i], filter == i))
                filter = i;
        ImGui::EndCombo();
    }
    at(x, y + 48);
    ImGui::SetNextItemWidth(w);
    ImGui::InputTextWithHint("##preset_search", "音色を検索", query, sizeof(query));
    at(x, y + 94);
    ImGui::BeginChild("presets", {w, h - 94}, false);
    const bool showRowCategory = category < 0;
    const float textInset = showRowCategory ? 60.f : 16.f;
    int count = 0;
    for (int i = 0; i < static_cast<int>(s.presetItems.size()); ++i)
    {
        const auto &preset = s.presetItems[i];
        if (preset.internalOnly || preset.name.starts_with("user/"))
            continue;
        if (category >= 0 && categoryIndex(preset.category) != category)
            continue;
        if (!matches(preset.displayName + " " + preset.description, query))
            continue;
        ++count;
        const auto p = ImGui::GetCursorScreenPos();
        const float rw = ImGui::GetContentRegionAvail().x;
        const bool selected = preset.name == part.draft.key && preset.revision == part.draft.presetRevision;
        const bool adopted = preset.name == part.adopted.key && preset.revision == part.adopted.presetRevision;
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("preset", {rw, 78}))
        {
            std::string error;
            if (!gui::SelectTonePreset(s, i, error))
                gui::RaiseGUIError(s, error, 0, true);
        }
        const bool hovered = ImGui::IsItemHovered();
        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s / %s\n%s\nクリックでこのパートの音色を切り替え", preset.displayName.c_str(),
                              categoryLabels[categoryIndex(preset.category)], preset.description.c_str());
        if (selected || hovered)
            box(p.x, p.y, rw, 78, panel);
        const ImU32 selectedColor = part.compare ? muted : accent;
        if (selected)
            box(p.x, p.y, 3, 78, selectedColor);
        if (showRowCategory)
            icon(categoryGlyphs[categoryIndex(preset.category)], p.x + 16, p.y + 21, 30, selected ? selectedColor : muted);
        const bool auditioning = selected && !part.compare && gui::TonePending(s, ch);
        const char *status = selected  ? (part.compare ? "保持中" : auditioning ? "試聴中" : "採用済み")
                             : adopted ? "採用済み"
                                       : "";
        if (!selected && !adopted)
            if (auto it = part.cache.find(gui::ToneCacheKey(preset.name, preset.revision)); it != part.cache.end())
                if (it->second.customizedBase || it->second.adjusted)
                    status = "調整済み";
        clipped(p.x + textInset, p.y + 9, rw - textInset - 92, preset.displayName.c_str(), fg, GetFonts().body);
        text(p.x + rw - 83, p.y + 11, status, auditioning ? pendingColor : muted, GetFonts().fontSmall);
        clipped(p.x + textInset, p.y + 43, rw - textInset - 12, preset.description.c_str(), muted, GetFonts().fontSmall);
        ImGui::PopID();
    }
    if (!count)
        ImGui::TextWrapped("該当する音色がありません。分類や検索語を変えてください。");
    ImGui::EndChild();
}
} // namespace studio
