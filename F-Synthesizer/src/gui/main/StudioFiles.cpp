#include "StudioFiles.h"
#include "StudioPanels.h"
#include "gui/GUIActions.h"
#include "gui/GUIChannelEditor.h"
#include "gui/GUIPlatform.h"
#include "gui/GUIPresetIO.h"
#include "gui/GUIStatePersistence.h"
#include "io/PlatformPaths.h"
#include <algorithm>
#include <cstring>
namespace studio
{
bool FileActions::saveSong(GUIState &s, bool saveAs)
{
    std::string path = s.activeProjectPath;
    if (saveAs || path.empty())
    {
        if (!BrowseSavePath(path.c_str(), L"F-Synthesizer Song (*.fsynth)\0*.fsynth\0", L"fsynth", path))
            return false;
        if (Utf8ToPath(path).extension() != ".fsynth")
            path += ".fsynth";
    }
    std::string error;
    if (!gui::SaveSongProjectFile(s, Utf8ToPath(path), error))
    {
        gui::RaiseGUIError(s, error, 0, true);
        return false;
    }
    s.observedNotesVersion = s.pianoRoll.notesVersion;
    return true;
}
void FileActions::openFile(GUIState &s)
{
    std::string error;
    if (windowState.openSong)
    {
        if (!gui::LoadSongProjectFile(s, Utf8ToPath(windowState.openPath), error))
        {
            gui::RaiseGUIError(s, error, 0, true);
            return;
        }
    }
    else
    {
        gui::PianoRollState piano;
        if (!gui::LoadPianoRollMIDI(piano, Utf8ToPath(windowState.openPath)))
        {
            gui::RaiseGUIError(s, piano.lastError, 0, true);
            return;
        }
        s.pianoRoll = std::move(piano);
        strncpy_s(s.midiPath, sizeof(s.midiPath), windowState.openPath.c_str(), _TRUNCATE);
        s.songMidiName = PathToUtf8(Utf8ToPath(windowState.openPath).filename());
        s.activeProjectPath.clear();
        s.stepSeq = {};
        s.presetDirty = true;
        gui::InitializeToneWorkspace(s, true);
    }
    gui::InitializeToneWorkspace(s);
    s.toneEditBefore.reset();
    s.songCursorTick = 0;
    s.toneNotesOpen = false;
    int selected = s.pianoRoll.displayChannel;
    if (!s.pianoRoll.noteCountByChannel[selected])
        for (int ch = 0; ch < 16; ++ch)
            if (s.pianoRoll.noteCountByChannel[ch])
            {
                selected = ch;
                break;
            }
    gui::SelectToneChannel(s, selected);
    s.observedNotesVersion = s.pianoRoll.notesVersion;
    windowState.openPath.clear();
    windowState.openAfterSave = false;
}
void FileActions::chooseFile(GUIState &s, bool song)
{
    std::string path;
    if (!BrowseOpenPath(
            "", song ? L"F-Synthesizer Song (*.fsynth)\0*.fsynth\0" : L"MIDI (*.mid;*.midi)\0*.mid;*.midi\0", path))
        return;
    windowState.openPath = path;
    windowState.openSong = song;
    if (!(s.presetDirty || gui::PendingToneCount(s)))
        openFile(s);
}
void FileActions::requestOperation(GUIState &s, int operation)
{
    windowState.operation = operation;
    windowState.confirmOperation = gui::PendingToneCount(s) > 0;
}
void FileActions::finishOperation(GUIState &s)
{
    if (!windowState.operation || windowState.confirmOperation)
        return;
    const int operation = windowState.operation;
    if (operation == 3 && (s.running || s.playback.playing.load()))
    {
        gui::PauseSongPlayback(s);
        return;
    }
    windowState.operation = 0;
    if (operation == 1 || operation == 2)
    {
        if (saveSong(s, operation == 2) && windowState.openAfterSave)
            openFile(s);
        else
            windowState.openAfterSave = false;
    }
    if (operation == 3)
    {
        std::string path;
        if (BrowseSavePath(s.wavPath, L"WAV (*.wav)\0*.wav\0", L"wav", path))
        {
            if (Utf8ToPath(path).extension() != ".wav")
                path += ".wav";
            strncpy_s(s.wavPath, sizeof(s.wavPath), path.c_str(), _TRUNCATE);
            s.targetChannel = -1;
            s.serialSave = false;
            gui::StartGUIRun(s, false);
        }
    }
}

void FileActions::favorites(GUIState &s, float x, float y, float w)
{
    static char search[128]{};
    static int category = -1;
    icon(StarIcon, x, y + 3, 22);
    text(x + 33, y, "お気に入り", fg, GetFonts().heading);
    at(x + 230, y - 2);
    ImGui::SetNextItemWidth(230);
    ImGui::InputTextWithHint("##favorite_search", "名前で検索", search, sizeof(search));
    at(x + 476, y - 2);
    ImGui::SetNextItemWidth(176);
    if (ImGui::BeginCombo("##favorite_category", category < 0 ? "すべての分類" : categoryLabels[category]))
    {
        if (ImGui::Selectable("すべての分類", category < 0))
            category = -1;
        for (int i = 0; i < 8; ++i)
            if (ImGui::Selectable(categoryLabels[i], category == i))
                category = i;
        ImGui::EndCombo();
    }
    if (button("いまの音を追加", x + w - 174, y - 2, 174, 36, false, false, StarIcon))
    {
        const auto &instrument = gui::AudibleInstrument(s, s.pianoRoll.displayChannel);
        std::filesystem::path saved;
        std::string error;
        if (gui::SaveUserPresetFile(FindProjectRootPath(), instrument, toneName(instrument), saved, error))
            gui::RefreshPresetItems(s, "user/" + PathToUtf8(saved.stem()));
        else
            gui::RaiseGUIError(s, error, 0, true);
    }
    at(x, y + 42);
    ImGui::BeginChild("favorites", {w, 86}, false, ImGuiWindowFlags_HorizontalScrollbar);
    bool first = true;
    for (int i = 0; i < static_cast<int>(s.presetItems.size()); ++i)
    {
        const auto &item = s.presetItems[i];
        if (!item.name.starts_with("user/") || !matches(item.displayName, search) ||
            (category >= 0 && categoryIndex(item.category) != category))
            continue;
        if (!first)
            ImGui::SameLine(0, 16);
        first = false;
        const auto p = ImGui::GetCursorScreenPos();
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("favorite", {265, 62}))
        {
            std::string error;
            if (!gui::SelectTonePreset(s, i, error))
                gui::RaiseGUIError(s, error, 0, true);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s\n右クリックで名前を変更", item.displayName.c_str());
        box(p.x, p.y, 265, 62, ImGui::IsItemHovered() ? raised : panel);
        icon(categoryGlyphs[categoryIndex(item.category)], p.x + 14, p.y + 18, 25);
        clipped(p.x + 52, p.y + 7, 202, item.displayName.c_str(), fg, GetFonts().body);
        text(p.x + 52, p.y + 35, categoryLabels[categoryIndex(item.category)], muted, GetFonts().fontSmall);
        if (ImGui::BeginPopupContextItem("favorite_menu"))
        {
            if (ImGui::MenuItem("名前を変更"))
            {
                windowState.renameKey = item.name;
                strncpy_s(windowState.renameText, item.displayName.c_str(), _TRUNCATE);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    if (first)
        ImGui::TextDisabled("気に入った音は「いまの音を追加」で保存できます。");
    ImGui::EndChild();
}

void FileActions::dialogs(GUIState &s)
{
    if (windowState.confirmOperation && !ImGui::IsPopupOpen("未採用の音色"))
        ImGui::OpenPopup("未採用の音色");
    if (ImGui::BeginPopupModal("未採用の音色", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%d チャンネルに、まだ採用していない音色があります。", gui::PendingToneCount(s));
        if (ImGui::Button("まとめて採用して続行"))
        {
            gui::AdoptAllTones(s);
            windowState.confirmOperation = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("戻って調整"))
        {
            windowState.operation = 0;
            windowState.confirmOperation = false;
            windowState.openAfterSave = false;
            windowState.openPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (!windowState.openPath.empty() && !windowState.openAfterSave && !ImGui::IsPopupOpen("曲を切り替える"))
        ImGui::OpenPopup("曲を切り替える");
    if (ImGui::BeginPopupModal("曲を切り替える", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("今の曲に変更があります。保存しますか？");
        if (ImGui::Button("保存して開く"))
        {
            windowState.openAfterSave = true;
            requestOperation(s, 1);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("破棄して開く"))
        {
            openFile(s);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("戻る"))
        {
            windowState.openPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (!windowState.renameKey.empty() && !ImGui::IsPopupOpen("お気に入りの名前"))
        ImGui::OpenPopup("お気に入りの名前");
    if (ImGui::BeginPopupModal("お気に入りの名前", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("名前", windowState.renameText, sizeof(windowState.renameText));
        if (ImGui::Button("変更") && windowState.renameText[0])
        {
            std::string error;
            if (gui::RenameUserPreset(FindProjectRootPath(), windowState.renameKey, windowState.renameText, error))
            {
                gui::RefreshPresetItems(s, windowState.renameKey);
                windowState.renameKey.clear();
                ImGui::CloseCurrentPopup();
            }
            else
                gui::RaiseGUIError(s, error, 0, true);
        }
        ImGui::SameLine();
        if (ImGui::Button("戻る"))
        {
            windowState.renameKey.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::SetNextWindowSize({850, 680}, ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("音色の詳細", nullptr, ImGuiWindowFlags_None))
    {
        ImGui::TextUnformatted("音源・エンベロープ・エフェクト");
        ImGui::SameLine();
        auto &history = s.tones[s.pianoRoll.displayChannel];
        ImGui::BeginDisabled(history.undo.empty());
        if (ImGui::Button("戻す"))
            gui::UndoToneEdit(s);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(history.redo.empty());
        if (ImGui::Button("やり直す"))
            gui::UndoToneEdit(s, true);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("閉じる"))
        {
            gui::FinishToneEdit(s);
            ImGui::CloseCurrentPopup();
        }
        ImGui::Separator();
        ImGui::BeginChild("detail_content", {0, 0});
        auto editedSound = history.draft.instrument.sound;
        if (gui::DrawChannelEditor(editedSound, s.pianoRoll.displayChannel, s.selectedDrumNote))
            gui::ApplyDetailedToneEdit(s, editedSound);
        if (!ImGui::IsAnyItemActive())
            gui::FinishToneEdit(s);
        if (!ImGui::GetIO().WantTextInput && !ImGui::IsAnyItemActive() && ImGui::GetIO().KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
                gui::UndoToneEdit(s);
            if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
                gui::UndoToneEdit(s, true);
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("設定", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("一音試聴");
        ImGui::SliderFloat("長さ", &s.auditionLengthSec, .2f, 3.f, "%.1f 秒");
        ImGui::Separator();
        ImGui::BeginDisabled(s.running || s.playback.playing.load());
        const char *rates[] = {"44100 Hz", "48000 Hz"};
        int rate = s.sampleRate == 48000 ? 1 : 0;
        if (ImGui::Combo("サンプルレート", &rate, rates, 2))
            s.sampleRate = rate ? 48000 : 44100;
        ImGui::EndDisabled();
        if (ImGui::CollapsingHeader("曲全体の効果"))
        {
            auto &fx = s.masterEffects;
            const auto before = fx;
            auto slider = [](const char *label, double &value, float low, float high) {
                float edit = static_cast<float>(value);
                ImGui::SetNextItemWidth(240);
                if (ImGui::SliderFloat(label, &edit, low, high, "%.2f"))
                    value = edit;
            };
            if (ImGui::TreeNode("リバーブ"))
            {
                ImGui::Checkbox("有効", &fx.reverb.enabled);
                slider("深さ", fx.reverb.mix, 0, 1);
                slider("広がり", fx.reverb.roomSize, 0, 1);
                slider("減衰", fx.reverb.damping, 0, 1);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("ディレイ"))
            {
                ImGui::Checkbox("有効", &fx.delay.enabled);
                slider("深さ", fx.delay.mix, 0, 1);
                slider("繰り返し", fx.delay.feedback, 0, .95f);
                ImGui::Checkbox("テンポに合わせる", &fx.delay.tempoSync);
                if (fx.delay.tempoSync)
                    slider("拍数", fx.delay.syncBeats, .125f, 4);
                else
                    slider("間隔（秒）", fx.delay.timeSec, .01f, 2);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("コーラス"))
            {
                ImGui::Checkbox("有効", &fx.chorus.enabled);
                slider("深さ", fx.chorus.mix, 0, 1);
                slider("速さ", fx.chorus.rateHz, .05f, 5);
                slider("幅", fx.chorus.depthMs, 0, 12);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("フランジャー"))
            {
                ImGui::Checkbox("有効", &fx.flanger.enabled);
                slider("深さ", fx.flanger.mix, 0, 1);
                slider("速さ", fx.flanger.rateHz, .05f, 5);
                slider("幅", fx.flanger.depthMs, 0, 5);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("音の粗さ"))
            {
                ImGui::SliderInt("ビット数", &fx.bitCrusher.bits, 1, 16);
                slider("サンプル間隔", fx.sampleRateReducer.ratio, .05f, 1);
                ImGui::TreePop();
            }
            if (before != fx)
                s.presetDirty = true;
        }
        if (ImGui::Button("閉じる"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("操作方法", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(
            "1. 上部のメニューから MIDI を開く\n2. チャンネルを選び、右のプリセットをクリック\n3. "
            "ノブで調整して「このchに採用」\n\nSpace: 再生・一時停止 / Ctrl+S: 曲を保存\nCtrl+Z / Ctrl+Y: "
            "元に戻す・やり直す\nノブ: 上下ドラッグ / Shift: 微調整 / 右クリック: 数値入力\n進行バー: クリックで移動 / "
            "ドラッグで区間リピート\nお気に入り: 右クリックで名前を変更");
        if (ImGui::Button("閉じる"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (s.showErrorDialog)
    {
        ImGui::OpenPopup("お知らせ");
        s.showErrorDialog = false;
    }
    if (ImGui::BeginPopupModal("お知らせ", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 560);
        ImGui::TextUnformatted(s.UIErrorMessage.c_str());
        ImGui::PopTextWrapPos();
        if (ImGui::Button("閉じる"))
        {
            gui::ClearGUIError(s);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
} // namespace studio
