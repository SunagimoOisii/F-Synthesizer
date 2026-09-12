#pragma once
struct GUIState;
struct GUIStepSeqState;
namespace gui
{
struct PianoRollState;
}
namespace studio
{
void LoadStepSeqFromPianoRoll(GUIStepSeqState &state, const gui::PianoRollState &pianoRoll);
void DrawStepSeqPanel(GUIState &state);
} // namespace studio
