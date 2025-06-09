#include "window.h"
#include "gpu.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "structs.h"
#include "threadPool.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <chrono>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <stdlib.h>


CMRC_DECLARE(assets);

#define IMGUI_ENABLE_FREETYPE

std::string licText;


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}



std::string find_key_by_value(const std::map<std::string, int>& my_map, int value) {
    for (const auto& pair : my_map) {
        if (pair.second == value) {
            return pair.first;
        }
    }
    return ""; // return an empty string if the value is not found in the map
}


//--- Main Window Routine ---//

int mainWindow::openWindow()
{

    // Load colorspace mappings
    loadMappings();

    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;

    // Decide GL+GLSL versions
    #if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100 (WebGL 1.0)
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    #elif defined(IMGUI_IMPL_OPENGL_ES3)
        // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
        const char* glsl_version = "#version 300 es";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    #elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    #else
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    #endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1600, 1000, "Filmvert", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    glfwWindowHint(GLFW_RED_BITS, 16);
    glfwWindowHint(GLFW_GREEN_BITS, 16);
    glfwWindowHint(GLFW_BLUE_BITS, 16);
    glfwWindowHint(GLFW_ALPHA_BITS, 16);

    //--------------------------------------------------------------//


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    //Font Loading
    auto fs = cmrc::assets::get_filesystem();
    auto regularFont = fs.open("assets/fonts/JetBrainsMono-Regular.ttf");
    if (!regularFont)
    {
        LOG_ERROR("Failed to load regular font");
        return -1;
    }
    auto boldFont = fs.open("assets/fonts/Roboto-Bold.ttf");
    if (!boldFont)
    {
        LOG_ERROR("Failed to load bold font");
        return -1;
    }

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddChar(0x2318);
    builder.AddChar(0x2325);
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.BuildRanges(&ranges);
    ImFont* ft_text = io.Fonts->AddFontFromMemoryTTF((void*)(regularFont->begin()), int(regularFont->size()), 18.0f, &font_cfg, ranges.Data);
    ImFont* ft_header = io.Fonts->AddFontFromMemoryTTF((void*)(boldFont->begin()), int(boldFont->size()), 20.0f, &font_cfg, ranges.Data);

    if (!ft_text || !ft_header)
    {
      LOG_ERROR("Failed to load fonts for window");
      return -1;
    }

    auto ocioConfigFile = fs.open("assets/studio-config-v3.0.0_aces-v2.0_ocio-v2.4.ocio");
    if (!ocioConfigFile) {
        LOG_ERROR("Error opening OCIO Config!");
        return -1;
    }
    std::string configStream(static_cast<const char*>(ocioConfigFile->begin()), int(ocioConfigFile->size()));
    //LOG_INFO("OCIO Config: {}", configStream.str());
    if (!ocioProc.initialize(configStream)) {
        return -1;
    }

    // Initialize GL Processor
    gpu = new openglGPU();
    // Initialize GL Kernels
    gpu->initialize(dispOCIO);

    // Setup Threadpool
    tPool = new ThreadPool(std::thread::hardware_concurrency());

    // Load in user preferences
    appPrefs.loadFromFile();

    if (!appPrefs.prefs.ocioPath.empty()) {
        if (ocioProc.initAltConfig(appPrefs.prefs.ocioPath) && appPrefs.prefs.ocioExt) {
            ocioProc.setExtActive();
        } else {
            ocioProc.setIntActive();
        }
    }

    // Load in logo file
    auto logoFile = fs.open("assets/logo.png");
    if (!logoFile) {
        LOG_ERROR("Error opening Logo File!");
    } else {
        loadLogoTexture(logoFile);
    }

    // Load in licenses text
    auto licFile = fs.open("assets/licenses.txt");
    if (!licFile) {
        LOG_ERROR("Error opening License File!");
    } else {
      licText = std::string(licFile->begin(), licFile->end());
    }

    // Set ini location
    setIni();


    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    imguistyle();

    // Clear all text buffers initially
    std::memset(ackMsg, 0, sizeof(ackMsg));
    std::memset(ackError, 0, sizeof(ackError));
    std::memset(ocioPath, 0, sizeof(ocioPath));
    std::memset(rollNameBuf, 0, sizeof(rollNameBuf));
    std::memset(rollPath, 0, sizeof(rollPath));


    // Main loop
    ImVec4 windowCL = ImVec4(22.0f/255.0f, 22.0f/255.0f, 29.0f/255.0f, 1.00f);
    int returnValue = -1;
    int loopCounter = 0;
    firstImage = true;
    auto start = std::chrono::steady_clock::now();
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        if (fpsFlag) {
            fps = 1000.0f / (float)dur.count();
            fpsFlag = false;
        }

        start = std::chrono::steady_clock::now();

        // Attempt close from elsewhere in the program
        if (wantClose) {
            glfwSetWindowShouldClose(window, true);
            wantClose = false;
        }

        if (glfwWindowShouldClose(window)) {
            if (unsavedChanges()) {
                // Don't quit immediately
                unsavedPopTrigger = true;
                closeMd = c_app;
                glfwSetWindowShouldClose(window, done);
            } else {
                done = true;
            }
        }

        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glfwGetWindowSize(window, &winWidth, &winHeight);
        if (loopCounter % 120 == 0) {
            // Only check periodically
            if (unsavedChanges()) {
                #ifdef __APPLE__
                setMacOSWindowModified(window, true);
                #endif
            } else {
                #ifdef __APPLE__
                setMacOSWindowModified(window, false);
                #endif
            }
            saveUI();
            loopCounter = 0;
        }

        ImGui::NewFrame();

        // Menu bar
        menuBar();

        /* DISPLAY WIN */
        imageView();

        /* CONTROLS WIN */
        paramView();

        /* THUMBNAILS WIN */
        thumbView();

        // Hotkeys
        checkHotkeys();

        if (validIm()) {
            if (activeImage() != prevIm && activeImage()->imageLoaded) {
                prevIm = activeImage();
                renderCall = true;
            }
        }

        if (renderCall && !isExporting) {
            imgRender();
            fpsFlag = true;
        }
        renderCall = false;

        // Routine for checking and writing out metadata
        checkMeta();

        // Routine for updating thumbnails
        rollRenderCheck();


        // Check for GPU errors
        if (gpu->getStatus()) {
            // We've encountered some kind of error
            std::strcpy(ackMsg, "GPU Rendering Error:");
            std::strcpy(ackError, gpu->getError().c_str());
            gpu->clearError();
            ackPopTrig = true;
        }

        // Popup functions
        importImagePopup();
        importRollPopup();
        batchRenderPopup();
        pastePopup();
        unsavedRollPopup();
        globalMetaPopup();
        localMetaPopup();
        preferencesPopup();
        shortcutsPopup();
        ackPopup();
        analyzePopup();
        importImMatchPopup();
        aboutPopup();

        // Check analyze timeout?

        loopCounter++;

        if (demoWin)
            ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // Render one image from the queue
        gpu->processQueue();

    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return returnValue;
}
