#pragma once
#include <string>
struct GUIState;
namespace studio
{
// The GUI loop owns dialog state; preset/song data stays in the existing IO APIs.
class FileActions
{
  public:
    void chooseFile(GUIState &state, bool song);
    void requestOperation(GUIState &state, int operation);
    void finishOperation(GUIState &state);
    void favorites(GUIState &state, float x, float y, float w);
    void dialogs(GUIState &state);

  private:
    bool saveSong(GUIState &state, bool saveAs);
    void openFile(GUIState &state);
    struct WindowState
    {
        int operation = 0; // 0: none, 1: save, 2: save as, 3: WAV
        bool confirmOperation = false;
        std::string openPath;
        bool openSong = false, openAfterSave = false;
        std::string renameKey;
        char renameText[128]{};
    } windowState;
};
} // namespace studio
