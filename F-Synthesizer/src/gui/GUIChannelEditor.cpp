#include "gui/GUIChannelEditor.h"
#include "channeleditor/SourceEditor.h"
#include "channeleditor/EnvelopeView.h"

#include <algorithm>
#include <array>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"

namespace
{
using HoverHelpFn = std::function<void(const char* what, const char* impact, const char* caution)>;

std::string ChannelLabel(int channel)
{
    channel = std::clamp(channel, 0, 15);
    std::string label = "ch" + std::to_string(channel + 1);
    if (channel == 9)
    {
        label += " Drums";
    }
    return label;
}

} // namespace

namespace gui
{
using detail::DrawADSRPreview;
bool DrawChannelEditor(
    InstrumentSoundConfig& chCfg,
    int channel,
    int& selectedDrumNote,
    bool showSourceTypeSelector,
    const std::function<void(const char* what, const char* impact, const char* caution)>& updateHoverHelp)
{
    bool changed = false;
    ImGui::Text("編集対象: %sの音色", ChannelLabel(channel).c_str());

    ImGui::Separator();
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
        changed |= ImGui::InputDouble("Focus Level##bassLayer", &layer.focusLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##bassLayer", &layer.drive, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Cutoff Hz##bassLayer", &layer.cutoffHz, 10.0, 100.0, "%.1f");

        layer.level = std::clamp(layer.level, 0.0, 1.0);
        layer.subLevel = std::clamp(layer.subLevel, 0.0, 1.0);
        layer.bodyLevel = std::clamp(layer.bodyLevel, 0.0, 1.0);
        layer.gritLevel = std::clamp(layer.gritLevel, 0.0, 1.0);
        layer.focusLevel = std::clamp(layer.focusLevel, 0.0, 1.0);
        layer.drive = std::clamp(layer.drive, 0.0, 1.0);
        layer.cutoffHz = std::clamp(layer.cutoffHz, 40.0, 8000.0);
        layer.pitchOffsetSemis = std::clamp(layer.pitchOffsetSemis, -24.0, 24.0);
        layer.velocityToDrive = std::clamp(layer.velocityToDrive, 0.0, 1.0);
        layer.focusHz = std::clamp(layer.focusHz, 60.0, 1200.0);
        layer.bodySaturation = std::clamp(layer.bodySaturation, 0.0, 1.0);
        layer.gritTone = std::clamp(layer.gritTone, 0.0, 1.0);
        layer.attackBoost = std::clamp(layer.attackBoost, 0.0, 1.0);
        layer.attackDecaySec = std::clamp(layer.attackDecaySec, 0.005, 0.25);
    }

    if (ImGui::CollapsingHeader("Lead Layer"))
    {
        auto& layer = chCfg.leadLayer;
        changed |= ImGui::Checkbox("Enabled##leadLayer", &layer.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "主旋律向けに硬いアタックと薄い厚みを重ねます。",
                "FM/analog/waveformのリードに金属的なエッジ、軽い二重化、短いしゃくりを足します。",
                "上げすぎると濁りやピーク過多が出ます。");
        }

        int leadType = static_cast<int>(layer.type);
        const char* leadTypes[] = { "blade", "brass", "edge" };
        changed |= ImGui::Combo("Type##leadLayer", &leadType, leadTypes, IM_ARRAYSIZE(leadTypes));
        leadType = std::clamp(leadType, 0, 2);
        layer.type = static_cast<LeadLayerType>(leadType);

        changed |= ImGui::InputDouble("Level##leadLayer", &layer.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Edge Level##leadLayer", &layer.edgeLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Body Level##leadLayer", &layer.bodyLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Detune Cents##leadLayer", &layer.detuneCents, 0.5, 2.0, "%.2f");
        changed |= ImGui::InputDouble("Attack Boost##leadLayer", &layer.attackBoost, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##leadLayer", &layer.drive, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Character Level##leadLayer", &layer.characterLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Character Tone##leadLayer", &layer.characterTone, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Bite Level##leadLayer", &layer.biteLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Wobble Depth Cents##leadLayer", &layer.wobbleDepthCents, 0.5, 2.0, "%.2f");

        layer.level = std::clamp(layer.level, 0.0, 1.0);
        layer.edgeLevel = std::clamp(layer.edgeLevel, 0.0, 1.0);
        layer.bodyLevel = std::clamp(layer.bodyLevel, 0.0, 1.0);
        layer.detuneCents = std::clamp(layer.detuneCents, -50.0, 50.0);
        layer.pitchBendSemis = std::clamp(layer.pitchBendSemis, -12.0, 12.0);
        layer.bendDecaySec = std::clamp(layer.bendDecaySec, 0.005, 0.25);
        layer.attackBoost = std::clamp(layer.attackBoost, 0.0, 1.0);
        layer.attackDecaySec = std::clamp(layer.attackDecaySec, 0.005, 0.25);
        layer.drive = std::clamp(layer.drive, 0.0, 1.0);
        layer.characterLevel = std::clamp(layer.characterLevel, 0.0, 1.0);
        layer.characterTone = std::clamp(layer.characterTone, 0.0, 1.0);
        layer.biteLevel = std::clamp(layer.biteLevel, 0.0, 1.0);
        layer.biteDecaySec = std::clamp(layer.biteDecaySec, 0.005, 0.25);
        layer.wobbleDepthCents = std::clamp(layer.wobbleDepthCents, 0.0, 30.0);
        layer.wobbleRateHz = std::clamp(layer.wobbleRateHz, 0.0, 12.0);
    }

    if (ImGui::CollapsingHeader("Chord Layer"))
    {
        auto& layer = chCfg.chordLayer;
        changed |= ImGui::Checkbox("Enabled##chordLayer", &layer.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "入力ノートに固定voicingの薄い追加音を重ねます。",
                "FM/analog/waveformの和音やブラスに厚みを足します。",
                "強くしすぎると和音が濁り、メロディやベースを覆います。");
        }

        changed |= ImGui::InputDouble("Level##chordLayer", &layer.level, 0.01, 0.05, "%.3f");
        for (size_t v = 0; v < layer.intervalsSemis.size(); v++)
        {
            int interval = layer.intervalsSemis[v];
            std::string label = "Interval " + std::to_string(v + 1) + "##chordLayer";
            if (ImGui::InputInt(label.c_str(), &interval, 1, 12))
            {
                layer.intervalsSemis[v] = std::clamp(interval, -24, 24);
                changed = true;
            }
        }
        changed |= ImGui::InputDouble("Detune Cents##chordLayer", &layer.detuneCents, 0.5, 2.0, "%.2f");
        changed |= ImGui::InputDouble("Spread##chordLayer", &layer.spread, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Cutoff Hz##chordLayer", &layer.cutoffHz, 10.0, 100.0, "%.1f");
        changed |= ImGui::InputDouble("Drive##chordLayer", &layer.drive, 0.01, 0.05, "%.3f");

        layer.level = std::clamp(layer.level, 0.0, 1.0);
        for (double& voiceLevel : layer.voiceLevels)
        {
            voiceLevel = std::clamp(voiceLevel, 0.0, 1.0);
        }
        layer.detuneCents = std::clamp(layer.detuneCents, 0.0, 50.0);
        layer.spread = std::clamp(layer.spread, 0.0, 1.0);
        layer.cutoffHz = std::clamp(layer.cutoffHz, 80.0, 10000.0);
        layer.drive = std::clamp(layer.drive, 0.0, 1.0);
    }

    if (ImGui::CollapsingHeader("Pad Layer"))
    {
        auto& layer = chCfg.padLayer;
        changed |= ImGui::Checkbox("Enabled##padLayer", &layer.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "暗い持続レイヤーを重ねて背景の厚みを作ります。",
                "遅いfade、軽いdetune、薄い揺れでパッドを広げます。",
                "明るさや量を上げすぎると主旋律を邪魔します。");
        }

        changed |= ImGui::InputDouble("Level##padLayer", &layer.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Octave Level##padLayer", &layer.octaveLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Detune Cents##padLayer", &layer.detuneCents, 0.5, 2.0, "%.2f");
        changed |= ImGui::InputDouble("Fade In##padLayer", &layer.fadeInSec, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Brightness##padLayer", &layer.brightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Motion Depth##padLayer", &layer.motionDepth, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Cutoff Hz##padLayer", &layer.cutoffHz, 10.0, 100.0, "%.1f");

        layer.level = std::clamp(layer.level, 0.0, 1.0);
        layer.octaveLevel = std::clamp(layer.octaveLevel, 0.0, 1.0);
        layer.detuneCents = std::clamp(layer.detuneCents, 0.0, 80.0);
        layer.spread = std::clamp(layer.spread, 0.0, 1.0);
        layer.fadeInSec = std::clamp(layer.fadeInSec, 0.005, 5.0);
        layer.brightness = std::clamp(layer.brightness, 0.0, 1.0);
        layer.motionDepth = std::clamp(layer.motionDepth, 0.0, 1.0);
        layer.motionRateHz = std::clamp(layer.motionRateHz, 0.0, 8.0);
        layer.cutoffHz = std::clamp(layer.cutoffHz, 80.0, 10000.0);
        layer.drive = std::clamp(layer.drive, 0.0, 1.0);
    }

    if (ImGui::CollapsingHeader("Pluck / String / Body"))
    {
        auto& pluck = chCfg.pluckLayer;
        changed |= ImGui::Checkbox("Enabled##pluckLayer", &pluck.enabled);
        changed |= ImGui::InputDouble("Level##pluckLayer", &pluck.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Decay##pluckLayer", &pluck.decaySec, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Brightness##pluckLayer", &pluck.brightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Noise Mix##pluckLayer", &pluck.noiseMix, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Body Send##pluckLayer", &pluck.bodySend, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##pluckLayer", &pluck.drive, 0.01, 0.05, "%.3f");
        pluck.level = std::clamp(pluck.level, 0.0, 1.0);
        pluck.decaySec = std::clamp(pluck.decaySec, 0.02, 2.0);
        pluck.brightness = std::clamp(pluck.brightness, 0.0, 1.0);
        pluck.noiseMix = std::clamp(pluck.noiseMix, 0.0, 1.0);
        pluck.bodySend = std::clamp(pluck.bodySend, 0.0, 1.0);
        pluck.drive = std::clamp(pluck.drive, 0.0, 1.0);

        ImGui::Separator();
        auto& stringLayer = chCfg.stringLayer;
        changed |= ImGui::Checkbox("Enabled##stringLayer", &stringLayer.enabled);
        changed |= ImGui::InputDouble("Level##stringLayer", &stringLayer.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Bow Level##stringLayer", &stringLayer.bowLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Detune Cents##stringLayer", &stringLayer.detuneCents, 0.5, 2.0, "%.2f");
        changed |= ImGui::InputDouble("Spread##stringLayer", &stringLayer.spread, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Fade In##stringLayer", &stringLayer.fadeInSec, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Brightness##stringLayer", &stringLayer.brightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Motion Depth##stringLayer", &stringLayer.motionDepth, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Body Send##stringLayer", &stringLayer.bodySend, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##stringLayer", &stringLayer.drive, 0.01, 0.05, "%.3f");
        stringLayer.level = std::clamp(stringLayer.level, 0.0, 1.0);
        stringLayer.bowLevel = std::clamp(stringLayer.bowLevel, 0.0, 1.0);
        stringLayer.detuneCents = std::clamp(stringLayer.detuneCents, 0.0, 80.0);
        stringLayer.spread = std::clamp(stringLayer.spread, 0.0, 1.0);
        stringLayer.fadeInSec = std::clamp(stringLayer.fadeInSec, 0.005, 3.0);
        stringLayer.brightness = std::clamp(stringLayer.brightness, 0.0, 1.0);
        stringLayer.motionDepth = std::clamp(stringLayer.motionDepth, 0.0, 1.0);
        stringLayer.bodySend = std::clamp(stringLayer.bodySend, 0.0, 1.0);
        stringLayer.drive = std::clamp(stringLayer.drive, 0.0, 1.0);

        ImGui::Separator();
        auto& body = chCfg.bodyLayer;
        changed |= ImGui::Checkbox("Enabled##bodyLayer", &body.enabled);
        const char* bodyModes[] = { "Harmonic", "Box", "Metal" };
        int bodyMode = 1;
        switch (body.mode)
        {
        case BodyLayerConfig::Mode::Harmonic: bodyMode = 0; break;
        case BodyLayerConfig::Mode::Box: bodyMode = 1; break;
        case BodyLayerConfig::Mode::Metal: bodyMode = 2; break;
        }
        if (ImGui::Combo("Mode##bodyLayer", &bodyMode, bodyModes, IM_ARRAYSIZE(bodyModes)))
        {
            body.mode = bodyMode == 0
                ? BodyLayerConfig::Mode::Harmonic
                : (bodyMode == 2 ? BodyLayerConfig::Mode::Metal : BodyLayerConfig::Mode::Box);
            changed = true;
        }
        changed |= ImGui::InputDouble("Mix##bodyLayer", &body.mix, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Size##bodyLayer", &body.size, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Tone##bodyLayer", &body.tone, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Damping##bodyLayer", &body.damping, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Stereo##bodyLayer", &body.stereo, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##bodyLayer", &body.drive, 0.01, 0.05, "%.3f");
        body.mix = std::clamp(body.mix, 0.0, 1.0);
        body.size = std::clamp(body.size, 0.0, 1.0);
        body.tone = std::clamp(body.tone, 0.0, 1.0);
        body.damping = std::clamp(body.damping, 0.0, 1.0);
        body.stereo = std::clamp(body.stereo, 0.0, 1.0);
        body.drive = std::clamp(body.drive, 0.0, 1.0);
    }

    if (ImGui::CollapsingHeader("Harmonic Layer"))
    {
        auto& harmonic = chCfg.harmonicLayer;
        changed |= ImGui::Checkbox("Enabled##harmonicLayer", &harmonic.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "押した音だけを基準に整数倍音を足します。",
                "オルガンや鍵盤の厚みを、和音やノイズを混ぜずに作ります。",
                "高次倍音やdriveを上げすぎると硬くなります。");
        }

        changed |= ImGui::InputDouble("Level##harmonicLayer", &harmonic.level, 0.01, 0.05, "%.3f");
        const char* harmonicLabels[] = { "1x", "2x", "3x", "4x", "5x", "6x", "8x", "10x" };
        for (size_t h = 0; h < harmonic.harmonicLevels.size(); h++)
        {
            std::string label = std::string("Harmonic ") + harmonicLabels[h] + "##harmonicLayer";
            changed |= ImGui::InputDouble(label.c_str(), &harmonic.harmonicLevels[h], 0.01, 0.05, "%.3f");
            harmonic.harmonicLevels[h] = std::clamp(harmonic.harmonicLevels[h], 0.0, 1.0);
        }
        changed |= ImGui::InputDouble("Brightness##harmonicLayer", &harmonic.brightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Key Click##harmonicLayer", &harmonic.keyClick, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Attack Sec##harmonicLayer", &harmonic.attackSec, 0.001, 0.01, "%.3f");
        changed |= ImGui::InputDouble("Release Damp##harmonicLayer", &harmonic.releaseDamp, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##harmonicLayer", &harmonic.drive, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Stereo##harmonicLayer", &harmonic.stereo, 0.01, 0.05, "%.3f");
        harmonic.level = std::clamp(harmonic.level, 0.0, 1.0);
        harmonic.brightness = std::clamp(harmonic.brightness, 0.0, 1.0);
        harmonic.keyClick = std::clamp(harmonic.keyClick, 0.0, 1.0);
        harmonic.attackSec = std::clamp(harmonic.attackSec, 0.001, 0.25);
        harmonic.releaseDamp = std::clamp(harmonic.releaseDamp, 0.0, 1.0);
        harmonic.drive = std::clamp(harmonic.drive, 0.0, 1.0);
        harmonic.stereo = std::clamp(harmonic.stereo, 0.0, 1.0);
    }

    if (ImGui::CollapsingHeader("Rock Layers"))
    {
        auto& powerChord = chCfg.powerChordLayer;
        changed |= ImGui::Checkbox("Enabled##powerChordLayer", &powerChord.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "押した音を基準に5度とオクターブだけを足します。",
                "ギターのPower Chord向けで、ChordLayerのような複雑な和音は作りません。",
                "LevelやDriveを上げすぎると低域が濁ります。");
        }
        changed |= ImGui::InputDouble("Level##powerChordLayer", &powerChord.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Fifth Level##powerChordLayer", &powerChord.fifthLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Octave Level##powerChordLayer", &powerChord.octaveLevel, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Detune Cents##powerChordLayer", &powerChord.detuneCents, 0.1, 1.0, "%.2f");
        changed |= ImGui::InputDouble("Spread##powerChordLayer", &powerChord.spread, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Tone##powerChordLayer", &powerChord.tone, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##powerChordLayer", &powerChord.drive, 0.01, 0.05, "%.3f");
        powerChord.level = std::clamp(powerChord.level, 0.0, 1.0);
        powerChord.fifthLevel = std::clamp(powerChord.fifthLevel, 0.0, 1.0);
        powerChord.octaveLevel = std::clamp(powerChord.octaveLevel, 0.0, 1.0);
        powerChord.detuneCents = std::clamp(powerChord.detuneCents, 0.0, 18.0);
        powerChord.spread = std::clamp(powerChord.spread, 0.0, 1.0);
        powerChord.tone = std::clamp(powerChord.tone, 0.0, 1.0);
        powerChord.drive = std::clamp(powerChord.drive, 0.0, 1.0);

        ImGui::Separator();
        auto& chug = chCfg.chugLayer;
        changed |= ImGui::Checkbox("Enabled##chugLayer", &chug.enabled);
        changed |= ImGui::InputDouble("Level##chugLayer", &chug.level, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Decay##chugLayer", &chug.decaySec, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Low Punch##chugLayer", &chug.lowPunch, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Pick##chugLayer", &chug.pick, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Tone##chugLayer", &chug.tone, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Tightness##chugLayer", &chug.tightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Drive##chugLayer", &chug.drive, 0.01, 0.05, "%.3f");
        chug.level = std::clamp(chug.level, 0.0, 1.0);
        chug.decaySec = std::clamp(chug.decaySec, 0.025, 0.75);
        chug.lowPunch = std::clamp(chug.lowPunch, 0.0, 1.0);
        chug.pick = std::clamp(chug.pick, 0.0, 1.0);
        chug.tone = std::clamp(chug.tone, 0.0, 1.0);
        chug.tightness = std::clamp(chug.tightness, 0.0, 1.0);
        chug.drive = std::clamp(chug.drive, 0.0, 1.0);

        ImGui::Separator();
        auto& ampCab = chCfg.ampCabLayer;
        changed |= ImGui::Checkbox("Enabled##ampCabLayer", &ampCab.enabled);
        changed |= ImGui::InputDouble("Drive##ampCabLayer", &ampCab.drive, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Tone##ampCabLayer", &ampCab.tone, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Cab Low##ampCabLayer", &ampCab.cabLow, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Cab High##ampCabLayer", &ampCab.cabHigh, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Presence##ampCabLayer", &ampCab.presence, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Output##ampCabLayer", &ampCab.output, 0.01, 0.05, "%.3f");
        ampCab.drive = std::clamp(ampCab.drive, 0.0, 1.0);
        ampCab.tone = std::clamp(ampCab.tone, 0.0, 1.0);
        ampCab.cabLow = std::clamp(ampCab.cabLow, 0.0, 1.0);
        ampCab.cabHigh = std::clamp(ampCab.cabHigh, 0.0, 1.0);
        ampCab.presence = std::clamp(ampCab.presence, 0.0, 1.0);
        ampCab.output = std::clamp(ampCab.output, 0.0, 1.4);
    }

    if (ImGui::CollapsingHeader("Expression Map"))
    {
        auto& map = chCfg.expressionMap;
        changed |= ImGui::Checkbox("Enabled##expressionMap", &map.enabled);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "Velocityを音量だけでなく音色変化にも割り当てます。",
                "強く弾いた音で明るさ、FM index、各Layerの押し出しを自然に増やします。",
                "上げすぎると強弱差が過剰になり、ピークや音色の暴れが出ます。");
        }

        changed |= ImGui::InputDouble("Velocity Curve##expressionMap", &map.velocityCurve, 0.05, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Amp##expressionMap", &map.velocityToAmp, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Brightness##expressionMap", &map.velocityToBrightness, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To FM Index##expressionMap", &map.velocityToFmIndex, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Attack##expressionMap", &map.velocityToAttack, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Bass##expressionMap", &map.velocityToBass, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Lead##expressionMap", &map.velocityToLead, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Pluck##expressionMap", &map.velocityToPluck, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To String##expressionMap", &map.velocityToString, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Velocity To Body##expressionMap", &map.velocityToBody, 0.01, 0.05, "%.3f");
        changed |= ImGui::InputDouble("Pressure To Filter Drive##expressionMap", &map.pressureToFilterDrive, 0.01, 0.05, "%.3f");

        map.velocityCurve = std::clamp(map.velocityCurve, 0.2, 3.0);
        map.velocityToAmp = std::clamp(map.velocityToAmp, 0.0, 1.0);
        map.velocityToBrightness = std::clamp(map.velocityToBrightness, -1.0, 1.0);
        map.velocityToFmIndex = std::clamp(map.velocityToFmIndex, 0.0, 1.0);
        map.velocityToAttack = std::clamp(map.velocityToAttack, 0.0, 1.0);
        map.velocityToBass = std::clamp(map.velocityToBass, 0.0, 1.0);
        map.velocityToLead = std::clamp(map.velocityToLead, 0.0, 1.0);
        map.velocityToPluck = std::clamp(map.velocityToPluck, 0.0, 1.0);
        map.velocityToString = std::clamp(map.velocityToString, 0.0, 1.0);
        map.velocityToBody = std::clamp(map.velocityToBody, 0.0, 1.0);
        map.velocityToChord = std::clamp(map.velocityToChord, 0.0, 1.0);
        map.velocityToPad = std::clamp(map.velocityToPad, 0.0, 1.0);
        map.modWheelToBrightness = std::clamp(map.modWheelToBrightness, -1.0, 1.0);
        map.modWheelToPad = std::clamp(map.modWheelToPad, 0.0, 1.0);
        map.pressureToDrive = std::clamp(map.pressureToDrive, 0.0, 1.0);
        map.pressureToFilterDrive = std::clamp(map.pressureToFilterDrive, 0.0, 1.0);
        map.cc74ToBrightness = std::clamp(map.cc74ToBrightness, -1.0, 1.0);
        map.cc74ToPadBrightness = std::clamp(map.cc74ToPadBrightness, -1.0, 1.0);
    }

    if (ImGui::CollapsingHeader("音源詳細", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (showSourceTypeSelector)
        {
            const config::SourceKind selectedKind = config::SourceConfigKind(chCfg.source);
            size_t guiSourceKindCount = 0;
            const auto guiSourceKinds = config::GuiSelectableSourceKinds(guiSourceKindCount);
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
                        chCfg.source = config::DefaultSourceConfig(candidate);
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

        changed |= detail::DrawSourceEditor(chCfg, selectedDrumNote, updateHoverHelp);
    }
    return changed;
}
} // namespace gui
