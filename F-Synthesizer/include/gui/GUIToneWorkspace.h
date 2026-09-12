#pragma once

#include <array>
#include <deque>
#include <map>
#include <string>
#include "project/ProjectModel.h"

struct GUIState;

namespace gui
{
// Trials and undo share an immutable starting sound. Only the active versions
// materialize their adjusted instrument; remembering a trial does not copy it.
struct ToneSnapshot
{
    std::string key;
    std::string presetRevision;
    std::shared_ptr<const InstrumentConfig> base;
    std::array<float, 6> values{}; // brightness, texture, release, attack, decay, motion
    bool customizedBase = false;
    bool adjusted = false; // Actual sound difference, captured with the snapshot.
};

struct ToneVersion : ToneSnapshot
{
    InstrumentConfig instrument;
};

ToneSnapshot RememberTone(const ToneVersion& tone);
ToneVersion RestoreTone(ToneSnapshot snapshot);

inline std::string ToneCacheKey(const std::string& key, const std::string& revision)
{
    return revision.empty() ? key : key + "\n" + revision;
}
inline std::string ToneCacheKey(const ToneSnapshot& tone)
{
    return ToneCacheKey(tone.key, tone.presetRevision);
}

struct ChannelToneWorkspace
{
    ToneVersion adopted;
    ToneVersion draft;
    std::map<std::string, ToneSnapshot> cache;
    std::string category;
    int auditionNote = -1; // -1: choose from the part
    bool compare = false;
    std::deque<ToneSnapshot> undo, redo;
};

void InitializeToneWorkspace(GUIState& state, bool reset = false);
bool TonePending(const GUIState& state, int channel);
int PendingToneCount(const GUIState& state);
const InstrumentConfig& AudibleInstrument(const GUIState& state, int channel);
void SelectToneChannel(GUIState& state, int channel);
bool SelectTonePreset(GUIState& state, int presetIndex, std::string& error);
void BeginToneEdit(GUIState& state);
void UpdateToneControls(GUIState& state);
// Applies a detailed edit to the selected channel through the same history as macros.
// FinishToneEdit closes the gesture; other channels and preset files are untouched.
void ApplyDetailedToneEdit(GUIState& state, const InstrumentSoundConfig& sound);
void FinishToneEdit(GUIState& state);
void UndoToneEdit(GUIState& state, bool redo = false);
void AdoptTone(GUIState& state, int channel);
void CancelTone(GUIState& state, int channel);
void AdoptAllTones(GUIState& state);
bool ToneControlSupported(const InstrumentSoundConfig& sound, int control);
int ChooseAuditionNote(const GUIState& state, int channel);
void ApplyToneValues(ToneVersion& tone);
std::string InferPartCategory(const GUIState& state, int channel);
}
