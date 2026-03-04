#pragma once

#include <filesystem>
#include <string>

#include "core/AudioBuffer.h"

// WAV書き出し失敗時の診断情報。
// GUI/CLI共通で「原因 + OSエラー + 対処ヒント」を表示するために使う。
struct WAVWriteError
{
    std::string code;
    std::string cause;
    std::string hint;
    std::filesystem::path path;
    int errnoValue = 0;
    unsigned long systemError = 0;
};

// 目的: SoundData をモノラルPCM WAVとして保存する。
// 前提: outError を使わない経路。失敗理由は戻り値のみで判定する。
bool SaveWAVFilePath(const SoundData& sound, const std::filesystem::path& filePath);
// 目的: SaveWAVFilePath の詳細診断付き関数。
// 副作用: 失敗時は outError に詳細を格納し、成功時は outError を初期化する。
bool SaveWAVFilePath(const SoundData& sound, const std::filesystem::path& filePath, WAVWriteError* outError);
