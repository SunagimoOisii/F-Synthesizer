namespace
{
void ApplyBlueprintBeatLightPalette(ImGuiStyle& style)
{
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.05f, 0.14f, 0.26f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.45f, 0.58f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.93f, 0.96f, 0.99f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.90f, 0.95f, 0.99f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.96f, 0.98f, 1.00f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.34f, 0.53f, 0.72f, 0.65f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.92f, 0.98f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.77f, 0.88f, 0.98f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.68f, 0.82f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.82f, 0.90f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.72f, 0.83f, 0.94f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.88f, 0.93f, 0.98f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.90f, 0.95f, 0.99f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.88f, 0.93f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.72f, 0.88f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.65f, 0.84f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.57f, 0.80f, 0.90f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.41f, 0.73f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.41f, 0.73f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.33f, 0.62f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.73f, 0.85f, 0.96f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.62f, 0.79f, 0.95f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.52f, 0.72f, 0.92f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.75f, 0.86f, 0.97f, 0.90f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.79f, 0.95f, 0.90f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.72f, 0.92f, 0.90f);
    colors[ImGuiCol_Separator] = ImVec4(0.34f, 0.53f, 0.72f, 0.65f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.46f, 0.68f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.18f, 0.40f, 0.65f, 0.82f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.34f, 0.53f, 0.72f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.46f, 0.70f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.18f, 0.40f, 0.65f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.82f, 0.90f, 0.97f, 0.95f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.64f, 0.79f, 0.95f, 0.95f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.73f, 0.85f, 0.96f, 0.95f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.87f, 0.92f, 0.97f, 0.95f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.80f, 0.87f, 0.95f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.18f, 0.50f, 0.82f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.18f, 0.50f, 0.82f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.55f, 0.72f, 0.88f, 0.45f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.41f, 0.73f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.11f, 0.41f, 0.73f, 0.80f);
    colors[ImGuiCol_TextLink] = ImVec4(0.15f, 0.45f, 0.75f, 1.00f);
}

void ApplyBlueprintBeatDarkPalette(ImGuiStyle& style)
{
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.54f, 0.64f, 0.76f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.11f, 0.20f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.15f, 0.26f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.17f, 0.30f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.44f, 0.60f, 0.81f, 0.68f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.23f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.30f, 0.48f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.38f, 0.58f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.17f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.23f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.13f, 0.23f, 0.75f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.04f, 0.11f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.11f, 0.20f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.29f, 0.49f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.37f, 0.58f, 0.78f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.47f, 0.68f, 0.88f, 0.90f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.63f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.63f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.74f, 0.91f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.39f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.48f, 0.71f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.29f, 0.46f, 0.88f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.41f, 0.62f, 0.88f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.51f, 0.74f, 0.88f);
    colors[ImGuiCol_Separator] = ImVec4(0.44f, 0.60f, 0.81f, 0.68f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.70f, 0.88f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.65f, 0.80f, 0.95f, 0.82f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.60f, 0.81f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.70f, 0.88f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.63f, 0.80f, 0.95f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.23f, 0.38f, 0.95f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.41f, 0.62f, 0.95f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.15f, 0.33f, 0.52f, 0.95f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.06f, 0.15f, 0.26f, 0.95f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.10f, 0.25f, 0.41f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.63f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.63f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.12f, 0.29f, 0.46f, 0.45f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.63f, 0.85f, 1.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.63f, 0.85f, 1.00f, 0.80f);
    colors[ImGuiCol_TextLink] = ImVec4(0.67f, 0.88f, 1.00f, 1.00f);
}

void ApplyUIThemeStyle(const GUIState& state, const ImGuiStyle& defaultDarkStyle)
{
    static int lastThemeIndex = -1;
    if (state.UIThemeIndex == lastThemeIndex)
    {
        return;
    }
    lastThemeIndex = state.UIThemeIndex;
    ImGuiStyle& style = ImGui::GetStyle();
    style = defaultDarkStyle;
    if (state.UIThemeIndex == 1)
    {
        ApplyBlueprintBeatDarkPalette(style);
    }
    else
    {
        ApplyBlueprintBeatLightPalette(style);
    }
}
} // namespace

int RunGUIApp()
{
    if (!glfwInit())
    {
        return 1;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "F-Synthesizer", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    SetupImGuiFont();
    ImGui::StyleColorsDark();
    const ImGuiStyle defaultDarkStyle = ImGui::GetStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    GUIState state{};
    // 起動時に「既定値 -> 保存状態の復元 -> 不正値修復」の順で状態を確定する。
    InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });

    {
        std::string err;
        if (!LoadGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state load failed: " + err);
        }
        else
        {
            AppendGUILog(state, "[GUI] gui_state loaded: " + PathToUtf8(GUIStatePath()));
        }
        RepairGUIStatePaths(
            state,
            [&](const std::string& preferName) { RefreshPresetItems(state, preferName); },
            [&](const std::string& line) { AppendGUILog(state, line); });
    }
    int lastFrameTab = state.UIModeTab;
    int pendingPresetIndex = -1;
    int pendingPresetOriginalIndex = -1;
    bool pendingCloseRequest = false;
    bool openUnsavedPopupNextFrame = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // 非同期Runの完了を毎フレーム先頭で回収し、UI遷移を遅延させない。
        TryFinalizeCompletedRun(state);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ApplyUIThemeStyle(state, defaultDarkStyle);
        ImGui::GetIO().FontGlobalScale = UIScaleFromIndex(state.UIScaleIndex);
        DrawMainWindowFrame(state, window, lastFrameTab, pendingPresetIndex, pendingPresetOriginalIndex, pendingCloseRequest, openUnsavedPopupNextFrame);

        ImGui::Render();
        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        const ImVec4& bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    if (state.running && state.runFuture.valid())
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        try
        {
            state.lastRunExitCode = state.runFuture.get();
        }
        catch (const std::exception& ex)
        {
            state.lastRunExitCode = 1;
            AppendGUILog(state, std::string("[GUI] Run shutdown exception: ") + ex.what());
        }
        catch (...)
        {
            state.lastRunExitCode = 1;
            AppendGUILog(state, "[GUI] Run shutdown exception: unknown exception");
        }
        state.hasRun = true;
        state.running = false;
    }

    {
        std::string err;
        if (!SaveGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state save failed: " + err);
        }
    }

    ShutdownPreviewAudio(state.playback);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
