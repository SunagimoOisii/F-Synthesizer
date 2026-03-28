// EnvelopeView.inl
// ADSR カーブと LFO 波形のインラインプレビューを描画するヘルパー関数群。
// GUIChannelEditor.cpp の匿名名前空間に #include して使用する。
//
// DrawADSRPreview  : メイン Envelope / Env2 どちらにも使用（curve=0.0f で線形）
// DrawLfo1WavePreview: LFO1 の全 LfoWave 列挙値に対応

#include <cmath>

static void DrawADSRPreview(
    const char* id,
    float attackSec,
    float decaySec,
    float sustainLevel,
    float releaseSec,
    float curve = 0.0f)
{
    constexpr int kN = 128;

    // curve=0: exponent=1 (linear), curve=1: exponent≈2.72 (gradual/convex)
    // 「低いと急激、高いとなだらか」に合わせ attack に凸カーブを使う
    const float exp_ = std::exp(curve); // 1.0 .. e (約 2.72)

    // Sustain ホールド区間: 全体に対して一定比率で確保
    const float holdSec = (attackSec + decaySec + releaseSec) * 0.3f + 0.01f;
    const float totalSec = attackSec + decaySec + holdSec + releaseSec + 0.001f;
    const float dt = totalSec / static_cast<float>(kN - 1);

    float buf[kN];
    for (int i = 0; i < kN; ++i)
    {
        const float t = i * dt;
        float level;
        if (t < attackSec)
        {
            const float alpha = attackSec > 0.0f ? t / attackSec : 1.0f;
            level = std::pow(alpha, 1.0f / exp_); // convex: curve 高いほどなだらか
        }
        else if (t < attackSec + decaySec)
        {
            const float alpha = decaySec > 0.0f ? (t - attackSec) / decaySec : 1.0f;
            level = 1.0f - (1.0f - sustainLevel) * std::pow(alpha, exp_);
        }
        else if (t < attackSec + decaySec + holdSec)
        {
            level = sustainLevel;
        }
        else
        {
            const float elapsed = t - (attackSec + decaySec + holdSec);
            const float alpha = releaseSec > 0.0f
                ? std::min(elapsed / releaseSec, 1.0f)
                : 1.0f;
            level = sustainLevel * (1.0f - std::pow(alpha, exp_));
        }
        buf[i] = level;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.35f, 0.80f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
    ImGui::PlotLines(id, buf, kN, 0, nullptr, 0.0f, 1.0f, ImVec2(-1.0f, 44.0f));
    ImGui::PopStyleColor(2);
}

static void DrawLfo1WavePreview(const char* id, LfoWave wave)
{
    constexpr int kN = 128;
    constexpr int kCycles = 3;

    // S&H 表示用固定ステップパターン（代表的なランダムホールド形状）
    constexpr float kSahSteps[8] = { 0.70f, -0.30f, 1.00f, -0.80f,
                                     0.20f, -0.65f, 0.90f, -0.10f };

    float buf[kN];
    for (int i = 0; i < kN; ++i)
    {
        const float t = static_cast<float>(i) / kN * kCycles;
        const float phase = t - std::floor(t); // 0..1 within one cycle
        float v;
        switch (wave)
        {
        case LfoWave::Sine:
            v = std::sin(phase * 6.28318f);
            break;
        case LfoWave::Triangle:
            v = (phase < 0.25f) ? (4.0f * phase)
                : (phase < 0.75f) ? (2.0f - 4.0f * phase)
                : (4.0f * phase - 4.0f);
            break;
        case LfoWave::Square:
            v = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case LfoWave::Saw:
            v = 2.0f * phase - 1.0f;
            break;
        case LfoWave::SampleAndHold:
        {
            const int step = static_cast<int>(phase * 8.0f) % 8;
            v = kSahSteps[step];
        }
        break;
        default:
            v = 0.0f;
            break;
        }
        buf[i] = v;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.40f, 0.70f, 1.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
    ImGui::PlotLines(id, buf, kN, 0, nullptr, -1.2f, 1.2f, ImVec2(-1.0f, 40.0f));
    ImGui::PopStyleColor(2);
}
