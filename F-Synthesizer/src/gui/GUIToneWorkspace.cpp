#include "gui/GUIToneWorkspace.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include "config/ProjectJSON.h"
#include "gui/GUIActions.h"
#include "gui/GUIProjectFacade.h"
#include "gui/GUIState.h"
#include "io/PlatformPaths.h"

namespace gui
{
namespace
{
bool SameTone(const ToneVersion& a, const ToneVersion& b)
{
    return a.key == b.key && a.instrument.sound == b.instrument.sound && a.instrument.comparisonGain == b.instrument.comparisonGain && a.values == b.values;
}
void SyncDraft(GUIState& state, int ch)
{
    auto& part = state.tones[ch];
    state.instruments[ch] = part.draft.instrument;
    part.cache[ToneCacheKey(part.draft)] = part.draft;
    state.presetDirty = true;
}
void PushUndo(ChannelToneWorkspace& part, const ToneVersion& tone)
{
    part.undo.push_back(tone);
    if (part.undo.size() > 32) part.undo.pop_front();
    part.redo.clear();
}
void AdjustDrum(DrumConfig& drum, const std::array<float, 6>& v)
{
    if (drum.type == DrumType::None) return;
    if (drum.lpCut > 0) drum.lpCut = std::clamp(drum.lpCut * std::exp2(v[0] * .65), 20.0, 18000.0);
    if (drum.hpCut > 0) drum.hpCut = std::clamp(drum.hpCut * std::exp2(v[0] * .35), 20.0, 16000.0);
    drum.drive = std::clamp(drum.drive + v[1] * .1, 0.0, 1.0);
    const double length = std::exp2(v[2] * .8);
    drum.decaySec *= length; drum.bodyDecaySec *= length; drum.snapDecaySec *= length;
    drum.humanizePitchCents = std::clamp(drum.humanizePitchCents + v[5] * 3, 0.0, 30.0);
}
}

std::string InferPartCategory(const GUIState& state, int channel)
{
    if (channel == 9) return "Drums";
    if (!state.pianoRoll.hasProgramByChannel[channel])
        return state.instruments[channel].category.empty() ? "Keys" : state.instruments[channel].category;
    const int program = state.pianoRoll.programByChannel[channel];
    if (program < 24) return "Keys";
    if (program < 32) return "Guitar";
    if (program < 40) return "Bass";
    if (program < 56) return "Pad";
    if (program < 88) return "Lead";
    if (program < 96) return "Pad";
    if (program < 104 || program >= 120) return "SFX";
    return "Support";
}

void InitializeToneWorkspace(GUIState& state, bool reset)
{
    if (state.toneWorkspaceReady && !reset) return;
    const auto instruments = std::make_unique<std::array<InstrumentConfig, 16>>(state.instruments);
    for (int ch = 0; ch < 16; ++ch)
    {
        state.instruments[ch] = (*instruments)[AssignedSoundSlot(state, ch)];
        state.channelAssignments[ch] = ch;
        auto& part = state.tones[ch];
        part = {};
        part.category = InferPartCategory(state, ch);
        part.draft.key = "song/" + std::to_string(ch);
        for (const auto& preset : state.presetItems)
            if (!preset.name.starts_with("user/") && preset.displayName == state.instruments[ch].displayName)
            {
                part.draft.key = preset.name;
                if (state.instruments[ch].comparisonGain == 1) state.instruments[ch].comparisonGain = preset.comparisonGain;
                break;
            }
        part.draft.base = part.draft.instrument = state.instruments[ch];
        part.adopted = part.draft;
        part.cache[ToneCacheKey(part.draft)] = part.draft;
    }
    state.toneWorkspaceReady = true;
    SelectToneChannel(state, std::clamp(state.pianoRoll.displayChannel, 0, 15));
}

bool TonePending(const GUIState& state, int channel)
{
    return state.toneWorkspaceReady && !SameTone(state.tones[channel].adopted, state.tones[channel].draft);
}
int PendingToneCount(const GUIState& state)
{
    int count = 0;
    for (int ch = 0; ch < 16; ++ch) count += TonePending(state, ch);
    return count;
}
const InstrumentConfig& AudibleInstrument(const GUIState& state, int channel)
{
    if (state.toneWorkspaceReady && state.tones[channel].compare) return state.tones[channel].adopted.instrument;
    return state.instruments[AssignedSoundSlot(state, channel)];
}
void SelectToneChannel(GUIState& state, int channel)
{
    FinishToneEdit(state);
    state.playEditingChannel = state.pianoRoll.displayChannel = std::clamp(channel, 0, 15);
    state.selectedSoundSlot = AssignedSoundSlot(state, state.playEditingChannel);
    state.pianoRoll.drumNameMode = channel == 9;
    state.audioScope->channel.store(state.playEditingChannel, std::memory_order_relaxed);
    state.tonePreviewNoteNumber = ChooseAuditionNote(state, channel);
}

bool SelectTonePreset(GUIState& state, int presetIndex, std::string& error)
{
    InitializeToneWorkspace(state);
    if (presetIndex < 0 || presetIndex >= static_cast<int>(state.presetItems.size())) return false;
    FinishToneEdit(state);
    const int ch = state.pianoRoll.displayChannel;
    auto& part = state.tones[ch];
    const auto& item = state.presetItems[presetIndex];
    try
    {
        ToneVersion next;
        if (const auto cached = part.cache.find(ToneCacheKey(item.name, item.revision)); cached != part.cache.end()) next = cached->second;
        else
        {
            const bool user = item.name.starts_with("user/");
            const auto path = FindProjectRootPath() / "config" / (user ? "user_presets" : "presets") /
                Utf8ToPath((user ? item.name.substr(5) : item.name) + ".json");
            ProjectModel model = DefaultProjectModel();
            model.instruments.reset(); model.projectChannels.reset();
            std::ifstream input(path, std::ios::binary);
            if (!config::ProjectFromJSON(nlohmann::json::parse(input), path.parent_path(), model, error)) return false;
            if (!model.instruments || model.instruments->empty()) { error = "音色が含まれていません。"; return false; }
            next.key = item.name;
            next.base = next.instrument = model.instruments->begin()->second;
            next.base.comparisonGain = next.instrument.comparisonGain = item.comparisonGain;
            // Recover pre-revision macro edits only when their original sound
            // still matches. Keep other old trials in their own cache entry.
            if (const auto old = part.cache.find(item.name); old != part.cache.end()
                && !old->second.customizedBase && old->second.base.sound == next.base.sound)
                next = old->second;
            next.presetRevision = item.revision;
        }
        if (!SameTone(next, part.draft)) PushUndo(part, part.draft);
        part.cache[ToneCacheKey(part.draft)] = part.draft;
        part.draft = std::move(next);
        part.compare = false;
        SyncDraft(state, ch);
        state.tonePreviewNoteNumber = ChooseAuditionNote(state, ch);
        return true;
    }
    catch (const std::exception& ex) { error = ex.what(); return false; }
}

bool ToneControlSupported(const InstrumentSoundConfig& sound, int control)
{
    // A waveform held at the attack level has no audible decay stage.
    if (control == 4 && sound.sustainLevel == 1.0 &&
        (std::holds_alternative<WaveformConfig>(sound.source) || std::holds_alternative<AnalogConfig>(sound.source))) return false;
    return std::visit([control](const auto& source) {
        using T = std::decay_t<decltype(source)>;
        if constexpr (std::is_same_v<T, FmConfig>) if (control == 0 && source.algorithm == 7) return false;
        if constexpr (!std::is_same_v<T, FmConfig> && requires { source.filterMode; })
            if (control == 0 && source.filterMode == FilterMode::Bypass) return false;
        if constexpr (std::is_same_v<T, PsgConfig>) return control == 2 || control == 3 || control == 4;
        if constexpr (std::is_same_v<T, NoiseConfig>) return control != 5;
        if constexpr (std::is_same_v<T, DrumKitConfig> || std::is_same_v<T, DrumConfig>) return control != 3 && control != 4;
        return true;
    }, sound.source);
}

void ApplyToneValues(ToneVersion& tone)
{
    tone.instrument = tone.base;
    auto& sound = tone.instrument.sound;
    const auto& v = tone.values;
    if (std::all_of(v.begin(), v.end(), [](float value) { return value == 0; })) return;
    // A full turn is deliberately less than an octave of filter/envelope change.
    sound.releaseSec *= std::exp2(v[2] * .8);
    if (v[3] != 0) sound.attackSec = std::max(0.0001, sound.attackSec * std::exp2(v[3]) + std::max(0.f, v[3]) * .025);
    sound.decaySec *= std::exp2(v[4] * .8);
    std::visit([&](auto& source) {
        using T = std::decay_t<decltype(source)>;
        if constexpr (std::is_same_v<T, FmConfig>)
        {
            source.brightness = std::clamp(source.brightness + v[0] * .12, 0.0, 1.0);
            // Continuous drive complements the chip's discrete feedback steps.
            source.drive = std::clamp(source.drive + v[1] * .075, 0.0, 1.0);
            source.feedback = std::clamp(source.feedback + v[1] * .15, 0.0, 1.0);
            for (auto& op : source.ops)
            {
                op.levelEnv.attackSec *= std::exp2(v[3]);
                op.levelEnv.decaySec *= std::exp2(v[4] * .8);
                op.levelEnv.releaseSec *= std::exp2(v[2] * .8);
            }
        }
        else if constexpr (std::is_same_v<T, DrumKitConfig>)
            for (auto& drum : source.map) AdjustDrum(drum, v);
        else if constexpr (std::is_same_v<T, DrumConfig>) AdjustDrum(source, v);
        else if constexpr (requires { source.filterCutoffHz; source.filterResonance; })
        {
            source.filterCutoffHz = std::clamp(source.filterCutoffHz * std::exp2(v[0] * .7), 20.0, 18000.0);
            source.filterResonance = std::clamp(source.filterResonance + v[1] * .25, .1, 6.0);
            if constexpr (requires { source.drive; }) source.drive = std::clamp(source.drive + v[1] * .075, 0.0, 1.0);
        }
        if constexpr (requires { source.modulation; })
        {
            if (v[5] != 0)
            {
                auto& mod = source.modulation;
                bool routed = false;
                for (auto& route : mod.matrix.routes)
                    if (route.enabled && route.source == ModSource::Lfo1) routed = true;
                if (!routed)
                    for (auto& route : mod.matrix.routes) if (!route.enabled)
                    { route = {ModSource::Lfo1, ModDestination::Pitch, .018, true}; break; }
                mod.lfo1.depth = std::clamp(mod.lfo1.depth + v[5] * .25, 0.0, 1.0);
            }
        }
    }, sound.source);
}

void BeginToneEdit(GUIState& state)
{
    if (state.toneEditBefore) return;
    state.toneEditChannel = state.pianoRoll.displayChannel;
    state.toneEditBefore = std::make_unique<ToneVersion>(state.tones[state.toneEditChannel].draft);
    state.tones[state.toneEditChannel].compare = false;
}
void UpdateToneControls(GUIState& state)
{
    const int ch = state.pianoRoll.displayChannel;
    ApplyToneValues(state.tones[ch].draft);
    SyncDraft(state, ch);
}
void FinishToneEdit(GUIState& state)
{
    if (!state.toneEditBefore) return;
    auto& part = state.tones[state.toneEditChannel];
    if (!SameTone(*state.toneEditBefore, part.draft)) PushUndo(part, *state.toneEditBefore);
    state.toneEditBefore.reset(); state.toneEditChannel = -1;
}
void UndoToneEdit(GUIState& state, bool redo)
{
    FinishToneEdit(state);
    const int ch = state.pianoRoll.displayChannel;
    auto& part = state.tones[ch];
    auto& source = redo ? part.redo : part.undo;
    auto& destination = redo ? part.undo : part.redo;
    if (source.empty()) return;
    destination.push_back(part.draft);
    part.draft = std::move(source.back()); source.pop_back();
    part.compare = false; SyncDraft(state, ch);
}
void AdoptTone(GUIState& state, int channel)
{
    FinishToneEdit(state);
    auto& part = state.tones[channel];
    part.adopted = part.draft;
    part.compare = false; part.undo.clear(); part.redo.clear();
    SyncDraft(state, channel);
}
void CancelTone(GUIState& state, int channel)
{
    FinishToneEdit(state);
    auto& part = state.tones[channel];
    part.cache[ToneCacheKey(part.draft)] = part.draft;
    part.draft = part.adopted;
    part.compare = false; part.undo.clear(); part.redo.clear();
    // Keep the exploratory cache even when returning to the adopted sound.
    state.instruments[channel] = part.draft.instrument;
    state.presetDirty = true;
}
void AdoptAllTones(GUIState& state)
{
    for (int ch = 0; ch < 16; ++ch) if (TonePending(state, ch)) AdoptTone(state, ch);
}
int ChooseAuditionNote(const GUIState& state, int channel)
{
    channel = std::clamp(channel, 0, 15);
    if (state.toneWorkspaceReady && state.tones[channel].auditionNote >= 0)
        return std::clamp(state.tones[channel].auditionNote, 0, 127);
    std::array<int, 128> counts{};
    for (const auto& note : state.pianoRoll.notes) if (note.channel == channel) ++counts[std::clamp(note.note, 0, 127)];
    const auto most = std::max_element(counts.begin(), counts.end());
    if (*most > 0) return static_cast<int>(most - counts.begin());
    return std::clamp(state.instruments[AssignedSoundSlot(state, channel)].recommendedRange.preview, 0, 127);
}
}
