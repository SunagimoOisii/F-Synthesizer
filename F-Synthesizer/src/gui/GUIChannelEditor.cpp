#include "gui/GUIChannelEditor.h"

#include <algorithm>
#include <array>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIMacroMapping.h"
#include "gui/GUIStateModel.h"

namespace
{
using HoverHelpFn = std::function<void(const char* what, const char* impact, const char* caution)>;

std::array<config::SourceKind, config::kSourceKindCount> BuildGuiSourceKindList(size_t& outCount)
{
    std::array<config::SourceKind, config::kSourceKindCount> kinds{};
    outCount = 0;
    for (int i = 0; i < config::kSourceKindCount; i++)
    {
        const config::SourceKind kind = config::SourceKindFromIndex(i);
        if (kind == config::SourceKind::Count)
        {
            continue;
        }

        const config::SourceCapability capability = config::SourceCapabilityOf(kind);
        // Soundタブでは one-shot 単発の Drum 直編集を避け、DrumKit を正規導線として残す。
        if (!capability.isPercussion || kind == config::SourceKind::DrumKit)
        {
            kinds[outCount++] = kind;
        }
    }
    if (outCount == 0)
    {
        kinds[outCount++] = config::SourceKind::Waveform;
    }
    return kinds;
}

void SetFmOperatorEnv(ModEnvelopeConfig& env, double attack, double decay, double sustain, double release, double curve)
{
    env.attackSec = attack;
    env.decaySec = decay;
    env.sustainLevel = sustain;
    env.releaseSec = release;
    env.curve = curve;
}

void ApplyFmTemplateByAlgorithm(FmConfig& fm, int algorithm)
{
    fm.algorithm = std::clamp(algorithm, 0, 7);
    fm.filterMode = FilterMode::Bypass;
    fm.filterCutoffHz = 8000.0;
    fm.filterResonance = 0.707;
    fm.feedback = 0.0;

    for (auto& op : fm.ops)
    {
        op.wave = WaveType::Sine;
        op.ratio = 1.0;
        op.level = 1.0;
        op.index = 0.0;
        SetFmOperatorEnv(op.levelEnv, 0.0, 0.02, 1.0, 0.05, 0.0);
        SetFmOperatorEnv(op.indexEnv, 0.0, 0.02, 1.0, 0.05, 0.0);
    }

    switch (fm.algorithm)
    {
    case 1:
        // [M->C] + [M->C]
        fm.feedback = 0.14;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.2;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.88; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.95; fm.ops[2].index = 1.6;
        fm.ops[3].ratio = 1.0; fm.ops[3].level = 0.80; fm.ops[3].index = 0.0;
        break;
    case 2:
        // M -> [C + C + C]
        fm.feedback = 0.08;
        fm.ops[0].ratio = 2.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.8;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.84; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.72; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 3.0; fm.ops[3].level = 0.66; fm.ops[3].index = 0.0;
        break;
    case 3:
        // M -> M -> M -> C
        fm.feedback = 0.22;
        fm.filterMode = FilterMode::LowPass;
        fm.filterCutoffHz = 3600.0;
        fm.filterResonance = 0.85;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 3.2;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 1.0; fm.ops[1].index = 1.8;
        fm.ops[2].ratio = 1.0; fm.ops[2].level = 0.9; fm.ops[2].index = 0.9;
        fm.ops[3].ratio = 1.0; fm.ops[3].level = 0.82; fm.ops[3].index = 0.0;
        break;
    case 4:
        // [M->C] + [M->C] (2ペア)
        fm.feedback = 0.10;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.0;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.85; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 1.0; fm.ops[2].index = 1.6;
        fm.ops[3].ratio = 1.0; fm.ops[3].level = 0.82; fm.ops[3].index = 0.0;
        break;
    case 5:
        // M -> [C + C + C]
        fm.feedback = 0.12;
        fm.ops[0].ratio = 2.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.4;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.80; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.74; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 3.0; fm.ops[3].level = 0.68; fm.ops[3].index = 0.0;
        break;
    case 6:
        // [M->C] + C + C
        fm.feedback = 0.10;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.0;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.84; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.72; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 3.0; fm.ops[3].level = 0.66; fm.ops[3].index = 0.0;
        break;
    case 7:
        // C + C + C + C
        fm.feedback = 0.08;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 0.82; fm.ops[0].index = 0.0;
        fm.ops[1].ratio = 2.0; fm.ops[1].level = 0.76; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 3.0; fm.ops[2].level = 0.70; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 4.0; fm.ops[3].level = 0.64; fm.ops[3].index = 0.0;
        break;
    case 0:
    default:
        // M -> C
        fm.feedback = 0.08;
        fm.ops[0].ratio = 2.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.4;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.88; fm.ops[1].index = 0.0;
        fm.ops[2].level = 0.0;
        fm.ops[3].level = 0.0;
        break;
    }

    for (auto& op : fm.ops)
    {
        SetFmOperatorEnv(op.levelEnv, 0.0, 0.04, 0.88, 0.08, 0.2);
        SetFmOperatorEnv(op.indexEnv, 0.0, 0.08, 0.65, 0.06, 0.45);
    }
    if (fm.algorithm == 3)
    {
        SetFmOperatorEnv(fm.ops[0].indexEnv, 0.0, 0.045, 0.35, 0.04, 0.7);
        SetFmOperatorEnv(fm.ops[1].indexEnv, 0.0, 0.07, 0.55, 0.04, 0.55);
    }

    fm.modulation = ModulationConfig{};
    fm.modulation.env2.attackSec = 0.0;
    fm.modulation.env2.decaySec = 0.12;
    fm.modulation.env2.sustainLevel = 0.0;
    fm.modulation.env2.releaseSec = 0.08;
    for (auto& route : fm.modulation.matrix.routes)
    {
        route = ModRoute{};
    }
    fm.modulation.matrix.routes[0] = ModRoute{
        ModSource::Env2,
        ModDestination::FmIndex,
        0.8,
        true
    };
}

bool DrawDrumConfigEditor(const char* IDPrefix, DrumConfig& d, const HoverHelpFn& updateHoverHelp)
{
    bool changed = false;
    int drumType = static_cast<int>(d.type);
    const char* drumTypes[] = { "none", "kick", "snare", "hat", "tom", "rim", "clap", "crash", "ride" };
    std::string key = std::string("Drum Type##") + IDPrefix;
    changed |= ImGui::Combo(key.c_str(), &drumType, drumTypes, IM_ARRAYSIZE(drumTypes));
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "Drum Type を選択します。",
            "ドラム発音モデルが切り替わります。",
            nullptr);
    }
    ImGui::TextDisabled("DrumConfig: 0 = 未指定（内部デフォルト）");
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "DrumConfig の未指定値ルールを確認します。",
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
        updateHoverHelp(
            "Drum Gain を調整します。",
            "該当ドラム音の音量が変わります。",
            "上げすぎるとクリップしやすくなります。");
    }

    if (d.type == DrumType::Kick)
    {
        key = std::string("Body Freq##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.bodyFreq, 1.0, 10.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("Body Freq を調整します。", "キックの低域の芯が変わります。", nullptr);
        key = std::string("Body Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.bodyLevel, 0.01, 0.1, "%.3f");
        key = std::string("Body Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.bodyDecaySec, 0.01, 0.1, "%.3f");
        key = std::string("Pitch Start##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchStart, 0.1, 1.0, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Pitch Start を調整します。", "キック開始時の高いピッチ量が変わります。", nullptr);
        key = std::string("Pitch Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Pitch Decay を調整します。", "キックのピッチ変化速度が変わります。", nullptr);
        key = std::string("Transient Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
        key = std::string("Transient Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        key = std::string("Drive##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
        if (updateHoverHelp) updateHoverHelp("Drive を調整します。", "ドラム専用ソフトクリップの強さが変わります。", nullptr);
    }
    else if (d.type == DrumType::Snare || d.type == DrumType::Tom || d.type == DrumType::Rim)
    {
        key = std::string("Body Freq##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.bodyFreq, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("Body Freq を調整します。", "胴鳴り/打撃トーンの周波数が変わります。", nullptr);
        key = std::string("Body Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.bodyLevel, 0.01, 0.1, "%.3f");
        key = std::string("Body Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.bodyDecaySec, 0.01, 0.1, "%.3f");
        if (d.type == DrumType::Tom)
        {
            key = std::string("Pitch Start##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchStart, 0.1, 1.0, "%.3f");
            key = std::string("Pitch Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
        }
        if (d.type == DrumType::Snare)
        {
            key = std::string("Snap Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.snapLevel, 0.01, 0.1, "%.3f");
            key = std::string("Snap Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.snapDecaySec, 0.001, 0.01, "%.3f");
        }
        key = std::string("Transient Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
        key = std::string("Transient Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        key = std::string("HP Cut##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("HP Cut を調整します。", "高域寄りに残す帯域が変わります。", nullptr);
        key = std::string("LP Cut##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("LP Cut を調整します。", "低域寄りに残す帯域が変わります。", nullptr);
        key = std::string("Drive##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
        if (updateHoverHelp) updateHoverHelp("Drive を調整します。", "ドラム専用ソフトクリップの強さが変わります。", nullptr);
    }
    else
    {
        key = std::string("Metal Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.metalLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Metal Level を調整します。", "ハットの硬い金属トーン量が変わります。", nullptr);
        key = std::string("Air Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.airLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Air Level を調整します。", "ハットの高域ノイズ量が変わります。", nullptr);
        key = std::string("Noise Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.noiseLevel, 0.01, 0.1, "%.3f");
        if (d.type == DrumType::Clap)
        {
            key = std::string("Transient Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.transientLevel, 0.01, 0.1, "%.3f");
            key = std::string("Transient Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.transientDecaySec, 0.001, 0.01, "%.3f");
        }
        key = std::string("Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.decaySec, 0.001, 0.01, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Decay を調整します。", "ハットの短さが変わります。", nullptr);
        key = std::string("HP Cut##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("HP Cut を調整します。", "高域寄りに残す帯域が変わります。", nullptr);
        key = std::string("LP Cut##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("LP Cut を調整します。", "低域寄りに残す帯域が変わります。", nullptr);
        key = std::string("Drive##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.drive, 0.01, 0.1, "%.3f");
        d.drive = std::clamp(d.drive, 0.0, 1.0);
        if (updateHoverHelp) updateHoverHelp("Drive を調整します。", "ドラム専用ソフトクリップの強さが変わります。", nullptr);
    }

    key = std::string("Velocity -> Tone##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.velocityToTone, 0.01, 0.1, "%.3f");
    key = std::string("Velocity -> Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.velocityToDecay, 0.01, 0.1, "%.3f");
    key = std::string("Humanize Pitch Cents##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.humanizePitchCents, 0.1, 1.0, "%.2f");
    key = std::string("Humanize Decay Pct##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.humanizeDecayPct, 0.01, 0.1, "%.3f");
    d.velocityToTone = std::clamp(d.velocityToTone, 0.0, 1.0);
    d.velocityToDecay = std::clamp(d.velocityToDecay, -1.0, 1.0);
    d.humanizePitchCents = std::clamp(d.humanizePitchCents, 0.0, 50.0);
    d.humanizeDecayPct = std::clamp(d.humanizeDecayPct, 0.0, 1.0);

    if (d.type == DrumType::Snare || d.type == DrumType::Hat || d.type == DrumType::Clap || d.type == DrumType::Crash || d.type == DrumType::Ride)
    {
        int noiseColor = d.noiseColor;
        const char* noises[] = { "white", "pink", "brown", "blue" };
        key = std::string("Noise Color##") + IDPrefix;
        changed |= ImGui::Combo(key.c_str(), &noiseColor, noises, IM_ARRAYSIZE(noises));
        if (updateHoverHelp) updateHoverHelp("Noise Color を選択します。", "スナップ/エア成分の周波数傾向が変わります。", nullptr);
        d.noiseColor = noiseColor;
    }
    return changed;
}

#include "channeleditor/EnvelopeView.inl"
#include "channeleditor/FmAlgorithmDiagram.inl"
} // namespace

namespace gui
{
bool DrawChannelEditor(
    GUIState& state,
    bool showSourceTypeSelector,
    const std::function<void(const char* what, const char* impact, const char* caution)>& updateHoverHelp)
{
    static ChannelConfig l3BeforeConfig{};
    static MacroSliderState l3BeforeSliders{};
    static int l3BeforeSlot = -1;
    static bool l3SessionChanged = false;

    bool changed = false;
    EnsureChannelConfigs(state);
    state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);
    // Layer3 Undo bracketing: IsAnyItemActive() の遷移を利用して before スナップショットを記録する。
    // 精度注記: IsAnyItemActive() はウィンドウ全体のフラグのため Layer2 の操作で誤アーム
    // することがあるが、Layer2 は独自の per-slider undo を持つため実用上の問題はない。
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    const ChannelConfig frameConfig = (*state.channelConfigs)[slot];
    const MacroSliderState frameSliders = state.macroSliders[slot];

    ImGui::TextUnformatted("音色スロット");
    changed |= ImGui::InputInt("選択スロット (0-15)", &state.selectedSoundSlot);
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "編集対象のSound Slotを指定します。",
            "この後の音色編集が適用されるスロットが変わります。",
            nullptr);
    }
    state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);
    const int prChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
    const int assignedSlot = std::clamp(state.channelAssignments[prChannel], 0, 15);
    ImGui::TextDisabled("表示ch%d -> 割当スロット s%d", prChannel, assignedSlot);
    ImGui::SameLine();
    if (ImGui::Button("この割当スロットを編集"))
    {
        state.selectedSoundSlot = assignedSlot;
    }
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "PR ch に割り当てられたSound Slotへ編集対象を移します。",
            "Music側の表示chで鳴る音色を直接編集できます。",
            nullptr);
    }

    auto sliderWaveParam = [&](const char* label, double& value, float minV, float maxV, const char* fmt = "%.3f") -> bool
    {
        float v = static_cast<float>(value);
        bool edited = ImGui::SliderFloat(label, &v, minV, maxV, fmt);
        if (edited)
        {
            value = static_cast<double>(v);
        }
        return edited;
    };
#include "channeleditor/ChannelEditorModulation.inl"
    ImGui::TextDisabled("Soundタブは音色定義のみ編集します（ミックス/割当はMusicタブ）。");

    ImGui::Separator();
    ChannelConfig& chCfg = (*state.channelConfigs)[state.selectedSoundSlot];
    ImGui::Text("選択スロット: s%d", state.selectedSoundSlot);
    ImGui::TextDisabled("Tone Preview は選択中スロットを使用します。");

    if (ImGui::CollapsingHeader("エンベロープ / 音量", ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::InputDouble("Amp", &chCfg.amp, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Amp を調整します。", "音量スケールが変わります。", "上げすぎるとクリップしやすくなります。");
        changed |= ImGui::InputDouble("Attack", &chCfg.attackSec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Attack を調整します。", "立ち上がり時間が変わります。", nullptr);
        changed |= ImGui::InputDouble("Decay", &chCfg.decaySec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Decay を調整します。", "サステインまでの減衰時間が変わります。", nullptr);
        changed |= ImGui::InputDouble("Sustain", &chCfg.sustainLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Sustain を調整します。", "押下維持中の音量が変わります。", nullptr);
        changed |= ImGui::InputDouble("Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Release を調整します。", "ノートオフ後の余韻時間が変わります。", nullptr);
        ImGui::TextDisabled("Envelope");
        float attackDrag = static_cast<float>(chCfg.attackSec);
        float decayDrag = static_cast<float>(chCfg.decaySec);
        float sustainDrag = static_cast<float>(chCfg.sustainLevel);
        float releaseDrag = static_cast<float>(chCfg.releaseSec);
        const bool adsrDragged = DrawADSRPreview(
            "##adsr_main",
            attackDrag,
            decayDrag,
            sustainDrag,
            releaseDrag,
            0.0f,
            &attackDrag,
            &decayDrag,
            &sustainDrag,
            &releaseDrag);
        if (adsrDragged)
        {
            chCfg.attackSec = std::max(0.0, static_cast<double>(attackDrag));
            chCfg.decaySec = std::max(0.0, static_cast<double>(decayDrag));
            chCfg.sustainLevel = std::clamp(static_cast<double>(sustainDrag), 0.0, 1.0);
            chCfg.releaseSec = std::max(0.0, static_cast<double>(releaseDrag));
            changed = true;
        }
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "ADSR グラフの点をドラッグして直接編集します。",
                "Attack/Decay/Sustain/Release を形で調整できます。",
                "ピーク点・減衰終点・リリース開始点をドラッグできます。");
        }
    }

    if (ImGui::CollapsingHeader("Attack Layer"))
    {
        auto& layer = chCfg.attackLayer;
        changed |= ImGui::Checkbox("Enabled##attackLayer", &layer.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "NoteOn直後だけ鳴る短い補助音を重ねます。",
                "ピック感、ブラスの吹き始め、金属的な打撃を外部PCMなしで足します。",
                "上げすぎるとピークやチープさが出ます。");
        }

        int attackType = static_cast<int>(layer.type);
        const char* attackTypes[] = { "pick", "brass", "metal" };
        changed |= ImGui::Combo("Type##attackLayer", &attackType, attackTypes, IM_ARRAYSIZE(attackTypes));
        attackType = std::clamp(attackType, 0, 2);
        layer.type = static_cast<AttackLayerType>(attackType);

        changed |= ImGui::InputDouble("Level##attackLayer", &layer.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Decay##attackLayer", &layer.decaySec, 0.001, 0.01, "%.3f");
        changed |= ImGui::InputDouble("Brightness##attackLayer", &layer.brightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Body Mix##attackLayer", &layer.bodyMix, 0.01, 0.05, "%.3f");

        layer.level = std::clamp(layer.level, 0.0, 1.0);
        layer.decaySec = std::clamp(layer.decaySec, 0.001, 0.25);
        layer.brightness = std::clamp(layer.brightness, 0.0, 1.0);
        layer.bodyMix = std::clamp(layer.bodyMix, 0.0, 1.0);
        layer.pitchOffsetSemis = std::clamp(layer.pitchOffsetSemis, -24.0, 24.0);
        layer.drive = std::clamp(layer.drive, 0.0, 1.0);
    }

    if (ImGui::CollapsingHeader("Bass Layer"))
    {
        auto& layer = chCfg.bassLayer;
        changed |= ImGui::Checkbox("Enabled##bassLayer", &layer.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "ベース向けに持続する低域と歪み成分を重ねます。",
                "FM/analog/waveformの本体にサブ、胴、荒い倍音を足して押し出しを作ります。",
                "上げすぎると低域過多やピーク過多になります。");
        }

        int bassType = static_cast<int>(layer.type);
        const char* bassTypes[] = { "sub", "drive", "grit" };
        changed |= ImGui::Combo("Type##bassLayer", &bassType, bassTypes, IM_ARRAYSIZE(bassTypes));
        bassType = std::clamp(bassType, 0, 2);
        layer.type = static_cast<BassLayerType>(bassType);

        changed |= ImGui::InputDouble("Level##bassLayer", &layer.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Sub Level##bassLayer", &layer.subLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Body Level##bassLayer", &layer.bodyLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Grit Level##bassLayer", &layer.gritLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##bassLayer", &layer.drive, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Cutoff Hz##bassLayer", &layer.cutoffHz, 10.0, 100.0, "%.1f");

        layer.level = std::clamp(layer.level, 0.0, 1.0);
        layer.subLevel = std::clamp(layer.subLevel, 0.0, 1.0);
        layer.bodyLevel = std::clamp(layer.bodyLevel, 0.0, 1.0);
        layer.gritLevel = std::clamp(layer.gritLevel, 0.0, 1.0);
        layer.drive = std::clamp(layer.drive, 0.0, 1.0);
        layer.cutoffHz = std::clamp(layer.cutoffHz, 40.0, 8000.0);
        layer.pitchOffsetSemis = std::clamp(layer.pitchOffsetSemis, -24.0, 24.0);
        layer.velocityToDrive = std::clamp(layer.velocityToDrive, 0.0, 1.0);
    }

    bool layer3Changed = false;
    if (ImGui::CollapsingHeader("音源詳細", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const bool changedBeforeSourceDetails = changed;
        if (showSourceTypeSelector)
        {
            const config::SourceKind selectedKind = config::SourceConfigKind(chCfg.source);
            size_t guiSourceKindCount = 0;
            const auto guiSourceKinds = BuildGuiSourceKindList(guiSourceKindCount);
            int srcType = 0;
            for (size_t i = 0; i < guiSourceKindCount; i++)
            {
                if (guiSourceKinds[i] == selectedKind)
                {
                    srcType = static_cast<int>(i);
                    break;
                }
            }
            if (ImGui::BeginCombo("音源タイプ", config::SourceKindToDisplayName(guiSourceKinds[static_cast<size_t>(srcType)])))
            {
                for (size_t i = 0; i < guiSourceKindCount; i++)
                {
                    const config::SourceKind candidate = guiSourceKinds[i];
                    const bool selected = (srcType == static_cast<int>(i));
                    if (ImGui::Selectable(config::SourceKindToDisplayName(candidate), selected))
                    {
                        srcType = static_cast<int>(i);
                        changed = true;
                        chCfg.source = DefaultSourceByType(config::SourceKindToIndex(candidate));
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "音源タイプを切り替えます。",
                    "Waveform/Analog/Noise/FM/DrumKit/PSG の編集対象に切り替わります。",
                    "切替時は該当タイプの既定設定で初期化されます。");
            }
        }

#include "channeleditor/ChannelEditorCommon.inl"

#include "channeleditor/ChannelEditorWaveform.inl"
#include "channeleditor/ChannelEditorNoise.inl"
#include "channeleditor/ChannelEditorFm.inl"
#include "channeleditor/ChannelEditorDrum.inl"
        layer3Changed = (changed && !changedBeforeSourceDetails);
    }
    if (layer3Changed)
    {
        // Layer2 マクロスライダーを Layer3 編集に追従させる。
        const int ch = std::clamp(state.selectedSoundSlot, 0, 15);
        state.macroSliders[ch] = ReadMacroSliders((*state.channelConfigs)[ch], state.macroSliders[ch]);
    }

    const bool anyItemActive = ImGui::IsAnyItemActive();

    // スロット切替でアーム状態をリセット
    if (l3BeforeSlot >= 0 && l3BeforeSlot != slot)
    {
        l3BeforeSlot = -1;
        l3SessionChanged = false;
    }

    // アクティブ開始: before スナップショットをキャプチャ
    if (anyItemActive && l3BeforeSlot < 0)
    {
        l3BeforeConfig = frameConfig; // このフレームのレンダリング前の値
        l3BeforeSliders = frameSliders;
        l3BeforeSlot = slot;
        l3SessionChanged = false;
    }

    if (changed)
    {
        l3SessionChanged = true;
    }

    // アクティブ終了: 変更があればスタックに積む
    if (!anyItemActive && l3BeforeSlot >= 0)
    {
        if (l3SessionChanged)
        {
            PushSoundHistoryEntry(state, l3BeforeSlot, l3BeforeConfig, l3BeforeSliders);
        }
        l3BeforeSlot = -1;
        l3SessionChanged = false;
    }
    return changed;
}
} // namespace gui
