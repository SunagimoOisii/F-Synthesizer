#pragma once

#include <array>
#include <deque>
#include <map>
#include <string>
#include "project/ProjectModel.h"

struct GUIState;

namespace gui
{
// Values are offsets from a copied preset, never edits to the preset file.
struct ToneVersion
{
    std::string key;
    InstrumentConfig base;
    InstrumentConfig instrument;
    std::array<float, 6> values{}; // brightness, texture, release, attack, decay, motion
    bool customizedBase = false;
};

struct ChannelToneWorkspace
{
    ToneVersion adopted;
    ToneVersion draft;
    std::map<std::string, ToneVersion> cache;
    std::string category;
    int auditionNote = -1; // -1: choose from the part
    bool compare = false;
    std::deque<ToneVersion> undo, redo;
};

void InitializeToneWorkspace(GUIState& state, bool reset = false);
bool TonePending(const GUIState& state, int channel);
int PendingToneCount(const GUIState& state);
const InstrumentConfig& AudibleInstrument(const GUIState& state, int channel);
void SelectToneChannel(GUIState& state, int channel);
bool SelectTonePreset(GUIState& state, int presetIndex, std::string& error);
void BeginToneEdit(GUIState& state);
void UpdateToneControls(GUIState& state);
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
