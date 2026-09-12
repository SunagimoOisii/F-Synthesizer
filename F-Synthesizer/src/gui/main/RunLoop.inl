int RunGUIApp()
{
    // Shell dialogs require an STA. Keep it alive until audio and windows are
    // destroyed, including early returns during GUI initialization.
    struct COMScope
    {
        HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        ~COMScope() { if (SUCCEEDED(result)) CoUninitialize(); }
    } com;
    if (FAILED(com.result))
    {
        MessageBoxW(nullptr, L"Windowsの画面処理を初期化できませんでした。", L"F-Synthesizer", MB_OK | MB_ICONERROR);
        return 1;
    }
    wchar_t capturePath[2048]{};
    const bool captureMode = GetEnvironmentVariableW(L"FSYNTH_CAPTURE", capturePath, 2048) > 0;
    bool capturePlaying = false;
    if (!glfwInit())
    {
        return 1;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    const GLFWvidmode* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const int width = videoMode ? std::min(1440, videoMode->width - 80) : 1280;
    const int height = videoMode ? std::min(900, videoMode->height - 100) : 720;
    if (captureMode) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(width, height, "F-Synthesizer", nullptr, nullptr);
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
    studio::fonts();
    ImGui::StyleColorsDark();


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    auto stateOwner = std::make_unique<GUIState>();
    GUIState& state = *stateOwner;
    // 起動時に「既定値 -> 保存状態の復元 -> 不正値修復」の順で状態を確定する。
    InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });

    {
        std::string err;
        if (!LoadGUIStateFile(state, err))
        {
            state.skipWorkspaceAutosave = true;
            AppendGUILog(state, "[GUI] Workspace load failed: " + err);
        }
        else
        {
            AppendGUILog(state, "[GUI] Workspace restored: " + PathToUtf8(GUIStatePath()));
        }
        RepairGUIStatePaths(
            state,
            [&](const std::string& preferName) { RefreshPresetItems(state, preferName); },
            [&](const std::string& line) { AppendGUILog(state, line); });
    }
    RefreshPresetItems(state, state.presetName);
    if (state.midiPath[0]) gui::LoadPianoRollMIDI(state.pianoRoll, Utf8ToPath(state.midiPath));
    gui::InitializeToneWorkspace(state);
    gui::SelectToneChannel(state, state.pianoRoll.displayChannel);
    glfwSetWindowSizeLimits(window, 1240, 900, GLFW_DONT_CARE, GLFW_DONT_CARE);
    if (captureMode)
    {
        wchar_t view[32]{}; GetEnvironmentVariableW(L"FSYNTH_CAPTURE_VIEW", view, 32);
        const std::wstring mode(view);
        state.toneExtraOpen = mode.find(L"controls") != std::wstring::npos;
        state.toneNotesOpen = mode.find(L"notes") != std::wstring::npos || mode == L"drums";
        if (mode.find(L"compact") != std::wstring::npos) glfwSetWindowSize(window, 1240, 900);
        if (mode == L"drums") { gui::SelectToneChannel(state, 9); state.stepSeq.viewActive = true; LoadStepSeqFromPianoRoll(state.stepSeq, state.pianoRoll); }
        if (mode == L"playing") { capturePlaying = true; state.scopeWholeMix = true; gui::RequestSongPlayback(state); }
    }
    int lastFrameTab = state.UIModeTab;

    int captureFrame = 0;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // 非同期Runの完了を毎フレーム先頭で回収し、UI遷移を遅延させない。
        TryFinalizeCompletedRun(state);
        gui::UpdateGUITransport(state);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().FontGlobalScale = 1.f;
        DrawMainWindowFrame(state, window, lastFrameTab);
        if (state.running && state.runIsPreview) gui::PublishLiveRenderSettings(state);

        ImGui::Render();
        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        const ImVec4& bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (captureMode && ++captureFrame >= 20 && (!capturePlaying || ImGui::GetTime() >= 3))
        { studio::capture(capturePath, displayW, displayH); break; }
        glfwSwapBuffers(window);
        if (captureMode) glfwWaitEventsTimeout(.01);
    }
    if (state.running && state.runFuture.valid())
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        StopPreviewAudio(state.playback);
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
        if (!captureMode && !state.skipWorkspaceAutosave && !SaveGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] Workspace save failed: " + err);
        }
    }

    ShutdownPreviewAudio(state.playback);
    glDeleteTextures(1, &studio::iconTexture);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
