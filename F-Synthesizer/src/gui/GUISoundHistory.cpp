#include "gui/GUISoundHistory.h"

#include <algorithm>

#include "gui/GUIState.h"
#include "gui/GUIProjectFacade.h"

void PushSoundHistoryEntry(
    GUIState& state,
    int slot,
    const InstrumentSoundConfig& config,
    const MacroSliderState& sliders)
{
    // Redo スタックをクリア（新しい操作はやり直し履歴を破棄する）
    state.soundRedoStack.clear();

    SoundUndoEntry entry;
    entry.slot = slot;
    entry.instrument = state.instruments[std::clamp(slot, 0, 15)];
    entry.instrument.sound = config;
    entry.macroSliders = sliders;
    state.soundUndoStack.push_back(std::move(entry));

    // 上限を超えた場合は古いエントリを削除
    while (static_cast<int>(state.soundUndoStack.size()) > kSoundUndoMaxDepth)
    {
        state.soundUndoStack.pop_front();
    }
}

void ApplySoundHistoryEntry(GUIState& state, const SoundUndoEntry& entry)
{
    const int slot = std::clamp(entry.slot, 0, 15);
    state.instruments[slot] = entry.instrument;
    state.selectedSoundSlot = slot;
    if (state.playEditingChannel >= 0 && gui::AssignedSoundSlot(state, state.playEditingChannel) != slot)
        state.playEditingChannel = -1;
    gui::MutableMacroSliders(state, slot) = entry.macroSliders;
    state.presetDirty = true;
}

bool UndoSound(GUIState& state)
{
    if (state.soundUndoStack.empty())
    {
        return false;
    }

    // 現在の状態を Redo スタックに退避
    const int slot = std::clamp(state.soundUndoStack.back().slot, 0, 15);
    SoundUndoEntry current;
    current.slot = slot;
    current.instrument = state.instruments[slot];
    current.macroSliders = gui::ReadMacroSliders(state, slot);
    state.soundRedoStack.push_back(std::move(current));

    // Undo スタックから取り出して適用
    ApplySoundHistoryEntry(state, state.soundUndoStack.back());
    state.soundUndoStack.pop_back();
    return true;
}

bool RedoSound(GUIState& state)
{
    if (state.soundRedoStack.empty())
    {
        return false;
    }

    // 現在の状態を Undo スタックに退避
    const int slot = std::clamp(state.soundRedoStack.back().slot, 0, 15);
    SoundUndoEntry current;
    current.slot = slot;
    current.instrument = state.instruments[slot];
    current.macroSliders = gui::ReadMacroSliders(state, slot);
    state.soundUndoStack.push_back(std::move(current));

    // Redo スタックから取り出して適用
    ApplySoundHistoryEntry(state, state.soundRedoStack.back());
    state.soundRedoStack.pop_back();
    return true;
}
