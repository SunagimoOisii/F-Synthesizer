#include "synth/YmfmVoice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include "ymfm_opm.h"
#include "ymfm_opn.h"
#include "third_party/miniaudio.h"

namespace
{
constexpr int kCarriers[8] = { 8, 8, 8, 8, 10, 14, 14, 15 };
int EnvelopeRate(double seconds, bool attack)
{
    if (seconds <= 0.001) return 31;
    // User-facing times approximate the chip's logarithmic, quantized rates.
    return std::clamp(static_cast<int>(std::lround((attack ? 19.0 : 22.0) - 4.0 * std::log2(seconds / 0.1))), 1, 31);
}
int TotalLevel(double level)
{
    return level <= 0.00001 ? 127 : std::clamp(static_cast<int>(std::lround(-20.0 * std::log10(level) / 0.75)), 0, 127);
}
}

struct YmfmVoice::Impl
{
    ymfm::ymfm_interface interface;
    std::unique_ptr<ymfm::ym2151> opm;
    std::unique_ptr<ymfm::ym2612> opn;
    std::array<int, 256> registers;
    ma_resampler resampler{};
    std::array<float, 64> input{}, output{};
    size_t inputOffset = 0, inputCount = 0, outputOffset = 0, outputCount = 0;
    bool keyed = false;
    const bool megaDrive;

    Impl(int sampleRate, int chip) : megaDrive(chip == 1)
    {
        registers.fill(-1);
        unsigned nativeRate;
        if (megaDrive)
        {
            opn = std::make_unique<ymfm::ym2612>(interface);
            opn->reset();
            nativeRate = opn->sample_rate(7670454);
            Write(0xb4, 0xc0);
            Write(0x2b, 0);
        }
        else
        {
            opm = std::make_unique<ymfm::ym2151>(interface);
            opm->reset();
            nativeRate = opm->sample_rate(3579545);
        }
        auto config = ma_resampler_config_init(ma_format_f32, 1, nativeRate, sampleRate, ma_resample_algorithm_linear);
        config.linear.lpfOrder = 4;
        if (ma_resampler_init(&config, nullptr, &resampler) != MA_SUCCESS)
            throw std::runtime_error("FM sample-rate conversion could not be initialized");
    }
    ~Impl() { ma_resampler_uninit(&resampler, nullptr); }

    void Write(int address, int value)
    {
        value &= 255;
        if (registers[address] == value) return;
        registers[address] = value;
        if (megaDrive) { opn->write(0, static_cast<uint8_t>(address)); opn->write(1, static_cast<uint8_t>(value)); }
        else { opm->write(0, static_cast<uint8_t>(address)); opm->write(1, static_cast<uint8_t>(value)); }
    }

    void Update(const FmConfig& config, double frequencyHz, double indexScale, bool released)
    {
        const int algorithm = std::clamp(config.algorithm, 0, 7);
        const int feedback = std::clamp(static_cast<int>(std::lround(config.feedback * 7)), 0, 7);
        Write(megaDrive ? 0xb0 : 0x20, algorithm | (feedback << 3) | (megaDrive ? 0 : 0xc0));
        constexpr int opmOffsets[] = { 0, 16, 8, 24 };
        constexpr int opnOffsets[] = { 0, 8, 4, 12 };
        for (int i = 0; i < 4; ++i)
        {
            const auto& op = config.ops[i];
            const int offset = megaDrive ? opnOffsets[i] : opmOffsets[i];
            const int multiple = op.ratio < 0.75 ? 0 : std::clamp(static_cast<int>(std::lround(op.ratio)), 1, 15);
            double level = op.level;
            if ((kCarriers[algorithm] & (1 << i)) == 0)
                level *= std::clamp(op.index * indexScale * std::exp2((config.brightness - 0.5) * 4.0) / 4.0, 0.0, 1.0);
            const auto& env = op.levelEnv;
            const int sustain = std::clamp(TotalLevel(env.sustainLevel) / 4, 0, 15);
            const int release = std::clamp((EnvelopeRate(env.releaseSec, false) - 1) / 2, 0, 15);
            Write((megaDrive ? 0x30 : 0x40) + offset, multiple);
            Write((megaDrive ? 0x40 : 0x60) + offset, TotalLevel(level));
            Write((megaDrive ? 0x50 : 0x80) + offset, EnvelopeRate(env.attackSec, true));
            Write((megaDrive ? 0x60 : 0xa0) + offset, EnvelopeRate(env.decaySec, false));
            Write((megaDrive ? 0x70 : 0xc0) + offset, 0); // Hold the sustain level until key-off.
            Write((megaDrive ? 0x80 : 0xe0) + offset, (sustain << 4) | release);
        }
        const double hz = std::clamp(frequencyHz, 8.0, 12500.0);
        if (megaDrive)
        {
            int block = 0;
            double number = hz * 144.0 * 1048576.0 * 2.0 / 7670454.0;
            while (number > 2047.0 && block < 7) { number *= 0.5; ++block; }
            const int fnum = std::clamp(static_cast<int>(std::lround(number)), 0, 2047);
            // High bits latch only when the low register is written.
            const int high = (block << 3) | (fnum >> 8);
            if (registers[0xa4] != high) { Write(0xa4, high); registers[0xa0] = -1; }
            Write(0xa0, fnum & 255);
        }
        else
        {
            constexpr int keyCodes[] = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14 };
            const double note = std::clamp(69.0 + 12.0 * std::log2(hz / 440.0) - 13.0, 0.0, 95.984375);
            const int semitone = static_cast<int>(note);
            Write(0x28, ((semitone / 12) << 4) | keyCodes[semitone % 12]);
            Write(0x30, std::clamp(static_cast<int>((note - semitone) * 64.0), 0, 63) << 2);
        }
        if (keyed != !released)
        {
            keyed = !released;
            Write(megaDrive ? 0x28 : 0x08, keyed ? (megaDrive ? 0xf0 : 0x78) : 0);
        }
    }

    double Sample(const FmConfig& config, double frequencyHz, double indexScale, bool released)
    {
        if (outputOffset == outputCount)
        {
            Update(config, frequencyHz, indexScale, released);
            outputOffset = outputCount = 0;
            while (outputCount == 0)
            {
                if (inputOffset == inputCount)
                {
                    if (megaDrive)
                    {
                        ymfm::ym2612::output_data frames[64];
                        opn->generate(frames, 64);
                        for (int i = 0; i < 64; ++i) input[i] = static_cast<float>(frames[i].data[0]) / 16384.0f;
                    }
                    else
                    {
                        ymfm::ym2151::output_data frames[64];
                        opm->generate(frames, 64);
                        for (int i = 0; i < 64; ++i) input[i] = static_cast<float>(frames[i].data[0]) / 16384.0f;
                    }
                    inputOffset = 0; inputCount = 64;
                }
                ma_uint64 consumed = inputCount - inputOffset, produced = output.size();
                if (ma_resampler_process_pcm_frames(&resampler, input.data() + inputOffset, &consumed, output.data(), &produced) != MA_SUCCESS)
                    throw std::runtime_error("FM sample-rate conversion failed");
                inputOffset += static_cast<size_t>(consumed);
                outputCount = static_cast<size_t>(produced);
            }
        }
        return output[outputOffset++];
    }
};

YmfmVoice::YmfmVoice(int sampleRate, int chip) : impl_(std::make_unique<Impl>(sampleRate, chip)) {}
YmfmVoice::~YmfmVoice() = default;
double YmfmVoice::Sample(const FmConfig& config, double frequencyHz, double indexScale, bool released)
{
    return impl_->Sample(config, frequencyHz, indexScale, released);
}
