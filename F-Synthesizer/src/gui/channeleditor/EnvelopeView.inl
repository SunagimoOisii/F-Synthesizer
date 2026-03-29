// EnvelopeView.inl
// ADSR カーブと LFO 波形のインラインプレビューを描画するヘルパー関数群。
// GUIChannelEditor.cpp の匿名名前空間に #include して使用する。
//
// DrawADSRPreview  : メイン Envelope / Env2 どちらにも使用（curve=0.0f で線形）
// DrawLfo1WavePreview: LFO1 の全 LfoWave 列挙値に対応

#include <cmath>
#include <algorithm>

static bool DrawADSRPreview(
    const char* id,
    float attackSec,
    float decaySec,
    float sustainLevel,
    float releaseSec,
    float curve = 0.0f,
    float* outAttackSec = nullptr,
    float* outDecaySec = nullptr,
    float* outSustainLevel = nullptr,
    float* outReleaseSec = nullptr)
{
    constexpr int   kN           = 128;
    constexpr float kMinSegNorm  = 0.03f;
    constexpr float kMinTotalSec = 0.03f;
    constexpr float kHandleRadius = 4.5f;
    constexpr float kPickRadius   = 12.0f;
    constexpr float kPad          = 6.0f;

    float a = std::max(attackSec,  0.0f);
    float d = std::max(decaySec,   0.0f);
    float s = std::clamp(sustainLevel, 0.0f, 1.0f);
    float r = std::max(releaseSec, 0.0f);
    bool changed = false;

    // curve=0: exponent=1 (linear), curve=1: exponent≈2.72 (gradual/convex)
    const float exp_ = std::exp(curve);

    const bool editable =
        (outAttackSec != nullptr && outDecaySec != nullptr &&
         outSustainLevel != nullptr && outReleaseSec != nullptr);

    ImGui::PushID(id);
    ImGui::InvisibleButton("adsr_canvas", ImVec2(-1.0f, 54.0f));
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();
    ImDrawList*  dl      = ImGui::GetWindowDrawList();

    const ImU32 frameCol  = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 lineCol   = ImGui::GetColorU32(ImVec4(0.35f, 0.80f, 0.45f, 1.0f));
    const ImU32 handleCol = ImGui::GetColorU32(ImVec4(0.55f, 0.92f, 0.62f, 1.0f));
    dl->AddRectFilled(itemMin, itemMax, frameCol, 3.0f);
    dl->AddRect(itemMin, itemMax, borderCol, 3.0f);

    auto toCanvas = [&](float nx, float ny) -> ImVec2
    {
        const float x = itemMin.x + kPad + nx * std::max(1.0f, (itemMax.x - itemMin.x - 2.0f * kPad));
        const float y = itemMax.y - kPad - ny * std::max(1.0f, (itemMax.y - itemMin.y - 2.0f * kPad));
        return ImVec2(x, y);
    };
    auto fromCanvasX = [&](float x) -> float
    {
        const float w = std::max(1.0f, (itemMax.x - itemMin.x - 2.0f * kPad));
        return std::clamp((x - (itemMin.x + kPad)) / w, 0.0f, 1.0f);
    };
    auto fromCanvasY = [&](float y) -> float
    {
        const float h = std::max(1.0f, (itemMax.y - itemMin.y - 2.0f * kPad));
        return std::clamp((itemMax.y - kPad - y) / h, 0.0f, 1.0f);
    };

    // 各ハンドル位置を ImGuiStorage に永続化。
    // ドラッグ中は storage の値が正、非ドラッグ時は入力秒数から毎フレーム再計算。
    ImGuiStorage* storage  = ImGui::GetStateStorage();
    const ImGuiID key_x1   = ImGui::GetID("x1");
    const ImGuiID key_x2   = ImGui::GetID("x2");
    const ImGuiID key_x3   = ImGui::GetID("x3");
    const ImGuiID key_sv   = ImGui::GetID("sv");
    const ImGuiID key_ts   = ImGui::GetID("ts");   // ドラッグ開始時の totalSec
    const ImGuiID key_ah   = ImGui::GetID("ah");   // active handle index

    float x1, x2, x3, sv;

    if (editable && ImGui::IsItemActive())
    {
        // ドラッグ中: storage から読み込む
        x1 = storage->GetFloat(key_x1, 0.2f);
        x2 = storage->GetFloat(key_x2, 0.5f);
        x3 = storage->GetFloat(key_x3, 0.75f);
        sv = storage->GetFloat(key_sv, s);
    }
    else
    {
        // 非ドラッグ: 入力秒数から正規化位置を再計算して storage に書き込む
        const float totalSec = std::max(a + d + r, kMinTotalSec);
        x1 = std::clamp(a / totalSec,           kMinSegNorm,              1.0f - 2.0f * kMinSegNorm);
        x2 = std::clamp((a + d) / totalSec,     x1 + kMinSegNorm,         1.0f - kMinSegNorm);
        x3 = std::clamp(1.0f - r / totalSec,    x2 + kMinSegNorm,         1.0f - kMinSegNorm);
        sv = s;
        storage->SetFloat(key_x1, x1);
        storage->SetFloat(key_x2, x2);
        storage->SetFloat(key_x3, x3);
        storage->SetFloat(key_sv, sv);
        storage->SetFloat(key_ts, totalSec);
        storage->SetInt(key_ah, -1);
    }

    if (editable && ImGui::IsItemActive())
    {
        int activeHandle = storage->GetInt(key_ah, -1);

        // ドラッグ開始時にハンドルを選ぶ
        if (activeHandle < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const ImVec2 hp[3] = {
                toCanvas(x1, 1.0f),
                toCanvas(x2, sv),
                toCanvas(x3, sv)
            };
            float bestDist = kPickRadius;
            for (int hi = 0; hi < 3; ++hi)
            {
                const float dx = ImGui::GetIO().MousePos.x - hp[hi].x;
                const float dy = ImGui::GetIO().MousePos.y - hp[hi].y;
                const float dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= bestDist) { bestDist = dist; activeHandle = hi; }
            }
            storage->SetInt(key_ah, activeHandle);
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            storage->SetInt(key_ah, -1);
            activeHandle = -1;
        }

        if (activeHandle >= 0)
        {
            const float nx = fromCanvasX(ImGui::GetIO().MousePos.x);
            const float ny = fromCanvasY(ImGui::GetIO().MousePos.y);
            const float totalSec = storage->GetFloat(key_ts, std::max(a + d + r, kMinTotalSec));

            // 各ハンドルを独立に動かす。隣接ハンドルとの順序制約のみ適用。
            if (activeHandle == 0)
            {
                x1 = std::clamp(nx, kMinSegNorm, x2 - kMinSegNorm);
                a  = x1 * totalSec;
                d  = (x2 - x1) * totalSec;
                // r は x3 が変わっていないので不変
            }
            else if (activeHandle == 1)
            {
                x2 = std::clamp(nx, x1 + kMinSegNorm, x3 - kMinSegNorm);
                sv = std::clamp(ny, 0.0f, 1.0f);
                d  = (x2 - x1) * totalSec;
                // a, r は不変
            }
            else
            {
                x3 = std::clamp(nx, x2 + kMinSegNorm, 1.0f - kMinSegNorm);
                sv = std::clamp(ny, 0.0f, 1.0f);
                r  = (1.0f - x3) * totalSec;
                // a, d は不変
            }

            storage->SetFloat(key_x1, x1);
            storage->SetFloat(key_x2, x2);
            storage->SetFloat(key_x3, x3);
            storage->SetFloat(key_sv, sv);
            s       = sv;
            changed = true;
        }
    }

    auto evalLevel = [&](float nx) -> float
    {
        nx = std::clamp(nx, 0.0f, 1.0f);
        if (nx < x1)
        {
            const float alpha = (x1 > 0.0f) ? (nx / x1) : 1.0f;
            return std::pow(alpha, 1.0f / exp_);
        }
        if (nx < x2)
        {
            const float alpha = (x2 > x1) ? ((nx - x1) / (x2 - x1)) : 1.0f;
            return 1.0f - (1.0f - sv) * std::pow(alpha, exp_);
        }
        if (nx < x3)
        {
            return sv;
        }
        const float alpha = (x3 < 1.0f) ? ((nx - x3) / (1.0f - x3)) : 1.0f;
        return sv * (1.0f - std::pow(std::clamp(alpha, 0.0f, 1.0f), exp_));
    };

    ImVec2 pts[kN];
    for (int i = 0; i < kN; ++i)
    {
        const float nx = static_cast<float>(i) / static_cast<float>(kN - 1);
        pts[i] = toCanvas(nx, evalLevel(nx));
    }
    dl->AddPolyline(pts, kN, lineCol, 0, 2.0f);

    const ImVec2 h1 = toCanvas(x1, 1.0f);
    const ImVec2 h2 = toCanvas(x2, sv);
    const ImVec2 h3 = toCanvas(x3, sv);
    dl->AddCircleFilled(h1, kHandleRadius, handleCol, 14);
    dl->AddCircleFilled(h2, kHandleRadius, handleCol, 14);
    dl->AddCircleFilled(h3, kHandleRadius, handleCol, 14);

    if (editable && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Drag points: Attack peak / Decay end / Release start");
    }

    if (changed)
    {
        *outAttackSec   = std::max(a, 0.0f);
        *outDecaySec    = std::max(d, 0.0f);
        *outSustainLevel = sv;
        *outReleaseSec  = std::max(r, 0.0f);
    }
    ImGui::PopID();
    return changed;
}

static void DrawLfo1WavePreview(const char* id, LfoWave wave)
{
    constexpr int kN      = 128;
    constexpr int kCycles = 3;

    // S&H 表示用固定ステップパターン（代表的なランダムホールド形状）
    constexpr float kSahSteps[8] = { 0.70f, -0.30f, 1.00f, -0.80f,
                                     0.20f, -0.65f, 0.90f, -0.10f };

    float buf[kN];
    for (int i = 0; i < kN; ++i)
    {
        const float t     = static_cast<float>(i) / kN * kCycles;
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
              :                   (4.0f * phase - 4.0f);
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
    ImGui::PushStyleColor(ImGuiCol_FrameBg,   ImVec4(0.10f, 0.10f, 0.10f, 0.80f));
    ImGui::PlotLines(id, buf, kN, 0, nullptr, -1.2f, 1.2f, ImVec2(-1.0f, 40.0f));
    ImGui::PopStyleColor(2);
}
