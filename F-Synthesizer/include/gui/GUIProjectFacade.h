#pragma once
#include "gui/GUIState.h"
#include "project/ProjectModel.h"

namespace gui
{
// Publish an immutable snapshot of the sounds currently being heard.
void PublishLiveRenderSettings(GUIState& state);
// Resolve persisted instrument IDs here; editing uses independent channel drafts.
ProjectModel BuildProjectModelFromGUI(const GUIState& state);
void ApplyProjectModelToGUI(GUIState& state, const ProjectModel& model);
}
