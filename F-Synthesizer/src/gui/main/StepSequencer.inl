// StepSequencer.inl
// DrawStepSeqPanel: GUIMain.cpp 匿名名前空間から呼び出す。
// ApplyStepSeqNotes を介して pianoRoll.notes を更新する。

struct StepSeqRowDef
{
    int midiNote;
    const char* label;
};

static constexpr StepSeqRowDef kStepSeqRows[GUIStepSeqState::kRows] = {
    { 36, "Kick" },
    { 38, "Snare" },
    { 42, "C.HH" },
    { 46, "O.HH" },
    { 45, "LTom" },
    { 48, "MTom" },
    { 49, "Crash" },
};

static void LoadStepSeqFromPianoRoll(GUIStepSeqState& ss, const gui::PianoRollState& pr)
{
    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            ss.steps[r][s] = false;
        }
    }

    const int tpq = (pr.ticksPerQuarter > 0) ? pr.ticksPerQuarter : 480;
    const int stepLen = tpq / 4;
    if (stepLen <= 0)
    {
        return;
    }

    for (const auto& n : pr.notes)
    {
        if (n.channel != 9)
        {
            continue;
        }
        for (int r = 0; r < GUIStepSeqState::kRows; ++r)
        {
            if (kStepSeqRows[r].midiNote != n.note)
            {
                continue;
            }
            const int step = n.startTick / stepLen;
            if (step >= 0 && step < GUIStepSeqState::kSteps)
            {
                ss.steps[r][step] = true;
                ss.velocity[r] = std::clamp(n.velocity, 1, 127);
            }
            break;
        }
    }
}

static void FlushStepSeqToPianoRoll(const GUIStepSeqState& ss, gui::PianoRollState& pr)
{
    const int tpq = (pr.ticksPerQuarter > 0) ? pr.ticksPerQuarter : 480;
    const int stepLen = tpq / 4;
    if (stepLen <= 0)
    {
        return;
    }

    std::vector<gui::PianoRollNote> ch9Notes;
    ch9Notes.reserve(static_cast<size_t>(GUIStepSeqState::kRows * GUIStepSeqState::kSteps));
    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            if (!ss.steps[r][s])
            {
                continue;
            }
            gui::PianoRollNote n{};
            n.channel = 9;
            n.note = kStepSeqRows[r].midiNote;
            n.velocity = std::clamp(ss.velocity[r], 1, 127);
            n.startTick = s * stepLen;
            n.endTick = n.startTick + stepLen - 1;
            ch9Notes.push_back(n);
        }
    }
    gui::ApplyStepSeqNotes(pr, ch9Notes);
}

static void DrawStepSeqPanel(GUIState& state)
{
    GUIStepSeqState& ss = state.stepSeq;
    gui::PianoRollState& pr = state.pianoRoll;

    if (state.midiPath[0] == '\0' || pr.loadedMidiPath.empty())
    {
        ImGui::TextDisabled("MIDI ファイルをロードするとステップが反映されます。");
    }

    constexpr float kRowH = 22.0f;
    constexpr float kLabelW = 52.0f;
    constexpr float kStepW = 22.0f;
    constexpr float kVelW = 48.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kLabelW + 4.0f);
    for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
    {
        if (s > 0)
        {
            ImGui::SameLine(0.0f, 2.0f);
        }
        if (s % 4 == 0 && s > 0)
        {
            ImGui::SameLine(0.0f, 6.0f);
        }
        ImGui::TextDisabled("%d", s + 1);
    }

    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        ImGui::PushID(r);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(kStepSeqRows[r].label);
        ImGui::SameLine(kLabelW + 4.0f);

        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            if (s > 0)
            {
                ImGui::SameLine(0.0f, 2.0f);
            }
            if (s % 4 == 0 && s > 0)
            {
                ImGui::SameLine(0.0f, 6.0f);
            }

            ImGui::PushID(s);
            const bool on = ss.steps[r][s];
            if (on)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(180, 120, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(210, 150, 60, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 90, 20, 255));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(55, 55, 55, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(90, 90, 90, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(35, 35, 35, 255));
            }
            if (ImGui::Button("##step", ImVec2(kStepW, kRowH)))
            {
                ss.steps[r][s] = !ss.steps[r][s];
                FlushStepSeqToPianoRoll(ss, pr);
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }

        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetNextItemWidth(kVelW);
        if (ImGui::SliderInt("##vel", &ss.velocity[r], 1, 127, "%d"))
        {
            FlushStepSeqToPianoRoll(ss, pr);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Row velocity: %d", ss.velocity[r]);
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    if (ImGui::Button("Clear All"))
    {
        for (int r = 0; r < GUIStepSeqState::kRows; ++r)
        {
            for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
            {
                ss.steps[r][s] = false;
            }
        }
        FlushStepSeqToPianoRoll(ss, pr);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("全ステップをクリアします。");
    }
}
