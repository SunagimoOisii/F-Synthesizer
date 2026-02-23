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
    int lastFrameTab = state.uiModeTab;
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
        ImGui::GetIO().FontGlobalScale = UiScaleFromIndex(state.uiScaleIndex);
        DrawMainWindowFrame(state, window, lastFrameTab, pendingPresetIndex, pendingPresetOriginalIndex, pendingCloseRequest, openUnsavedPopupNextFrame);

        ImGui::Render();
        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    if (state.running && state.runFuture.valid())
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        state.lastRunExitCode = state.runFuture.get();
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
