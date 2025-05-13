#include "window.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <chrono>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <stdlib.h>
#include <cmrc/cmrc.hpp>  //read embedded stuff



CMRC_DECLARE(assets);

#define IMGUI_ENABLE_FREETYPE



std::string find_key_by_value(const std::map<std::string, int>& my_map, int value) {
    for (const auto& pair : my_map) {
        if (pair.second == value) {
            return pair.first;
        }
    }
    return ""; // return an empty string if the value is not found in the map
}



int mainWindow::openWindow()
{

    // Load colorspace mappings
    loadMappings();

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        //printf("Error: %s\n", SDL_GetError());
        LOG_CRITICAL("Error initializing SDL context: {}", SDL_GetError());
        return -1;
    }

    #ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    #endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    SDL_Window* window = SDL_CreateWindow("Filmvert", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 1000, window_flags);
    if (window == nullptr)
    {
        LOG_CRITICAL("Error creating window: {}", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
      LOG_CRITICAL("Error creating SDL renderer");
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

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

    auto ocioConfigFile = fs.open("assets/studio-config-v2.2.0_aces-v1.3_ocio-v2.3.ocio");
    if (!ocioConfigFile) {
        LOG_ERROR("Error opening OCIO Config!");
        return -1;
    }
    std::string configStream(static_cast<const char*>(ocioConfigFile->begin()), int(ocioConfigFile->size()));
    //LOG_INFO("OCIO Config: {}", configStream.str());
    if (!ocioProc.initialize(configStream)) {
        return -1;
    }

    // Initialie the OCIO metal kernel
    mtlGPU->initOCIOKernels(dispOCIO);

    // Load in user preferences
    appPrefs.loadFromFile();

    if (!appPrefs.ocioPath.empty()) {
        if (ocioProc.initAltConfig(appPrefs.ocioPath) && appPrefs.ocioExt) {
            ocioProc.setExtActive();
        } else {
            ocioProc.setIntActive();
        }
    }

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    imguistyle();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);



    // Main loop
    ImVec4 windowCL = ImVec4(22.0f/255.0f, 22.0f/255.0f, 29.0f/255.0f, 1.00f);
    int returnValue = -1;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        SDL_GetWindowSize(window, &winWidth, &winHeight);

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
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

        if (renderCall && !isExporting) {
            imgRender();
        }
        renderCall = false;

        // Routine for checking and writing out metadata
        checkMeta();

        // Routine for updating thumbnails
        rollRenderCheck();

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

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        //SDL_SetRenderDrawColor(renderer, (Uint8)(windowCL.x * 255), (Uint8)(windowCL.y * 255), (Uint8)(windowCL.z * 255), (Uint8)(windowCL.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return returnValue;
}

void mainWindow::actionA() {
    LOG_INFO("Menu Button Press");
}
