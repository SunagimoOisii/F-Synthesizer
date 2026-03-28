#pragma once

#include <deque>

#include "AppCore.h"
#include "gui/GUIMacroSliders.h"

struct GUIState; // forward declaration (GUIState includes this header)

struct SoundUndoEntry
{
    int slot = 0;
    ChannelConfig channelConfig;
    MacroSliderState macroSliders;
};

constexpr int kSoundUndoMaxDepth = 50;

// 現在の slot 状態をアンドゥスタックに積む。Redo スタックはクリアされる。
void PushSoundHistoryEntry(
    GUIState& state,
    int slot,
    const ChannelConfig& config,
    const MacroSliderState& sliders);

// エントリを状態へ適用する（Undo/Redo 共通）。presetDirty = true をセット。
void ApplySoundHistoryEntry(GUIState& state, const SoundUndoEntry& entry);

// Ctrl+Z: Undo スタックからポップして適用。成功時 true を返す。
bool UndoSound(GUIState& state);

// Ctrl+Y: Redo スタックからポップして適用。成功時 true を返す。
bool RedoSound(GUIState& state);
