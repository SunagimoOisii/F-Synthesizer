#include "RunInternal.h"

#include <sstream>

#include "io/Writer.h"
#include "io/PlatformPaths.h"
#include "project/ProjectModel.h"

namespace app::run
{
int SaveRunOutput(
    const ProjectModel& project,
    const RenderOptions& options,
    const SoundData& sound,
    IRunObserver* observer)
{
    if (!options.writeWAV)
    {
        // Preview経路では保存I/Oを行わず、呼び出し側へ成功を返す。
        LogLine(observer, "Preview render completed (memory only, no WAV write).");
        return 0;
    }

    std::error_code existsEc;
    if (std::filesystem::exists(project.wavPath, existsEc))
    {
        std::error_code rmEc;
        std::filesystem::remove(project.wavPath, rmEc);
        if (rmEc)
        {
            LogLine(observer, "[SavePrep] failed to remove old file: " + rmEc.message());
        }
    }
    else if (existsEc)
    {
        LogLine(observer, "[SavePrep] failed to inspect old file: " + existsEc.message());
    }

    WAVWriteError err{};
    if (!SaveWAVFilePath(sound, project.wavPath, &err))
    {
        // 保存失敗は path + cause + hint 形式へそろえて上位ログへ返す。
        std::ostringstream cause;
        cause << err.cause
            << " code=" << err.code
            << " errno=" << err.errnoValue
            << " winerr=" << err.systemError;
        LogLine(observer, FormatPathDiagnostic("save wav", project.wavPath, cause.str(), err.hint));
        return 1;
    }

    LogLine(observer, "Saved SoundData: " + PathToUtf8(project.wavPath));
    return 0;
}
} // namespace app::run
