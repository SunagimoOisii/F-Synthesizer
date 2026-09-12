#pragma once
#include "StudioWidgets.h"
#include <string>
struct GUIState;
struct InstrumentConfig;
namespace studio
{
inline constexpr const char *categories[] = {"Lead", "Bass", "Keys", "Pad", "Guitar", "Drums", "SFX", "Support"};
inline constexpr const char *categoryLabels[] = {"リード", "ベース", "鍵盤",   "パッド",
                                                 "ギター", "ドラム", "効果音", "電子音"};
inline constexpr Icon categoryGlyphs[] = {MusicIcon,  GuitarIcon, PianoIcon,    LayersIcon,
                                          GuitarIcon, DrumIcon,   SparklesIcon, ChipIcon};
int categoryIndex(const std::string &name);
bool matches(std::string name, std::string query);
std::string toneName(const InstrumentConfig &instrument);
std::string pitchName(int note);
void waveform(GUIState &state, float x, float y, float w, float h);
void toneControls(GUIState &state, float x, float y, float w);
void extraControls(GUIState &state, float x, float y, float w);
void channelStrip(GUIState &state, float width);
void mixControls(GUIState &state, float x, float y, float w);
void presetList(GUIState &state, float x, float y, float w, float h);
} // namespace studio
