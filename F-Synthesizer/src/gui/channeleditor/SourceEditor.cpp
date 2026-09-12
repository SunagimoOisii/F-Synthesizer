#include "SourceEditor.h"
#include "EnvelopeView.h"
#include "gui/GUIConfigUtils.h"
#include <algorithm>
#include <array>
#include <string>
#include <imgui.h>
namespace gui::detail
{
namespace
{
void ApplyFmTemplateByAlgorithm(FmConfig &fm, int algorithm)
{
    const int chip = fm.chip;
    fm = FmConfig{};
    fm.chip = chip;
    fm.algorithm = std::clamp(algorithm, 0, 7);
    constexpr int carriers[] = {8, 8, 8, 8, 10, 14, 14, 15};
    for (int i = 0; i < 4; ++i)
    {
        fm.ops[i].level = (carriers[fm.algorithm] & (1 << i)) ? 0.6 : 0.25;
        fm.ops[i].index = 4.0;
    }
}

bool DrawDrumConfigEditor(const char *IDPrefix, DrumConfig &d, const HoverHelpFn &updateHoverHelp)
{
    bool changed = false;
    int drumType = static_cast<int>(d.type);
    const char *drumTypes[] = {"none", "kick", "snare",  "hat",    "tom",     "rim",       "clap", "crash",
                               "ride", "bell", "shaker", "scrape", "whistle", "woodblock", "cuica"};
    std::string key = std::string("Drum Type##") + IDPrefix;
    changed |= ImGui::Combo(key.c_str(), &drumType, drumTypes, IM_ARRAYSIZE(drumTypes));
    if (updateHoverHelp)
    {
        updateHoverHelp("Drum Type を選択します。", "ドラム発音モデルが切り替わります。", nullptr);
    }
    ImGui::TextDisabled("DrumConfig: 0 = 未指定（内部デフォルト）");
    if (updateHoverHelp)
    {
        updateHoverHelp("DrumConfig の未指定値ルールを確認します。",
                        "数値パラメータを 0 にすると内部デフォルトが使われます。",
                        "明示値に戻す場合は 0 以外の値を入力してください。");
    }
    d.type = static_cast<DrumType>(drumType);

    if (d.type == DrumType::None)
    {
        return changed;
    }

    key = std::string("Gain##") + IDPrefix;
    changed |= ImGui::InputDouble(key.c_str(), &d.gain, 0.01, 0.1, "%.3f");
    if (updateHoverHelp)
    {
        updateHoverHelp("Drum Gain を調整します。", "該当ドラム音の音量が変わります。",
                        "上げすぎるとクリップしやすくなります。");
    }

    if (d.type >= DrumType::Bell)
    {
        key = std::string("Body Freq##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyFreq, 10.0, 100.0, "%.2f");
        if (d.type != DrumType::Shaker)
        {
            key = std::string("Body Level##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.bodyLevel, 0.01, 0.1, "%.3f");
        }
        if (d.type == DrumType::Bell || d.type == DrumType::Woodblock)
        {
            key = std::string("Body Decay##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.bodyDecaySec, 0.01, 0.1, "%.3f");
        }
        if (d.type == DrumType::Bell || d.type == DrumType::Shaker)
        {
            key = std::string("Metal Level##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.metalLevel, 0.01, 0.1, "%.3f");
        }
        if (d.type == DrumType::Whistle || d.type == DrumType::Cuica)
        {
            key = std::string("Pitch Start##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.pitchStart, 0.01, 0.1, "%.3f");
            key = std::string("Pitch Decay##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.001, 0.01, "%.3f");
        }
        if (d.type == DrumType::Shaker || d.type == DrumType::Scrape)
        {
            key = std::string("Pulse Interval##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.001, 0.01, "%.3f");
        }
        if (d.type != DrumType::Bell && d.type != DrumType::Woodblock)
        {
            key = std::string("Noise Level##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.noiseLevel, 0.01, 0.1, "%.3f");
        }
        key = std::string("Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.decaySec, 0.001, 0.01, "%.3f");
        key = std::string("Transient Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
        key = std::string("Transient Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        key = std::string("HP Cut##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
        key = std::string("LP Cut##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");
        key = std::string("Drive##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
    }
    else if (d.type == DrumType::Kick)
    {
        key = std::string("Body Freq##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyFreq, 1.0, 10.0, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Body Freq を調整します。", "キックの低域の芯が変わります。", nullptr);
        key = std::string("Body Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyLevel, 0.01, 0.1, "%.3f");
        key = std::string("Body Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyDecaySec, 0.01, 0.1, "%.3f");
        key = std::string("Pitch Start##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.pitchStart, 0.1, 1.0, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Pitch Start を調整します。", "キック開始時の高いピッチ量が変わります。", nullptr);
        key = std::string("Pitch Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Pitch Decay を調整します。", "キックのピッチ変化速度が変わります。", nullptr);
        key = std::string("Transient Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
        key = std::string("Transient Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        key = std::string("Drive##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
        if (updateHoverHelp)
            updateHoverHelp("Drive を調整します。", "ドラム専用ソフトクリップの強さが変わります。", nullptr);
    }
    else if (d.type == DrumType::Snare || d.type == DrumType::Tom || d.type == DrumType::Rim)
    {
        key = std::string("Body Freq##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyFreq, 10.0, 100.0, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Body Freq を調整します。", "胴鳴り/打撃トーンの周波数が変わります。", nullptr);
        key = std::string("Body Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyLevel, 0.01, 0.1, "%.3f");
        key = std::string("Body Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.bodyDecaySec, 0.01, 0.1, "%.3f");
        if (d.type == DrumType::Tom)
        {
            key = std::string("Pitch Start##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.pitchStart, 0.1, 1.0, "%.3f");
            key = std::string("Pitch Decay##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
        }
        if (d.type == DrumType::Snare)
        {
            key = std::string("Snap Level##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.snapLevel, 0.01, 0.1, "%.3f");
            key = std::string("Snap Decay##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.snapDecaySec, 0.001, 0.01, "%.3f");
        }
        key = std::string("Transient Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
        key = std::string("Transient Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        key = std::string("HP Cut##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("HP Cut を調整します。", "高域寄りに残す帯域が変わります。", nullptr);
        key = std::string("LP Cut##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("LP Cut を調整します。", "低域寄りに残す帯域が変わります。", nullptr);
        key = std::string("Drive##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
        if (updateHoverHelp)
            updateHoverHelp("Drive を調整します。", "ドラム専用ソフトクリップの強さが変わります。", nullptr);
    }
    else
    {
        key = std::string("Metal Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.metalLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Metal Level を調整します。", "ハットの硬い金属トーン量が変わります。", nullptr);
        key = std::string("Air Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.airLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Air Level を調整します。", "ハットの高域ノイズ量が変わります。", nullptr);
        key = std::string("Noise Level##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.noiseLevel, 0.01, 0.1, "%.3f");
        if (d.type == DrumType::Clap)
        {
            key = std::string("Transient Level##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
            key = std::string("Transient Decay##") + IDPrefix;
            changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        }
        key = std::string("Decay##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.decaySec, 0.001, 0.01, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Decay を調整します。", "ハットの短さが変わります。", nullptr);
        key = std::string("HP Cut##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("HP Cut を調整します。", "高域寄りに残す帯域が変わります。", nullptr);
        key = std::string("LP Cut##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("LP Cut を調整します。", "低域寄りに残す帯域が変わります。", nullptr);
        key = std::string("Drive##") + IDPrefix;
        changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
        if (updateHoverHelp)
            updateHoverHelp("Drive を調整します。", "ドラム専用ソフトクリップの強さが変わります。", nullptr);
    }

    key = std::string("Velocity -> Tone##") + IDPrefix;
    changed |= ImGui::InputDouble(key.c_str(), &d.velocityToTone, 0.01, 0.1, "%.3f");
    key = std::string("Velocity -> Decay##") + IDPrefix;
    changed |= ImGui::InputDouble(key.c_str(), &d.velocityToDecay, 0.01, 0.1, "%.3f");
    key = std::string("Humanize Pitch Cents##") + IDPrefix;
    changed |= ImGui::InputDouble(key.c_str(), &d.humanizePitchCents, 0.1, 1.0, "%.2f");
    key = std::string("Humanize Decay Pct##") + IDPrefix;
    changed |= ImGui::InputDouble(key.c_str(), &d.humanizeDecayPct, 0.01, 0.1, "%.3f");
    d.velocityToTone = std::clamp(d.velocityToTone, 0.0, 1.0);
    d.velocityToDecay = std::clamp(d.velocityToDecay, -1.0, 1.0);
    d.humanizePitchCents = std::clamp(d.humanizePitchCents, 0.0, 50.0);
    d.humanizeDecayPct = std::clamp(d.humanizeDecayPct, 0.0, 1.0);

    if (d.type == DrumType::Snare || d.type == DrumType::Hat || d.type == DrumType::Clap || d.type == DrumType::Crash ||
        d.type == DrumType::Ride)
    {
        int noiseColor = d.noiseColor;
        const char *noises[] = {"white", "pink", "brown", "blue"};
        key = std::string("Noise Color##") + IDPrefix;
        changed |= ImGui::Combo(key.c_str(), &noiseColor, noises, IM_ARRAYSIZE(noises));
        if (updateHoverHelp)
            updateHoverHelp("Noise Color を選択します。", "スナップ/エア成分の周波数傾向が変わります。", nullptr);
        d.noiseColor = noiseColor;
    }
    return changed;
}

bool sliderWaveParam(const char *label, double &value, float minV, float maxV, const char *fmt = "%.3f")
{
    float v = static_cast<float>(value);
    bool edited = ImGui::SliderFloat(label, &v, minV, maxV, fmt);
    if (edited)
    {
        value = static_cast<double>(v);
    }
    return edited;
}
bool drawModulationEditor(const char *idPrefix, ModulationConfig &modulation, bool allowFilterCutoff, bool allowFmIndex,
                          bool allowPulseWidth, const HoverHelpFn &updateHoverHelp)
{
    bool localChanged = false;
    ImGui::Separator();
    ImGui::TextUnformatted("モジュレーション");

    const char *lfoWaves[] = {"Sine", "Triangle", "Square", "Saw", "S&H"};
    int lfoWaveIdx = 0;
    switch (modulation.lfo1.wave)
    {
    case LfoWave::Sine:
        lfoWaveIdx = 0;
        break;
    case LfoWave::Triangle:
        lfoWaveIdx = 1;
        break;
    case LfoWave::Square:
        lfoWaveIdx = 2;
        break;
    case LfoWave::Saw:
        lfoWaveIdx = 3;
        break;
    case LfoWave::SampleAndHold:
        lfoWaveIdx = 4;
        break;
    }
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::Combo("LFO1 波形", &lfoWaveIdx, lfoWaves, IM_ARRAYSIZE(lfoWaves)))
    {
        switch (lfoWaveIdx)
        {
        case 0:
            modulation.lfo1.wave = LfoWave::Sine;
            break;
        case 1:
            modulation.lfo1.wave = LfoWave::Triangle;
            break;
        case 2:
            modulation.lfo1.wave = LfoWave::Square;
            break;
        case 3:
            modulation.lfo1.wave = LfoWave::Saw;
            break;
        case 4:
            modulation.lfo1.wave = LfoWave::SampleAndHold;
            break;
        default:
            modulation.lfo1.wave = LfoWave::Sine;
            break;
        }
        localChanged = true;
    }
    if (updateHoverHelp)
        updateHoverHelp("LFO1 の波形を選択します。",
                        "変調の形が変わります。Sine=なめらか、Square=段階的、Saw=のこぎり、S&H=ランダムホールド。",
                        nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("LFO1 Rate (Hz)", modulation.lfo1.rateHz, 0.0f, 100.0f, "%.2f");
    if (updateHoverHelp)
        updateHoverHelp("LFO1 Rate を調整します。", "周期変調の速さが変わります。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("LFO1 Depth", modulation.lfo1.depth, 0.0f, 1.0f, "%.3f");
    if (updateHoverHelp)
        updateHoverHelp("LFO1 Depth を調整します。", "LFOの変調量が変わります。", nullptr);
    localChanged |= ImGui::Checkbox("LFOを±方向で効かせる", &modulation.lfo1.bipolar);
    if (updateHoverHelp)
        updateHoverHelp("LFO1 Bipolar を切り替えます。", "LFO出力の極性レンジが変わります。", nullptr);
    localChanged |= ImGui::Checkbox("鍵盤ごとに揺れをリセット", &modulation.lfo1.keySync);
    if (updateHoverHelp)
    {
        updateHoverHelp("ノートオン時に LFO 位相を 0 にリセットします。",
                        "オフの場合は free-run（位相を引き継ぐ）です。", "");
    }
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("LFO1 Delay (ms)", modulation.lfo1.delayMs, 0.0f, 2000.0f, "%.1f");
    if (updateHoverHelp)
        updateHoverHelp("LFO1 Delay を調整します。", "ノートオン後にLFOが有効になるまでの待機時間が変わります。",
                        nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("LFO1 Fade (ms)", modulation.lfo1.fadeMs, 0.0f, 2000.0f, "%.1f");
    if (updateHoverHelp)
        updateHoverHelp("LFO1 Fade を調整します。", "LFOが最大深さに達するまでの立ち上がり時間が変わります。", nullptr);
    ImGui::TextDisabled("LFO1 波形");
    DrawLfo1WavePreview((std::string("##lfo1_preview_") + idPrefix).c_str(), modulation.lfo1.wave);

    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("Env2 Attack", modulation.env2.attackSec, 0.0f, 10.0f, "%.3f");
    if (updateHoverHelp)
        updateHoverHelp("Env2 Attack を調整します。", "変調が最大値に達するまでの立ち上がり時間が変わります。",
                        nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("Env2 Decay", modulation.env2.decaySec, 0.0f, 10.0f, "%.3f");
    if (updateHoverHelp)
        updateHoverHelp("Env2 Decay を調整します。", "ピーク後にサステインレベルへ落ちるまでの減衰時間が変わります。",
                        nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("Env2 Sustain", modulation.env2.sustainLevel, 0.0f, 1.0f, "%.3f");
    if (updateHoverHelp)
        updateHoverHelp("Env2 Sustain を調整します。", "ノート押下中に維持する変調量が変わります (0〜1)。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("Env2 Release", modulation.env2.releaseSec, 0.0f, 10.0f, "%.3f");
    if (updateHoverHelp)
        updateHoverHelp("Env2 Release を調整します。",
                        "ノートオフ後に変調量がゼロに戻るまでのリリース時間が変わります。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("Env2 Curve", modulation.env2.curve, 0.0f, 1.0f, "%.3f");
    if (updateHoverHelp)
        updateHoverHelp("Env2 Curve を調整します。",
                        "変化の加速感が変わります。低いと急激に、高いとなだらかに変化します。", nullptr);
    ImGui::TextDisabled("Env2 エンベロープ");
    float env2AttackDrag = static_cast<float>(modulation.env2.attackSec);
    float env2DecayDrag = static_cast<float>(modulation.env2.decaySec);
    float env2SustainDrag = static_cast<float>(modulation.env2.sustainLevel);
    float env2ReleaseDrag = static_cast<float>(modulation.env2.releaseSec);
    const bool env2Dragged =
        DrawADSRPreview((std::string("##env2_preview_") + idPrefix).c_str(), env2AttackDrag, env2DecayDrag,
                        env2SustainDrag, env2ReleaseDrag, static_cast<float>(modulation.env2.curve), &env2AttackDrag,
                        &env2DecayDrag, &env2SustainDrag, &env2ReleaseDrag);
    if (env2Dragged)
    {
        modulation.env2.attackSec = std::clamp(static_cast<double>(env2AttackDrag), 0.0, 10.0);
        modulation.env2.decaySec = std::clamp(static_cast<double>(env2DecayDrag), 0.0, 10.0);
        modulation.env2.sustainLevel = std::clamp(static_cast<double>(env2SustainDrag), 0.0, 1.0);
        modulation.env2.releaseSec = std::clamp(static_cast<double>(env2ReleaseDrag), 0.0, 10.0);
        localChanged = true;
    }
    if (updateHoverHelp)
    {
        updateHoverHelp("Env2 の点をドラッグして直接編集します。", "Env2 の ADSR 形状をグラフ上で調整できます。",
                        "ピーク点・減衰終点・リリース開始点をドラッグできます。");
    }

    const char *modSources[] = {"none", "lfo1", "env2", "velocity", "channelPressure", "polyPressure", "ModWheel"};
    struct DestinationChoice
    {
        const char *label;
        ModDestination value;
    };
    std::array<DestinationChoice, 7> destinationChoices{{
        {"none", ModDestination::None},
        {"pitchMul", ModDestination::Pitch},
        {"amp", ModDestination::Amp},
        {"filterCutoffHz", ModDestination::FilterCutoff},
        {"filterResonance", ModDestination::FilterResonance},
        {"PulseWidth", ModDestination::PulseWidth},
        {"fm.index", ModDestination::FmIndex},
    }};
    int destinationCount = 3;
    if (allowFilterCutoff)
    {
        destinationCount += 2;
    }
    if (allowPulseWidth)
    {
        destinationChoices[destinationCount++] = {"PulseWidth", ModDestination::PulseWidth};
    }
    if (allowFmIndex)
    {
        destinationChoices[destinationCount++] = {"fm.index", ModDestination::FmIndex};
    }

    auto destinationLabel = [&](ModDestination destination) -> const char * {
        for (int i = 0; i < destinationCount; i++)
        {
            if (destinationChoices[i].value == destination)
            {
                return destinationChoices[i].label;
            }
        }
        return "none";
    };
    auto destinationIndex = [&](ModDestination destination) -> int {
        for (int i = 0; i < destinationCount; i++)
        {
            if (destinationChoices[i].value == destination)
            {
                return i;
            }
        }
        return 0;
    };

    ImGui::PushID(idPrefix);
    const int routeCount = static_cast<int>(modulation.matrix.routes.size());
    for (int routeIdx = 0; routeIdx < routeCount; routeIdx++)
    {
        ModRoute &route = modulation.matrix.routes[static_cast<size_t>(routeIdx)];
        ImGui::PushID(routeIdx);

        std::string summary;
        bool hasRoute = (route.source != ModSource::None && route.destination != ModDestination::None);
        if (hasRoute)
        {
            char amtBuf[16];
            snprintf(amtBuf, sizeof(amtBuf), "%+.2f", static_cast<float>(route.amount));
            summary = "Route " + std::to_string(routeIdx) + ": " + modSources[static_cast<int>(route.source)] + " -> " +
                      destinationLabel(route.destination) + " (" + amtBuf + ")" + (route.enabled ? "" : " [off]");
        }
        else
        {
            summary = "Route " + std::to_string(routeIdx) + ": (empty)";
        }

        if (!ImGui::CollapsingHeader(summary.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PopID();
            continue;
        }

        localChanged |= ImGui::Checkbox("有効", &route.enabled);
        if (updateHoverHelp)
            updateHoverHelp("Route Enabled を切り替えます。", "このモジュレーション経路の有効/無効が変わります。",
                            nullptr);

        int srcIdx = 0;
        switch (route.source)
        {
        case ModSource::None:
            srcIdx = 0;
            break;
        case ModSource::Lfo1:
            srcIdx = 1;
            break;
        case ModSource::Env2:
            srcIdx = 2;
            break;
        case ModSource::Velocity:
            srcIdx = 3;
            break;
        case ModSource::ChannelPressure:
            srcIdx = 4;
            break;
        case ModSource::PolyPressure:
            srcIdx = 5;
            break;
        case ModSource::ModWheel:
            srcIdx = 6;
            break;
        }
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("変調元", &srcIdx, modSources, IM_ARRAYSIZE(modSources)))
        {
            switch (srcIdx)
            {
            case 0:
                route.source = ModSource::None;
                break;
            case 1:
                route.source = ModSource::Lfo1;
                break;
            case 2:
                route.source = ModSource::Env2;
                break;
            case 3:
                route.source = ModSource::Velocity;
                break;
            case 4:
                route.source = ModSource::ChannelPressure;
                break;
            case 5:
                route.source = ModSource::PolyPressure;
                break;
            case 6:
                route.source = ModSource::ModWheel;
                break;
            default:
                route.source = ModSource::None;
                break;
            }
            localChanged = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Route Source を選択します。", "変調元が変わります。", nullptr);

        int dstIdx = destinationIndex(route.destination);
        const char *destinationLabels[7] = {};
        for (int i = 0; i < destinationCount; i++)
        {
            destinationLabels[i] = destinationChoices[i].label;
        }
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("変調先", &dstIdx, destinationLabels, destinationCount))
        {
            route.destination = destinationChoices[dstIdx].value;
            localChanged = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Route Destination を選択します。", "変調先パラメータが変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Amount", route.amount, -1.0f, 1.0f, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Route Amount を調整します。", "変調量と極性が変わります。", nullptr);
        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::TextDisabled("モジュレーション配線図");
    const ImVec2 canvasSize = ImVec2(0.0f, 190.0f);
    ImGui::BeginChild((std::string("##mod_route_view_") + idPrefix).c_str(), canvasSize, true);
    {
        struct SourceNode
        {
            ModSource source;
            const char *label;
        };
        constexpr std::array<SourceNode, 6> sourceNodes{{
            {ModSource::Lfo1, "LFO1"},
            {ModSource::Env2, "Env2"},
            {ModSource::Velocity, "Velocity"},
            {ModSource::ChannelPressure, "ChPressure"},
            {ModSource::PolyPressure, "PolyPressure"},
            {ModSource::ModWheel, "ModWheel"},
        }};

        std::array<ModDestination, 7> destinationNodes{};
        int destinationNodeCount = 0;
        for (int i = 0; i < destinationCount; i++)
        {
            const ModDestination destination = destinationChoices[i].value;
            if (destination == ModDestination::None)
            {
                continue;
            }
            destinationNodes[static_cast<size_t>(destinationNodeCount++)] = destination;
        }
        if (destinationNodeCount <= 0)
        {
            destinationNodes[0] = ModDestination::Amp;
            destinationNodeCount = 1;
        }

        auto sourceIndexOf = [&](ModSource source) -> int {
            for (int i = 0; i < static_cast<int>(sourceNodes.size()); i++)
            {
                if (sourceNodes[static_cast<size_t>(i)].source == source)
                {
                    return i;
                }
            }
            return -1;
        };
        auto destinationIndexOf = [&](ModDestination destination) -> int {
            for (int i = 0; i < destinationNodeCount; i++)
            {
                if (destinationNodes[static_cast<size_t>(i)] == destination)
                {
                    return i;
                }
            }
            return -1;
        };

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const ImVec2 topLeft = ImGui::GetCursorScreenPos();
        const float width = (std::max)(220.0f, ImGui::GetContentRegionAvail().x - 8.0f);
        const float leftX = topLeft.x + 8.0f;
        const float rightX = topLeft.x + width - 144.0f;
        const float nodeW = 132.0f;
        const float nodeH = 22.0f;
        const float yTop = topLeft.y + 8.0f;
        const float leftSpan = (sourceNodes.size() > 1) ? 148.0f / static_cast<float>(sourceNodes.size() - 1) : 0.0f;
        const float rightSpan =
            (destinationNodeCount > 1) ? 148.0f / static_cast<float>(destinationNodeCount - 1) : 0.0f;

        std::array<ImVec2, sourceNodes.size()> srcAnchors{};
        for (int i = 0; i < static_cast<int>(sourceNodes.size()); i++)
        {
            const float y = yTop + leftSpan * static_cast<float>(i);
            const ImVec2 p0 = ImVec2(leftX, y);
            const ImVec2 p1 = ImVec2(leftX + nodeW, y + nodeH);
            drawList->AddRectFilled(p0, p1, IM_COL32(56, 72, 96, 190), 4.0f);
            drawList->AddRect(p0, p1, IM_COL32(110, 130, 170, 220), 4.0f);
            drawList->AddText(ImVec2(p0.x + 8.0f, p0.y + 4.0f), IM_COL32(230, 236, 245, 255),
                              sourceNodes[static_cast<size_t>(i)].label);
            srcAnchors[static_cast<size_t>(i)] = ImVec2(p1.x, p0.y + nodeH * 0.5f);
        }

        std::array<ImVec2, 7> dstAnchors{};
        for (int i = 0; i < destinationNodeCount; i++)
        {
            const float y = yTop + rightSpan * static_cast<float>(i);
            const ImVec2 p0 = ImVec2(rightX, y);
            const ImVec2 p1 = ImVec2(rightX + nodeW, y + nodeH);
            drawList->AddRectFilled(p0, p1, IM_COL32(72, 88, 62, 190), 4.0f);
            drawList->AddRect(p0, p1, IM_COL32(120, 170, 120, 220), 4.0f);
            drawList->AddText(ImVec2(p0.x + 8.0f, p0.y + 4.0f), IM_COL32(236, 245, 230, 255),
                              destinationLabel(destinationNodes[static_cast<size_t>(i)]));
            dstAnchors[static_cast<size_t>(i)] = ImVec2(p0.x, p0.y + nodeH * 0.5f);
        }

        bool hasAnyRoute = false;
        for (const ModRoute &route : modulation.matrix.routes)
        {
            if (route.source == ModSource::None || route.destination == ModDestination::None)
            {
                continue;
            }
            const int srcIdx = sourceIndexOf(route.source);
            const int dstIdx = destinationIndexOf(route.destination);
            if (srcIdx < 0 || dstIdx < 0)
            {
                continue;
            }
            const ImVec2 p0 = srcAnchors[static_cast<size_t>(srcIdx)];
            const ImVec2 p3 = dstAnchors[static_cast<size_t>(dstIdx)];
            const float cdx = (p3.x - p0.x) * 0.4f;
            const ImVec2 p1 = ImVec2(p0.x + cdx, p0.y);
            const ImVec2 p2 = ImVec2(p3.x - cdx, p3.y);
            const ImU32 lineColor =
                route.enabled ? (route.amount >= 0.0 ? IM_COL32(110, 220, 140, 235) : IM_COL32(220, 140, 110, 235))
                              : IM_COL32(120, 120, 120, 150);
            const float thickness = route.enabled ? 2.4f : 1.2f;
            drawList->AddBezierCubic(p0, p1, p2, p3, lineColor, thickness);
            hasAnyRoute = true;
        }

        if (!hasAnyRoute)
        {
            drawList->AddText(ImVec2(topLeft.x + 12.0f, topLeft.y + 166.0f), IM_COL32(180, 180, 180, 220),
                              "No active routes");
        }

        ImGui::Dummy(ImVec2(width, 170.0f));
    }
    ImGui::EndChild();
    ImGui::PopID();
    return localChanged;
}
template <class Source>
bool drawWaveformLikeCoreEditor(Source &src, const char *hardSyncTag, const char *arpeggioTag,
                                const HoverHelpFn &updateHoverHelp)
{
    bool localChanged = false;
    int idx = WaveToIndex(src.wave);
    const char *waves[] = {"sine", "square", "saw", "triangle"};
    localChanged |= ImGui::Combo("波形", &idx, waves, IM_ARRAYSIZE(waves));
    if (updateHoverHelp)
        updateHoverHelp("Wave を選択します。", "基本波形キャラクターが変わります。", nullptr);
    src.wave = WaveFromIndex(idx);
    if (src.wave == WaveType::Square)
    {
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Pulse Width", src.pulseWidth, 0.05f, 0.95f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Pulse Width を調整します。",
                            "音の細さ・鋭さが変わります。LFO でゆっくり揺らすとクラリネット的な揺らぎになります。",
                            nullptr);
    }

    src.unisonVoices = std::clamp(src.unisonVoices, 1, 8);
    src.unisonDetuneCents = std::clamp(src.unisonDetuneCents, 0.0, 120.0);
    src.unisonSpread = std::clamp(src.unisonSpread, 0.0, 1.0);
    src.subOscLevel = std::clamp(src.subOscLevel, 0.0, 2.0);
    src.pulseWidth = std::clamp(src.pulseWidth, 0.05, 0.95);
    src.hardSyncRatio = std::clamp(src.hardSyncRatio, 0.5, 8.0);
    src.ringModRatio = std::clamp(src.ringModRatio, 0.125, 16.0);
    src.ringModMix = std::clamp(src.ringModMix, 0.0, 1.0);
    src.arpeggio.rateHz = std::clamp(src.arpeggio.rateHz, 0.5, 40.0);
    src.arpeggio.steps = std::clamp(src.arpeggio.steps, 1, 8);
    for (int &semitone : src.arpeggio.semitones)
    {
        semitone = std::clamp(semitone, -24, 24);
    }
    src.filterCutoffHz = std::clamp(src.filterCutoffHz, 10.0, 20000.0);
    src.filterResonance = std::clamp(src.filterResonance, 0.1, 18.0);
    src.filterKeytrack = std::clamp(src.filterKeytrack, 0.0, 1.0);
    src.filterDrive = std::clamp(src.filterDrive, 0.0, 1.0);

    int unisonVoices = src.unisonVoices;
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderInt("ユニゾン発音数", &unisonVoices, 1, 8))
    {
        src.unisonVoices = unisonVoices;
        localChanged = true;
    }
    if (updateHoverHelp)
        updateHoverHelp("Unison Voices を調整します。", "重ねる発音数が変わり厚みが変わります。",
                        "増やすほどCPU負荷が上がります。");
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("重ね音のずれ量", src.unisonDetuneCents, 0.0f, 120.0f, "%.1f");
    if (updateHoverHelp)
        updateHoverHelp("Unison Detune を調整します。", "重ね音のピッチ差が変わります。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("ユニゾン広がり", src.unisonSpread, 0.0f, 1.0f, "%.2f");
    if (updateHoverHelp)
        updateHoverHelp("Unison Spread を調整します。", "ステレオの広がりが変わります。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("サブオシレータ音量", src.subOscLevel, 0.0f, 2.0f, "%.2f");
    if (updateHoverHelp)
        updateHoverHelp("Sub Osc Level を調整します。", "低域補助成分の音量が変わります。", nullptr);

    std::string hardSyncLabel = std::string("強制同期（硬い音）##") + hardSyncTag;
    localChanged |= ImGui::Checkbox(hardSyncLabel.c_str(), &src.hardSyncEnabled);
    if (updateHoverHelp)
        updateHoverHelp("Hard Sync を切り替えます。",
                        "倍音の金属感が変わります。クラシックなジッパーサウンドを作れます。", nullptr);
    if (src.hardSyncEnabled)
    {
        std::string syncRatioLabel = std::string("Sync Ratio##") + hardSyncTag;
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam(syncRatioLabel.c_str(), src.hardSyncRatio, 0.5f, 8.0f, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Sync Ratio を調整します。",
                            "スレーブ周波数の倍率が変わります。高いほど高次倍音が強調されます。", nullptr);
    }
    localChanged |= ImGui::Checkbox("金属音ミックス", &src.ringModEnabled);
    if (updateHoverHelp)
        updateHoverHelp("Ring Mod を切り替えます。", "2つの音が干渉して金属的・ベル的な響きになります。", nullptr);
    if (src.ringModEnabled)
    {
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Ring Ratio", src.ringModRatio, 0.125f, 16.0f, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Ring Ratio を調整します。",
                            "変調オシレーターの周波数比率が変わります。整数比でベル的、非整数比で金属的になります。",
                            nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Ring Mix", src.ringModMix, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Ring Mix を調整します。", "原音とリングモジュレーション音のブレンド量が変わります。",
                            nullptr);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("アルペジオ");
    std::string arpEnabledLabel = std::string("Enabled##") + arpeggioTag;
    std::string arpRateLabel = std::string("Rate Hz##") + arpeggioTag;
    std::string arpStepsLabel = std::string("Steps##") + arpeggioTag;
    localChanged |= ImGui::Checkbox(arpEnabledLabel.c_str(), &src.arpeggio.enabled);
    if (updateHoverHelp)
        updateHoverHelp("Arpeggio を切り替えます。", "オンにするとステップ順でノートを繰り返し発音します。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam(arpRateLabel.c_str(), src.arpeggio.rateHz, 0.5f, 40.0f, "%.2f");
    if (updateHoverHelp)
        updateHoverHelp("Arpeggio Rate を調整します。", "音が繰り返す速さが変わります。高いほど細かく刻みます。",
                        nullptr);
    int arpSteps = src.arpeggio.steps;
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderInt(arpStepsLabel.c_str(), &arpSteps, 1, 8))
    {
        src.arpeggio.steps = arpSteps;
        localChanged = true;
    }
    if (updateHoverHelp)
        updateHoverHelp("Arpeggio Steps を調整します。",
                        "繰り返すノート数 (1〜8) が変わります。Note 1〜N を順番に再生します。", nullptr);
    for (int k = 0; k < src.arpeggio.steps; k++)
    {
        char label[64];
        snprintf(label, sizeof(label), "Note %d##%s", k + 1, arpeggioTag);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= ImGui::SliderInt(label, &src.arpeggio.semitones[static_cast<size_t>(k)], -24, 24);
        if (updateHoverHelp)
            updateHoverHelp("Arpeggio Note を調整します。",
                            "基音からの半音オフセットが変わります。0=ユニゾン、12=1オクターブ上。", nullptr);
    }

    const char *filterModes[] = {"bypass", "lowpass", "highpass", "bandpass", "ladderLowpass", "vocal"};
    int filterModeIdx = 0;
    switch (src.filterMode)
    {
    case FilterMode::Bypass:
        filterModeIdx = 0;
        break;
    case FilterMode::LowPass:
        filterModeIdx = 1;
        break;
    case FilterMode::HighPass:
        filterModeIdx = 2;
        break;
    case FilterMode::BandPass:
        filterModeIdx = 3;
        break;
    case FilterMode::LadderLowPass:
        filterModeIdx = 4;
        break;
    case FilterMode::Vocal:
        filterModeIdx = 5;
        break;
    }
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::Combo("フィルタモード", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
    {
        switch (filterModeIdx)
        {
        case 0:
            src.filterMode = FilterMode::Bypass;
            break;
        case 1:
            src.filterMode = FilterMode::LowPass;
            break;
        case 2:
            src.filterMode = FilterMode::HighPass;
            break;
        case 3:
            src.filterMode = FilterMode::BandPass;
            break;
        case 4:
            src.filterMode = FilterMode::LadderLowPass;
            break;
        case 5:
            src.filterMode = FilterMode::Vocal;
            break;
        default:
            src.filterMode = FilterMode::Bypass;
            break;
        }
        localChanged = true;
    }
    if (updateHoverHelp)
        updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
    if (src.filterMode != FilterMode::Bypass)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
    }
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("フィルタカットオフ (Hz)", src.filterCutoffHz, 10.0f, 20000.0f, "%.1f");
    if (updateHoverHelp)
        updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("フィルタ強調", src.filterResonance, 0.1f, 18.0f, "%.2f");
    if (updateHoverHelp)
    {
        updateHoverHelp("Filter Resonance を調整します。",
                        "カットオフ付近の強調量が変わります。Layer2「荒さ」スライダーの書き込み範囲は 0.5〜6.0 です。",
                        nullptr);
    }
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("フィルタキー追従", src.filterKeytrack, 0.0f, 1.0f, "%.2f");
    if (updateHoverHelp)
        updateHoverHelp("Filter Keytrack を調整します。",
                        "ノート音程に連動してカットオフが動く量が変わります。基準は C4(60)。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("フィルタ入力ドライブ", src.filterDrive, 0.0f, 1.0f, "%.2f");
    if (updateHoverHelp)
        updateHoverHelp("Filter Drive を調整します。", "ladderLowpass の入力段で太さと潰れが増えます。", nullptr);
    return localChanged;
}

template <class Source>
bool drawWaveformLikeSmoothingAndModulation(Source &src, const char *modulationId, const HoverHelpFn &updateHoverHelp)
{
    bool localChanged = false;
    src.smoothing.ampTimeMs = std::clamp(src.smoothing.ampTimeMs, 0.0, 1000.0);
    src.smoothing.pitchTimeMs = std::clamp(src.smoothing.pitchTimeMs, 0.0, 1000.0);
    src.smoothing.filterCutoffTimeMs = std::clamp(src.smoothing.filterCutoffTimeMs, 0.0, 1000.0);
    src.modulation.lfo1.rateHz = std::clamp(src.modulation.lfo1.rateHz, 0.0, 100.0);
    src.modulation.lfo1.depth = std::clamp(src.modulation.lfo1.depth, 0.0, 1.0);
    src.modulation.lfo1.delayMs = std::clamp(src.modulation.lfo1.delayMs, 0.0, 2000.0);
    src.modulation.lfo1.fadeMs = std::clamp(src.modulation.lfo1.fadeMs, 0.0, 2000.0);
    src.modulation.env2.attackSec = std::clamp(src.modulation.env2.attackSec, 0.0, 10.0);
    src.modulation.env2.decaySec = std::clamp(src.modulation.env2.decaySec, 0.0, 10.0);
    src.modulation.env2.sustainLevel = std::clamp(src.modulation.env2.sustainLevel, 0.0, 1.0);
    src.modulation.env2.releaseSec = std::clamp(src.modulation.env2.releaseSec, 0.0, 10.0);
    src.modulation.env2.curve = std::clamp(src.modulation.env2.curve, 0.0, 1.0);
    for (auto &route : src.modulation.matrix.routes)
    {
        route.amount = std::clamp(route.amount, -1.0, 1.0);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("スムージング");
    localChanged |= ImGui::Checkbox("スムージング有効", &src.smoothing.enabled);
    if (updateHoverHelp)
        updateHoverHelp("Smoothing Enabled を切り替えます。", "パラメータ変化の段差を抑えます。", nullptr);
    localChanged |= ImGui::Checkbox("ピッチスムージング有効", &src.smoothing.pitchEnabled);
    if (updateHoverHelp)
        updateHoverHelp("Pitch Smoothing Enabled を切り替えます。", "ピッチ変化の滑らかさが変わります。", nullptr);
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("音量スムージング (ms)", src.smoothing.ampTimeMs, 0.0f, 1000.0f, "%.1f");
    ImGui::SetNextItemWidth(220.0f);
    localChanged |= sliderWaveParam("ピッチスムージング (ms)", src.smoothing.pitchTimeMs, 0.0f, 1000.0f, "%.1f");
    ImGui::SetNextItemWidth(220.0f);
    localChanged |=
        sliderWaveParam("フィルタスムージング (ms)", src.smoothing.filterCutoffTimeMs, 0.0f, 1000.0f, "%.1f");
    localChanged |= drawModulationEditor(modulationId, src.modulation, true, false, true, updateHoverHelp);
    return localChanged;
}

} // namespace

bool DrawSourceEditor(InstrumentSoundConfig &chCfg, int &selectedDrumNote, const HoverHelpFn &updateHoverHelp)
{
    bool changed = false;
    if (auto *wf = std::get_if<WaveformConfig>(&chCfg.source))
    {
        changed |= drawWaveformLikeCoreEditor(*wf, "wave", "wave_arp", updateHoverHelp);
        changed |= drawWaveformLikeSmoothingAndModulation(*wf, "waveform_modulation", updateHoverHelp);
    }
    else if (auto *analog = std::get_if<AnalogConfig>(&chCfg.source))
    {
        changed |= drawWaveformLikeCoreEditor(*analog, "analog", "analog_arp", updateHoverHelp);

        analog->drive = std::clamp(analog->drive, 0.0, 1.0);
        analog->driftDepthCents = std::clamp(analog->driftDepthCents, 0.0, 20.0);
        analog->driftRateHz = std::clamp(analog->driftRateHz, 0.01, 2.0);

        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Drive", analog->drive, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp)
            updateHoverHelp("Drive を調整します。", "ソフトクリップ量が変わります。",
                            "上げすぎると飽和が強くなります。");

        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Drift Depth (cent)", analog->driftDepthCents, 0.0f, 20.0f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Drift Depth を調整します。", "ピッチ揺らぎ量が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        float driftRateHz = static_cast<float>(analog->driftRateHz);
        if (ImGui::SliderFloat("Drift Rate (Hz)", &driftRateHz, 0.01f, 2.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            analog->driftRateHz = static_cast<double>(driftRateHz);
            changed = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Drift Rate を調整します。", "ドリフトLFOの速度が変わります。", nullptr);

        changed |= drawWaveformLikeSmoothingAndModulation(*analog, "analog_modulation", updateHoverHelp);
    }
    else if (auto *nz = std::get_if<NoiseConfig>(&chCfg.source))
    {
        nz->filterCutoffHz = std::clamp(nz->filterCutoffHz, 10.0, 20000.0);
        nz->filterResonance = std::clamp(nz->filterResonance, 0.1, 18.0);
        nz->filterDrive = std::clamp(nz->filterDrive, 0.0, 1.0);

        int idx = NoiseToIndex(nz->noise);
        const char *noises[] = {"white", "pink", "brown", "blue"};
        changed |= ImGui::Combo("Noise", &idx, noises, IM_ARRAYSIZE(noises));
        if (updateHoverHelp)
            updateHoverHelp("Noise を選択します。", "ノイズ種別（色）が変わります。", nullptr);
        nz->noise = NoiseFromIndex(idx);

        const char *filterModes[] = {"bypass", "lowpass", "highpass", "bandpass", "ladderLowpass", "vocal"};
        int filterModeIdx = 0;
        switch (nz->filterMode)
        {
        case FilterMode::Bypass:
            filterModeIdx = 0;
            break;
        case FilterMode::LowPass:
            filterModeIdx = 1;
            break;
        case FilterMode::HighPass:
            filterModeIdx = 2;
            break;
        case FilterMode::BandPass:
            filterModeIdx = 3;
            break;
        case FilterMode::LadderLowPass:
            filterModeIdx = 4;
            break;
        case FilterMode::Vocal:
            filterModeIdx = 5;
            break;
        }
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("Filter Mode", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
        {
            switch (filterModeIdx)
            {
            case 0:
                nz->filterMode = FilterMode::Bypass;
                break;
            case 1:
                nz->filterMode = FilterMode::LowPass;
                break;
            case 2:
                nz->filterMode = FilterMode::HighPass;
                break;
            case 3:
                nz->filterMode = FilterMode::BandPass;
                break;
            case 4:
                nz->filterMode = FilterMode::LadderLowPass;
                break;
            case 5:
                nz->filterMode = FilterMode::Vocal;
                break;
            default:
                nz->filterMode = FilterMode::Bypass;
                break;
            }
            changed = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
        if (nz->filterMode != FilterMode::Bypass)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
        }
        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Filter Cutoff (Hz)", nz->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
        if (updateHoverHelp)
            updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Filter Resonance (Q)", nz->filterResonance, 0.1f, 18.0f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Filter Resonance を調整します。", "カットオフ付近の強調量が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Filter Drive", nz->filterDrive, 0.0f, 1.0f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Filter Drive を調整します。", "ladderLowpass の入力段で太さと潰れが増えます。", nullptr);
    }
    else if (auto *fm = std::get_if<FmConfig>(&chCfg.source))
    {
        fm->algorithm = std::clamp(fm->algorithm, 0, 7);
        fm->feedback = std::clamp(fm->feedback, 0.0, 1.0);
        for (auto &op : fm->ops)
        {
            op.ratio = std::clamp(op.ratio, 0.0, 32.0);
            op.level = std::clamp(op.level, 0.0, 1.0);
            op.index = std::clamp(op.index, 0.0, 32.0);
        }
        fm->filterCutoffHz = std::clamp(fm->filterCutoffHz, 10.0, 20000.0);
        fm->filterResonance = std::clamp(fm->filterResonance, 0.1, 18.0);
        fm->modulation.lfo1.rateHz = std::clamp(fm->modulation.lfo1.rateHz, 0.0, 100.0);
        fm->modulation.lfo1.depth = std::clamp(fm->modulation.lfo1.depth, 0.0, 1.0);
        fm->modulation.lfo1.delayMs = std::clamp(fm->modulation.lfo1.delayMs, 0.0, 2000.0);
        fm->modulation.lfo1.fadeMs = std::clamp(fm->modulation.lfo1.fadeMs, 0.0, 2000.0);
        fm->modulation.env2.attackSec = std::clamp(fm->modulation.env2.attackSec, 0.0, 10.0);
        fm->modulation.env2.decaySec = std::clamp(fm->modulation.env2.decaySec, 0.0, 10.0);
        fm->modulation.env2.sustainLevel = std::clamp(fm->modulation.env2.sustainLevel, 0.0, 1.0);
        fm->modulation.env2.releaseSec = std::clamp(fm->modulation.env2.releaseSec, 0.0, 10.0);
        fm->modulation.env2.curve = std::clamp(fm->modulation.env2.curve, 0.0, 1.0);
        for (auto &route : fm->modulation.matrix.routes)
        {
            route.amount = std::clamp(route.amount, -1.0, 1.0);
        }

        const char *algoLabels[] = {"0: M->C  (classic pair)",       "1: [M->C]+[M->C]  (2-pair)",
                                    "2: M->[C+C+C]  (1mod 3car)",    "3: M->M->M->C  (chain)",
                                    "4: [M->C]+[M->C]  (dual pair)", "5: M->[C+C+C]  (triple car)",
                                    "6: [M->C]+C+C  (hybrid)",       "7: C+C+C+C  (all car)"};
        changed |= ImGui::Combo("FM Algorithm", &fm->algorithm, algoLabels, IM_ARRAYSIZE(algoLabels));
        if (updateHoverHelp)
            updateHoverHelp("FM アルゴリズムを選択します。", "オペレータの接続構造が変わります。", nullptr);
        ImGui::SameLine();
        if (ImGui::Button("テンプレートに戻す"))
        {
            ApplyFmTemplateByAlgorithm(*fm, fm->algorithm);
            changed = true;
        }
        if (updateHoverHelp)
        {
            updateHoverHelp("現在アルゴリズムの推奨テンプレートへ戻します。",
                            "FMオペレータ/フィルタ/変調の初期値を安全域へ復帰します。",
                            "現在の微調整値は上書きされます。");
        }

        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Feedback", fm->feedback, 0.0f, 1.0f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Op1 の自己フィードバック量です。", "大きくするとサチュレーション気味になります。",
                            nullptr);

        const char *waves[] = {"sine", "square", "saw", "triangle"};
        const char *opHeadersAlgo0[] = {"Op 1 (Mod)", "Op 2 (Car)", "Op 3", "Op 4"};
        const char *opHeadersDefault[] = {"Op 1", "Op 2", "Op 3", "Op 4"};
        for (int opIdx = 0; opIdx < 4; opIdx++)
        {
            const char *header = (fm->algorithm == 0) ? opHeadersAlgo0[opIdx] : opHeadersDefault[opIdx];
            if (ImGui::CollapsingHeader(header))
            {
                FmOperator &op = fm->ops[static_cast<size_t>(opIdx)];

                int waveIdx = WaveToIndex(op.wave);
                const std::string waveId = "Wave##op" + std::to_string(opIdx);
                changed |= ImGui::Combo(waveId.c_str(), &waveIdx, waves, IM_ARRAYSIZE(waves));
                if (updateHoverHelp)
                    updateHoverHelp("Wave を選択します。", "このオペレータの波形が変わります。", nullptr);
                op.wave = WaveFromIndex(waveIdx);

                const std::string ratioId = "Ratio##op" + std::to_string(opIdx);
                changed |= ImGui::InputDouble(ratioId.c_str(), &op.ratio, 0.01, 0.1, "%.3f");
                if (updateHoverHelp)
                    updateHoverHelp("Ratio を調整します。", "このオペレータの周波数比が変わります。", nullptr);

                const std::string levelId = "Level##op" + std::to_string(opIdx);
                changed |= ImGui::InputDouble(levelId.c_str(), &op.level, 0.01, 0.1, "%.3f");
                if (updateHoverHelp)
                    updateHoverHelp("Level を調整します。", "このオペレータの出力レベルが変わります。", nullptr);

                const std::string indexId = "Index##op" + std::to_string(opIdx);
                changed |= ImGui::InputDouble(indexId.c_str(), &op.index, 0.01, 0.1, "%.3f");
                if (updateHoverHelp)
                    updateHoverHelp("Index を調整します。", "このオペレータの変調深さが変わります。", nullptr);
            }
        }

        const char *filterModes[] = {"bypass", "lowpass", "highpass", "bandpass", "ladderLowpass", "vocal"};
        int filterModeIdx = 0;
        switch (fm->filterMode)
        {
        case FilterMode::Bypass:
            filterModeIdx = 0;
            break;
        case FilterMode::LowPass:
            filterModeIdx = 1;
            break;
        case FilterMode::HighPass:
            filterModeIdx = 2;
            break;
        case FilterMode::BandPass:
            filterModeIdx = 3;
            break;
        case FilterMode::LadderLowPass:
            filterModeIdx = 4;
            break;
        case FilterMode::Vocal:
            filterModeIdx = 5;
            break;
        }
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("Filter Mode", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
        {
            switch (filterModeIdx)
            {
            case 0:
                fm->filterMode = FilterMode::Bypass;
                break;
            case 1:
                fm->filterMode = FilterMode::LowPass;
                break;
            case 2:
                fm->filterMode = FilterMode::HighPass;
                break;
            case 3:
                fm->filterMode = FilterMode::BandPass;
                break;
            case 4:
                fm->filterMode = FilterMode::LadderLowPass;
                break;
            case 5:
                fm->filterMode = FilterMode::Vocal;
                break;
            default:
                fm->filterMode = FilterMode::Bypass;
                break;
            }
            changed = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
        if (fm->filterMode != FilterMode::Bypass)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
        }
        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Filter Cutoff (Hz)", fm->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
        if (updateHoverHelp)
            updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Filter Resonance (Q)", fm->filterResonance, 0.1f, 18.0f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Filter Resonance を調整します。", "カットオフ付近の強調量が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        changed |= sliderWaveParam("Filter Drive", fm->filterDrive, 0.0f, 1.0f, "%.2f");
        if (updateHoverHelp)
            updateHoverHelp("Filter Drive を調整します。", "ladderLowpass の入力段で太さと潰れが増えます。", nullptr);

        changed |= drawModulationEditor("fm_modulation", fm->modulation, false, true, false, updateHoverHelp);
    }
    else if (auto *psg = std::get_if<PsgConfig>(&chCfg.source))
    {
        psg->duty = std::clamp(psg->duty, 0, 7);
        psg->volumeSteps = std::clamp(psg->volumeSteps, 0, 15);
        psg->maxVoices = std::clamp(psg->maxVoices, 1, 8);

        int psgWaveIdx = 0;
        switch (psg->wave)
        {
        case PsgWaveType::Square:
            psgWaveIdx = 0;
            break;
        case PsgWaveType::Pulse:
            psgWaveIdx = 1;
            break;
        case PsgWaveType::Triangle:
            psgWaveIdx = 2;
            break;
        case PsgWaveType::Noise:
            psgWaveIdx = 3;
            break;
        }
        const char *psgWaves[] = {"Square", "Pulse", "Triangle", "Noise"};
        if (ImGui::Combo("Wave", &psgWaveIdx, psgWaves, IM_ARRAYSIZE(psgWaves)))
        {
            switch (psgWaveIdx)
            {
            case 0:
                psg->wave = PsgWaveType::Square;
                break;
            case 1:
                psg->wave = PsgWaveType::Pulse;
                break;
            case 2:
                psg->wave = PsgWaveType::Triangle;
                break;
            case 3:
                psg->wave = PsgWaveType::Noise;
                break;
            default:
                psg->wave = PsgWaveType::Square;
                break;
            }
            changed = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("PSG Wave を選択します。", "PSG波形（Square/Pulse/Triangle/Noise）が切り替わります。",
                            nullptr);

        if (psg->wave == PsgWaveType::Pulse)
        {
            int duty = psg->duty;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Duty", &duty, 0, 7))
            {
                psg->duty = duty;
                changed = true;
            }
            if (updateHoverHelp)
                updateHoverHelp("Duty を調整します。", "Pulse波のパルス幅が変わります。", nullptr);
        }

        int volumeSteps = psg->volumeSteps;
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::SliderInt("Volume Steps", &volumeSteps, 0, 15))
        {
            psg->volumeSteps = volumeSteps;
            changed = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Volume Steps を調整します。", "PSGの離散音量ステップ数が変わります。", nullptr);

        int maxVoices = psg->maxVoices;
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::SliderInt("Max Voices", &maxVoices, 1, 8))
        {
            psg->maxVoices = maxVoices;
            changed = true;
        }
        if (updateHoverHelp)
            updateHoverHelp("Max Voices を調整します。", "PSGの同時発音上限が変わります。", nullptr);
    }
    else if (auto *kit = std::get_if<DrumKitConfig>(&chCfg.source))
    {
        if (ImGui::CollapsingHeader("Drum Bus", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrumBusConfig &bus = kit->drumBus;
            changed |= ImGui::Checkbox("Enabled##drumBus", &bus.enabled);
            if (updateHoverHelp)
            {
                updateHoverHelp("Drum Bus を有効化します。",
                                "キット全体の前後感、まとまり、刺さりをまとめて調整します。",
                                "単発の音量ではなく合算後の配置を変えます。");
            }
            changed |= ImGui::InputDouble("Level##drumBus", &bus.level, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Attack Trim##drumBus", &bus.attackTrim, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Sustain Lift##drumBus", &bus.sustainLift, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Glue##drumBus", &bus.glue, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Presence Cut##drumBus", &bus.presenceCut, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Low Tighten##drumBus", &bus.lowTighten, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Room Send##drumBus", &bus.roomSend, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Drive Trim##drumBus", &bus.driveTrim, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Velocity Ceiling##drumBus", &kit->velocityCeiling, 0.01, 0.05, "%.3f");
            changed |= ImGui::InputDouble("Velocity Curve##drumBus", &kit->velocityCurve, 0.01, 0.05, "%.3f");

            bus.level = std::clamp(bus.level, 0.0, 2.0);
            bus.attackTrim = std::clamp(bus.attackTrim, 0.0, 1.0);
            bus.sustainLift = std::clamp(bus.sustainLift, 0.0, 1.0);
            bus.glue = std::clamp(bus.glue, 0.0, 1.0);
            bus.presenceCut = std::clamp(bus.presenceCut, 0.0, 1.0);
            bus.lowTighten = std::clamp(bus.lowTighten, 0.0, 1.0);
            bus.roomSend = std::clamp(bus.roomSend, 0.0, 1.0);
            bus.driveTrim = std::clamp(bus.driveTrim, 0.0, 1.0);
            kit->velocityCeiling = std::clamp(kit->velocityCeiling, 0.0, 1.0);
            kit->velocityCurve = std::clamp(kit->velocityCurve, 0.2, 3.0);
        }

        changed |= ImGui::InputInt("DrumKit Note (0-127)", &selectedDrumNote);
        if (updateHoverHelp)
        {
            updateHoverHelp("DrumKit Note を選択します。", "編集対象ノートのドラム定義が切り替わります。", nullptr);
        }
        selectedDrumNote = std::clamp(selectedDrumNote, 0, 127);
        DrumConfig &d = kit->map[selectedDrumNote];
        changed |= DrawDrumConfigEditor("drum_kit", d, updateHoverHelp);
    }

    return changed;
}
} // namespace gui::detail
