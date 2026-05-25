#pragma once

#include <array>
#include <memory>
#include <vector>

#include "AppCore.h"
#include "core/RenderConfig.h"
#include "midi/MIDIPipeline.h"
#include "project/ProjectModel.h"

struct ResolvedRenderConfigInputs
{
    std::shared_ptr<const std::array<InstrumentSoundConfig, 16>> soundSlots;
    std::shared_ptr<const std::array<ChannelMixState, 16>> channelMixStates;
};

ResolvedRenderConfigInputs ResolveRenderConfigInputs(const ProjectModel& project);

RenderConfig BuildRenderConfig(
    const ProjectModel& project,
    const RenderOptions& options,
    const std::vector<MIDIEvent>& events,
    const MIDIBuildOutput& midiOut,
    const ResolvedRenderConfigInputs& inputs);
