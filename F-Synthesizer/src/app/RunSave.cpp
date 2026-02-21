#include "RunInternal.h"

#include <sstream>

#include "Writer.h"
#include "io/PlatformPaths.h"

namespace app::run
{
int SaveRunOutput(
    const AppConfig& config,
    const RenderOptions& options,
    const SoundData& sound,
    IRunObserver* observer)
{
    if (!options.writeWav)
    {
        // Preview経路では保存I/Oを行わず、呼び出し側へ成功を返す。
        LogLine(observer, "Preview render completed (memory only, no WAV write).");
        return 0;
    }

    if (std::filesystem::exists(config.wavPath))
    {
        std::error_code rmEc;
        std::filesystem::remove(config.wavPath, rmEc);
        if (rmEc)
        {
            LogLine(observer, "[SavePrep] failed to remove old file: " + rmEc.message());
        }
    }

    WavWriteError err{};
    if (!SaveWavFilePath(sound, config.wavPath, &err))
    {
        // 保存失敗は path + cause + hint 形式へそろえて上位ログへ返す。
        std::ostringstream cause;
        cause << err.cause
            << " code=" << err.code
            << " errno=" << err.errnoValue
            << " winerr=" << err.systemError;
        LogLine(observer, FormatPathDiagnostic("save wav", config.wavPath, cause.str(), err.hint));
        return 1;
    }

    LogLine(observer, "Saved SoundData: " + PathToUtf8(config.wavPath));
    return 0;
}
} // namespace app::run
