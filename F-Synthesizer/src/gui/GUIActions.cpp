#include "gui/GUIActions.h"

#include <algorithm>
#include <cctype>
#include <mutex>

#include "gui/GUIActionsInternal.h"

namespace gui::detail
{
std::vector<std::string>& LogsByTab(GUIState& state, int tab)
{
    if (tab == 1) { return state.musicLogs; }
    if (tab == 2) { return state.exportLogs; }
    return state.soundLogs;
}

const std::vector<std::string>& LogsByTab(const GUIState& state, int tab)
{
    if (tab == 1) { return state.musicLogs; }
    if (tab == 2) { return state.exportLogs; }
    return state.soundLogs;
}

void AppendGUILogToTab(GUIState& state, int tab, const std::string& line)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    LogsByTab(state, tab).push_back(line);
}
} // namespace gui::detail

namespace gui
{
void AppendGUILog(GUIState& state, const std::string& line)
{
    detail::AppendGUILogToTab(state, state.runLogTab, line);
}

void RaiseGUIError(GUIState& state, const std::string& message, int actionHint, bool showDialog)
{
    state.hasUIError = true;
    state.UIErrorMessage = message;
    state.UIErrorAction = std::clamp(actionHint, 0, 4);
    state.showErrorDialog = showDialog;
}

void ClearGUIError(GUIState& state)
{
    state.hasUIError = false;
    state.showErrorDialog = false;
    state.UIErrorAction = 0;
    state.UIErrorMessage.clear();
}
} // namespace gui
