// FmAlgorithmDiagram.inl
// FM Algorithm 0〜7 のオペレーター接続図を DrawList で描画する。
// DrawFmAlgorithmDiagram(id, algorithm) を ChannelEditorFm.inl から呼び出す。
// GUIChannelEditor.cpp 匿名名前空間に #include して使用する。

#include <cmath>
#include <cstdio>

static void DrawFmAlgorithmDiagram(const char* id, int algorithm)
{
    const float kCH = 80.0f; // canvas height
    const float kBW = 36.0f; // op box width
    const float kBH = 22.0f; // op box height
    const float kHalfW = kBW * 0.5f;
    const float kHalfH = kBH * 0.5f;

    const float cW = std::max(ImGui::GetContentRegionAvail().x, 200.0f);
    ImGui::InvisibleButton(id, ImVec2(cW, kCH));
    const ImVec2 org = ImGui::GetItemRectMin();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // ---- Background ----
    dl->AddRectFilled(org, { org.x + cW, org.y + kCH }, IM_COL32(18, 18, 18, 215), 4.0f);
    dl->AddRect(org, { org.x + cW, org.y + kCH }, IM_COL32(55, 55, 55, 180), 4.0f);

    // ---- Colors ----
    const ImU32 kColCarFill = IM_COL32(160, 120, 20, 230); // gold
    const ImU32 kColModFill = IM_COL32(30, 65, 160, 230); // blue
    const ImU32 kColCarBd = IM_COL32(255, 210, 60, 255);
    const ImU32 kColModBd = IM_COL32(90, 150, 255, 255);
    const ImU32 kColArrow = IM_COL32(200, 200, 200, 220);
    const ImU32 kColFb = IM_COL32(220, 145, 40, 210);

    // ---- Op position descriptor ----
    struct OpPos
    {
        float cx, cy;
        bool isCarrier;
        bool active;
    };

    // ---- Draw helpers ----
    auto drawArrowhead = [&](ImVec2 tip, float ndx, float ndy, float al, float aw, ImU32 col)
    {
        const float bx = tip.x - ndx * al;
        const float by = tip.y - ndy * al;
        dl->AddTriangleFilled(tip,
            { bx - ndy * aw, by + ndx * aw },
            { bx + ndy * aw, by - ndx * aw },
            col);
    };

    auto drawOp = [&](const OpPos& op, int opIdx)
    {
        if (!op.active) { return; }
        const float x0 = org.x + op.cx - kHalfW;
        const float y0 = org.y + op.cy - kHalfH;
        const float x1 = x0 + kBW;
        const float y1 = y0 + kBH;
        dl->AddRectFilled({ x0, y0 }, { x1, y1 }, op.isCarrier ? kColCarFill : kColModFill, 3.0f);
        dl->AddRect({ x0, y0 }, { x1, y1 }, op.isCarrier ? kColCarBd : kColModBd, 3.0f);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d/%s", opIdx + 1, op.isCarrier ? "C" : "M");
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const ImU32 textCol = op.isCarrier ? IM_COL32(255, 228, 100, 255)
            : IM_COL32(160, 200, 255, 255);
        dl->AddText({ x0 + (kBW - ts.x) * 0.5f, y0 + (kBH - ts.y) * 0.5f }, textCol, buf);
    };

    auto drawArrow = [&](const OpPos& from, const OpPos& to)
    {
        const float x0 = org.x + from.cx + kHalfW;
        const float y0 = org.y + from.cy;
        const float x1 = org.x + to.cx - kHalfW;
        const float y1 = org.y + to.cy;
        const float dx = x1 - x0;
        const float dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.01f) { return; }
        const float ndx = dx / len;
        const float ndy = dy / len;
        dl->AddLine({ x0, y0 }, { x1 - ndx * 7.0f, y1 - ndy * 7.0f }, kColArrow, 1.5f);
        drawArrowhead({ x1, y1 }, ndx, ndy, 7.0f, 3.5f, kColArrow);
    };

    // Feedback bezier arc above Op0 top edge
    auto drawFeedback = [&](const OpPos& op)
    {
        if (!op.active) { return; }
        const float cx = org.x + op.cx;
        const float ty = org.y + op.cy - kHalfH; // top of box
        const ImVec2 p0 = { cx - 10.0f, ty };
        const ImVec2 c1 = { cx - 26.0f, ty - 22.0f };
        const ImVec2 c2 = { cx + 26.0f, ty - 22.0f };
        const ImVec2 p3 = { cx + 10.0f, ty };
        dl->AddBezierCubic(p0, c1, c2, p3, kColFb, 1.5f);

        const float tdx = p3.x - c2.x;
        const float tdy = p3.y - c2.y;
        const float tlen = std::sqrt(tdx * tdx + tdy * tdy);
        if (tlen > 0.01f)
        {
            drawArrowhead(p3, tdx / tlen, tdy / tlen, 6.0f, 3.0f, kColFb);
        }
    };

    // ---- Per-algorithm layout ----
    constexpr float kMidY = 40.0f;
    constexpr float kTopY = 18.0f;
    constexpr float kBotY = 62.0f;

    OpPos ops[4] = {};
    int arrows[6][2] = {};
    int arrowCount = 0;

    switch (algorithm)
    {
    case 0: // Op0(M+fb) → Op1(C)
        ops[0] = { 65, kMidY, false, true };
        ops[1] = { 185, kMidY, true, true };
        ops[2] = { 0, 0, true, false };
        ops[3] = { 0, 0, true, false };
        arrows[0][0] = 0; arrows[0][1] = 1; arrowCount = 1;
        break;
    case 1:
    case 4: // [Op0→Op1] + [Op2→Op3]
        ops[0] = { 55, kTopY, false, true };
        ops[1] = { 160, kTopY, true, true };
        ops[2] = { 55, kBotY, false, true };
        ops[3] = { 160, kBotY, true, true };
        arrows[0][0] = 0; arrows[0][1] = 1;
        arrows[1][0] = 2; arrows[1][1] = 3;
        arrowCount = 2;
        break;
    case 2:
    case 5: // Op0 → {Op1, Op2, Op3}
        ops[0] = { 50, kMidY, false, true };
        ops[1] = { 168, kTopY, true, true };
        ops[2] = { 168, kMidY, true, true };
        ops[3] = { 168, kBotY, true, true };
        arrows[0][0] = 0; arrows[0][1] = 1;
        arrows[1][0] = 0; arrows[1][1] = 2;
        arrows[2][0] = 0; arrows[2][1] = 3;
        arrowCount = 3;
        break;
    case 3: // Op0 → Op1 → Op2 → Op3
        ops[0] = { 25, kMidY, false, true };
        ops[1] = { 85, kMidY, false, true };
        ops[2] = { 145, kMidY, false, true };
        ops[3] = { 205, kMidY, true, true };
        arrows[0][0] = 0; arrows[0][1] = 1;
        arrows[1][0] = 1; arrows[1][1] = 2;
        arrows[2][0] = 2; arrows[2][1] = 3;
        arrowCount = 3;
        break;
    case 6: // Op0→Op1, Op2(独立C), Op3(独立C)
        ops[0] = { 28, kMidY, false, true };
        ops[1] = { 95, kMidY, true, true };
        ops[2] = { 155, kMidY, true, true };
        ops[3] = { 215, kMidY, true, true };
        arrows[0][0] = 0; arrows[0][1] = 1;
        arrowCount = 1;
        break;
    case 7: // 全キャリア
        ops[0] = { 25, kMidY, true, true };
        ops[1] = { 90, kMidY, true, true };
        ops[2] = { 155, kMidY, true, true };
        ops[3] = { 220, kMidY, true, true };
        arrowCount = 0;
        break;
    default:
        break;
    }

    for (int a = 0; a < arrowCount; ++a)
    {
        drawArrow(ops[arrows[a][0]], ops[arrows[a][1]]);
    }
    drawFeedback(ops[0]);

    for (int i = 0; i < 4; ++i)
    {
        drawOp(ops[i], i);
    }

    constexpr float kLegY = 68.0f;
    dl->AddRectFilled({ org.x + 5, org.y + kLegY },
        { org.x + 21, org.y + kLegY + 10 }, kColCarFill, 2.0f);
    dl->AddText({ org.x + 24, org.y + kLegY }, IM_COL32(255, 228, 100, 200), "Carrier");
    dl->AddRectFilled({ org.x + 78, org.y + kLegY },
        { org.x + 94, org.y + kLegY + 10 }, kColModFill, 2.0f);
    dl->AddText({ org.x + 97, org.y + kLegY }, IM_COL32(160, 200, 255, 200), "Modulator");

    dl->AddLine({ org.x + 170, org.y + kLegY + 5 }, { org.x + 185, org.y + kLegY + 5 }, kColFb, 1.5f);
    dl->AddText({ org.x + 188, org.y + kLegY }, IM_COL32(220, 145, 40, 200), "FB");
}
