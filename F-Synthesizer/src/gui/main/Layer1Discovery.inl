#include <cctype>

#include "config/SourceRegistry.h"
#include "gui/GUIMacroSliders.h"

namespace
{
static constexpr const char* kMacroTags[] = {
    "ゲーム的", "暗い", "明るい", "金属的", "柔らかい",
    "ノイジー", "シンプル", "複雑", "アタック", "パッド",
    "FM", "アナログ", "ドラム", "ノイズ"
};

bool PresetMatchesTag(const std::string& name, int tagIdx)
{
    static constexpr const char* keywords[kMacroTagCount] = {
        "game", "dark", "bright", "metal", "soft",
        "noise", "simple", "complex", "attack", "pad",
        "fm", "analog", "drum", "noise"
    };

    if (tagIdx < 0 || tagIdx >= kMacroTagCount)
    {
        return false;
    }

    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower.find(keywords[tagIdx]) != std::string::npos;
}

bool PresetMatchesTagWithMetadata(
    const std::string& name,
    const std::vector<std::string>& tags,
    int tagIdx)
{
    static constexpr const char* keywords[kMacroTagCount] = {
        "game", "dark", "bright", "metal", "soft",
        "noise", "simple", "complex", "attack", "pad",
        "fm", "analog", "drum", "noise"
    };

    if (tagIdx < 0 || tagIdx >= kMacroTagCount)
    {
        return false;
    }

    const std::string key = keywords[tagIdx];
    for (const auto& tag : tags)
    {
        std::string lowerTag = tag;
        std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lowerTag == key || lowerTag.find(key) != std::string::npos)
        {
            return true;
        }
    }
    return PresetMatchesTag(name, tagIdx);
}

template <typename ApplyPresetFn>
void DrawLayer1Discovery(
    GUIState& state,
    int& pendingPresetIndex,
    int& pendingPresetOriginalIndex,
    bool& openUnsavedPopupNextFrame,
    ApplyPresetFn&& applyPresetByIndex)
{
    ImGui::SetNextItemOpen(state.layer1Expanded, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("発見  ( プリセット + タグ )"))
    {
        state.layer1Expanded = false;
        return;
    }
    state.layer1Expanded = true;

    if (state.channelConfigs)
    {
        const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
        const config::SourceKind kind = config::SourceConfigKind((*state.channelConfigs)[slot].source);
        ImGui::TextDisabled("Source: %s  (%d presets)",
            config::SourceKindToDisplayName(kind),
            static_cast<int>(state.presetItems.size()));
    }

    ImGui::TextDisabled("タグ:");
    const ImGuiStyle& style = ImGui::GetStyle();
    const float contentWidth = ImGui::GetContentRegionAvail().x;
    float cursorX = 0.0f;
    for (int i = 0; i < kMacroTagCount; ++i)
    {
        const float btnWidth = ImGui::CalcTextSize(kMacroTags[i]).x
            + style.FramePadding.x * 2.0f
            + style.ItemSpacing.x;
        cursorX += btnWidth;
        if (i > 0 && cursorX < contentWidth)
        {
            ImGui::SameLine();
        }
        else
        {
            cursorX = btnWidth;
        }
        const bool active = state.macroTagFilters[i];
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::SmallButton(kMacroTags[i]))
        {
            state.macroTagFilters[i] = !state.macroTagFilters[i];
        }
        if (active)
        {
            ImGui::PopStyleColor();
        }
    }

    bool anyTag = false;
    for (int i = 0; i < kMacroTagCount; ++i)
    {
        if (state.macroTagFilters[i])
        {
            anyTag = true;
            break;
        }
    }

    ImGui::Spacing();
    ImGui::BeginChild("##layer1_presets", ImVec2(0.0f, 112.0f), true);
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); ++i)
    {
        const std::string& name = state.presetItems[i];
        static const std::vector<std::string> kEmptyTags{};
        const std::vector<std::string>& tags =
            (i < static_cast<int>(state.presetItemTags.size())) ? state.presetItemTags[i] : kEmptyTags;
        if (anyTag)
        {
            bool match = false;
            for (int t = 0; t < kMacroTagCount; ++t)
            {
                if (state.macroTagFilters[t] && PresetMatchesTagWithMetadata(name, tags, t))
                {
                    match = true;
                    break;
                }
            }
            if (!match)
            {
                continue;
            }
        }

        const bool selected = (state.presetIndex == i);
        if (ImGui::Selectable(name.c_str(), selected))
        {
            if (state.presetDirty)
            {
                pendingPresetOriginalIndex = state.presetIndex;
                pendingPresetIndex = i;
                openUnsavedPopupNextFrame = true;
                state.presetIndex = i;
            }
            else
            {
                applyPresetByIndex(i);
            }
        }
    }
    ImGui::EndChild();

    if (state.presetIndex >= 0
        && state.presetIndex < static_cast<int>(state.presetItemDescriptions.size()))
    {
        const std::string& desc = state.presetItemDescriptions[state.presetIndex];
        if (!desc.empty())
        {
            ImGui::TextWrapped("%s", desc.c_str());
        }
    }
}
} // namespace
