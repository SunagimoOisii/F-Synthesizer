#pragma once

#include <memory>
#include "SynthEngine/FmConfig.h"

// One emulated channel per MIDI voice; ownership stays on the render thread.
class YmfmVoice
{
public:
    YmfmVoice(int sampleRate, int chip);
    ~YmfmVoice();
    YmfmVoice(const YmfmVoice&) = delete;
    YmfmVoice& operator=(const YmfmVoice&) = delete;
    double Sample(const FmConfig& config, double frequencyHz, double indexScale, bool released);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
