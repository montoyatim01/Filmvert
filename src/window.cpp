#include "window.h"
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <cstring>
#include <imgui.h>
#include <stdlib.h>
#include <cmrc/cmrc.hpp>  //read embedded stuff
#include <string>
#include <thread>

#include "fmt/format.h"
#include "imageIO.h"
#include "imgui_impl_sdl2.h"
#include "logger.h"
#include "portable-file-dialogs.h"

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

void imguistyle()
{
  //Roboto-Regular.ttf
  //ImGui::GetStyle().FrameRounding = 6.0f;
    //ImGui::GetStyle().GrabRounding = 6.0f;

    ImGuiStyle * style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(10, 10);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(8, 4);
	style->FrameRounding = 6.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(22.0f/255.0f, 22.0f/255.0f, 86.0f/255.0f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(44.0f/255.0f, 44.0f/255.0f, 62.0f/255.0, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(40.0f/255.0f, 40.0f/255.0f, 209.0f/255.0, 1.00f);

    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(44.0f/255.0f, 44.0f/255.0f, 62.0f/255.0f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(40.0f/255.0f, 40.0f/255.0f, 209.0f/255.0f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(44.0f/255.0f, 33.0f/255.0f, 86.0f/255.0f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);

    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
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
    SDL_Window* window = SDL_CreateWindow("tGrain", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 1000, window_flags);
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
    auto regularFont = fs.open("assets/fonts/Roboto-Regular.ttf");
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

    ImFont* ft_text = io.Fonts->AddFontFromMemoryTTF((void*)(regularFont->begin()), int(regularFont->size()), 18.0f, &font_cfg);
    ImFont* ft_header = io.Fonts->AddFontFromMemoryTTF((void*)(boldFont->begin()), int(boldFont->size()), 20.0f, &font_cfg);

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



    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);



    // Main loop
    ImVec4 windowCL = ImVec4(22.0f/255.0f, 22.0f/255.0f, 29.0f/255.0f, 1.00f);
    bool done = false;
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

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("Open Image(s)")) {
                            openImages();
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Export Selected")) {
                            exportPopup = true;
                        }
                        if (ImGui::MenuItem("Exit")) {
                            done = true;
                        }

                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
        }

        bool calcBaseColor = false;

        /* DISPLAY WIN */
        ImGui::SetNextWindowPos(ImVec2(0,25));
        ImGui::SetNextWindowSize(ImVec2(winWidth * 0.65,winHeight - (25 + 240)));
        ImGui::Begin("Image Display", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (selIm >= 0) {
            // Pre-calc
            dispSize = ImVec2(dispScale * activeImages[selIm].width,
                            dispScale * activeImages[selIm].height);

            cursorPos.x = (ImGui::GetWindowSize().x - dispSize.x) * 0.5f;
            cursorPos.y = (ImGui::GetWindowSize().y - dispSize.y) * 0.5f;
            if (cursorPos.x < 0)
                cursorPos.x = 0;
            if (cursorPos.y < 0)
                cursorPos.y = 0;

            ImGui::SetCursorPos(cursorPos);

            // Store the current position for later reference
            ImVec2 imagePos = ImVec2(
                ImGui::GetWindowPos().x + cursorPos.x - ImGui::GetScrollX(),
                ImGui::GetWindowPos().y + cursorPos.y - ImGui::GetScrollY()
            );

            ImGui::Image(
                reinterpret_cast<ImTextureID>(activeImages[selIm].texture),
                dispSize);

            ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
            ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);

            scroll.x = ImGui::GetScrollX();
            scroll.y = ImGui::GetScrollY();

            ImGuiWindow *win = ImGui::GetCurrentWindow();
            bool currentlyInteracting = false;

            // Variables for drag handling
            static int draggedCorner = -1;
            static bool dragging = false;

            // Convert crop box coordinates to screen space
            ImVec2 cropBoxScreen[4];
            for (int i = 0; i < 4; i++) {
                // Calculate screen positions with proper scroll offset
                cropBoxScreen[i].x = imagePos.x + activeImages[selIm].cropBoxX[i] * dispScale;
                cropBoxScreen[i].y = imagePos.y + activeImages[selIm].cropBoxY[i] * dispScale;

                // Log coordinates for debugging
                //printf("Corner %d: Image coords (%i, %i), Screen coords (%.1f, %.1f)\n",
                       //i, activeImages[selIm].cropBoxX[i], activeImages[selIm].cropBoxY[i],
                       //cropBoxScreen[i].x, cropBoxScreen[i].y);
            }

            // Draw the crop box lines
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImU32 lineColor = IM_COL32(255, 255, 0, 255); // Yellow
            float lineThickness = 2.0f;

            // Draw the lines connecting the corners - connect in order: top-left, top-right, bottom-right, bottom-left
            if (cropDisplay) {
                drawList->AddLine(cropBoxScreen[0], cropBoxScreen[1], lineColor, lineThickness); // Top line
                drawList->AddLine(cropBoxScreen[1], cropBoxScreen[2], lineColor, lineThickness); // Right line
                drawList->AddLine(cropBoxScreen[2], cropBoxScreen[3], lineColor, lineThickness); // Bottom line
                drawList->AddLine(cropBoxScreen[3], cropBoxScreen[0], lineColor, lineThickness); // Left line
            }


            // Debug drawing to verify corner positions
            char cornerText[32];
            for (int i = 0; i < 4; i++) {
                sprintf(cornerText, "C%d", i);
                if (cropDisplay) {
                    drawList->AddText(cropBoxScreen[i], IM_COL32(255, 255, 255, 255), cornerText);
                }
            }

            // Draw the corner handles
            float handleRadius = 8.0f;
            ImU32 handleColor = IM_COL32(255, 0, 0, 255); // Red
            ImU32 handleHoverColor = IM_COL32(255, 128, 0, 255); // Orange

            for (int i = 0; i < 4; i++) {
                // Check if mouse is hovering over this handle
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                float distSq = (mousePos.x - cropBoxScreen[i].x) * (mousePos.x - cropBoxScreen[i].x) +
                               (mousePos.y - cropBoxScreen[i].y) * (mousePos.y - cropBoxScreen[i].y);
                bool handleHovered = distSq <= (handleRadius * handleRadius);

                // Draw the handle with appropriate color
                if (cropDisplay) {
                    drawList->AddCircleFilled(
                        cropBoxScreen[i],
                        handleRadius,
                        handleHovered ? handleHoverColor : handleColor
                    );
                }


                // Handle dragging
                if (handleHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !dragging) {
                    draggedCorner = i;
                    dragging = true;
                    currentlyInteracting = true;
                }
            }
            // Draw min/max positions if available
            if (minMaxDisp) {
                if (validIm()){
                    if (activeImages[selIm].minX != 0 && activeImages[selIm].maxY != 0) {
                        ImVec2 minPoint;
                        minPoint.x = imagePos.x + (float)activeImages[selIm].minX * dispScale;
                        minPoint.y = imagePos.y + (float)activeImages[selIm].minY * dispScale;
                        ImVec2 maxPoint;
                        maxPoint.x = imagePos.x + (float)activeImages[selIm].maxX * dispScale;
                        maxPoint.y = imagePos.y + (float)activeImages[selIm].maxY * dispScale;
                        float pointRadius = 8.0f;
                        ImU32 handleColor = IM_COL32(255, 0, 0, 255);
                        char minText[4] = "Min";
                        char maxText[4] = "Max";
                        drawList->AddText(minPoint, IM_COL32(255, 255, 255, 255), minText);
                        drawList->AddText(maxPoint, IM_COL32(255, 255, 255, 255), maxText);
                        drawList->AddCircleFilled(minPoint, pointRadius, handleColor);
                        drawList->AddCircleFilled(maxPoint, pointRadius, handleColor);
                    }
                }
            }

            // If dragging a corner, update its position
            if (dragging && draggedCorner >= 0) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    // Calculate the new position in image space
                    ImVec2 newPosImage;
                    newPosImage.x = (ImGui::GetIO().MousePos.x - imagePos.x) / dispScale;
                    newPosImage.y = (ImGui::GetIO().MousePos.y - imagePos.y) / dispScale;

                    // Constrain to image boundaries
                    newPosImage.x = ImClamp(newPosImage.x, 0.0f, (float)activeImages[selIm].width);
                    newPosImage.y = ImClamp(newPosImage.y, 0.0f, (float)activeImages[selIm].height);

                    // Update the corner position
                    activeImages[selIm].cropBoxX[draggedCorner] = newPosImage.x;
                    activeImages[selIm].cropBoxY[draggedCorner] = newPosImage.y;

                    currentlyInteracting = true;
                } else {
                    // Mouse released, stop dragging
                    dragging = false;
                    draggedCorner = -1;
                }
            }

            // RECTANGULAR BOX SELECTION IMPLEMENTATION
            // Variables for rectangle selection
            static bool isSelecting = false;
            static ImVec2 selectionStart;
            static ImVec2 selectionEnd;

            // Check if Ctrl+Shift is pressed
            bool ctrlShiftPressed = ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift;

            // Handle rectangle selection
            if (ImGui::IsItemHovered()) {
                // Calculate mouse position in image coordinates
                ImVec2 mousePosInImage;
                mousePosInImage.x = (ImGui::GetIO().MousePos.x - imagePos.x) / dispScale;
                mousePosInImage.y = (ImGui::GetIO().MousePos.y - imagePos.y) / dispScale;

                // Start selection when mouse is clicked while holding Ctrl+Shift
                if (ctrlShiftPressed && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isSelecting && !dragging) {
                    isSelecting = true;
                    selectionStart = mousePosInImage;
                    // Clamp to image boundaries
                    selectionStart.x = ImClamp(selectionStart.x, 0.0f, (float)activeImages[selIm].width);
                    selectionStart.y = ImClamp(selectionStart.y, 0.0f, (float)activeImages[selIm].height);

                    // Initialize end position as same as start
                    selectionEnd = selectionStart;

                    // Store in sample arrays
                    activeImages[selIm].sampleX[0] = selectionStart.x;
                    activeImages[selIm].sampleY[0] = selectionStart.y;
                    activeImages[selIm].sampleX[1] = selectionEnd.x;
                    activeImages[selIm].sampleY[1] = selectionEnd.y;

                    currentlyInteracting = true;
                }

                // Update selection while dragging
                if (isSelecting && ctrlShiftPressed && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    selectionEnd = mousePosInImage;
                    // Clamp to image boundaries
                    selectionEnd.x = ImClamp(selectionEnd.x, 0.0f, (float)activeImages[selIm].width);
                    selectionEnd.y = ImClamp(selectionEnd.y, 0.0f, (float)activeImages[selIm].height);

                    // Store in sample arrays
                    activeImages[selIm].sampleX[1] = selectionEnd.x;
                    activeImages[selIm].sampleY[1] = selectionEnd.y;

                    currentlyInteracting = true;
                }

                // End selection when mouse is released
                if (isSelecting && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    isSelecting = false;
                    // Final update of sample arrays
                    activeImages[selIm].sampleX[0] = selectionStart.x;
                    activeImages[selIm].sampleY[0] = selectionStart.y;
                    activeImages[selIm].sampleX[1] = selectionEnd.x;
                    activeImages[selIm].sampleY[1] = selectionEnd.y;

                    calcBaseColor = true;

                    //printf("Rectangle selection: (%.1f, %.1f) to (%.1f, %.1f)\n",
                      //     sampleX[0], sampleY[0], sampleX[1], sampleY[1]);

                    currentlyInteracting = true;
                }
            }

            // Draw the selection rectangle if actively selecting
            if (validIm()){
                if ((isSelecting && ctrlShiftPressed) || activeImages[selIm].sampleVisible) {
                    // Convert image coordinates to screen coordinates
                    ImVec2 selStartScreen, selEndScreen;
                    selStartScreen.x = imagePos.x + activeImages[selIm].sampleX[0] * dispScale;
                    selStartScreen.y = imagePos.y + activeImages[selIm].sampleY[0] * dispScale;
                    selEndScreen.x = imagePos.x + activeImages[selIm].sampleX[1] * dispScale;
                    selEndScreen.y = imagePos.y + activeImages[selIm].sampleY[1] * dispScale;

                    // Draw selection rectangle
                    ImU32 selectionColor = IM_COL32(0, 255, 255, 128); // Cyan with transparency
                    ImU32 selectionBorderColor = IM_COL32(0, 255, 255, 255); // Solid cyan for border

                    drawList->AddRectFilled(selStartScreen, selEndScreen, selectionColor);
                    drawList->AddRect(selStartScreen, selEndScreen, selectionBorderColor, 0.0f, 0, 2.0f);
                }
            }


            // Display the current selection coordinates if a selection exists
            /*if (sampleX[0] != sampleX[1] || sampleY[0] != sampleY[1]) {
                ImVec2 textPos = ImVec2(10, 10); // Position in top-left of window
                char selText[128];
                sprintf(selText, "Selection: (%.1f, %.1f) to (%.1f, %.1f)",
                        sampleX[0], sampleY[0], sampleX[1], sampleY[1]);
                drawList->AddText(ImGui::GetWindowPos() + textPos, IM_COL32(255, 255, 255, 255), selText);
            }*/

            // Controls for panning and zooming the image (your existing code)
            if (ImGui::IsItemHovered()) {
                ImVec2 mousePosInImage;
                mousePosInImage.x =
                    (ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x -
                     cursorPos.x + ImGui::GetScrollX()) /
                    dispScale;
                mousePosInImage.y =
                    (ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y -
                     cursorPos.y + ImGui::GetScrollY()) /
                    dispScale;

                // Inside the mouse wheel condition:
                if (ImGui::GetIO().MouseWheel != 0 && ImGui::GetIO().KeyShift) {
                    // Store the mouse position relative to the image before zooming
                    float mouseXRatio = mousePosInImage.x / activeImages[selIm].width;
                    float mouseYRatio = mousePosInImage.y / activeImages[selIm].height;

                    // Adjust scale factor
                    float prevScale = dispScale;
                    dispScale = dispScale * pow(1.05f, ImGui::GetIO().MouseWheel);

                    // Clamp the scale
                    if (dispScale < 0.1f) dispScale = 0.1f;
                    if (dispScale > 30.0f) dispScale = 30.0f;

                    // Calculate new display size
                    dispSize = ImVec2(dispScale * activeImages[selIm].width,
                                      dispScale * activeImages[selIm].height);

                    // Recalculate cursor position correctly
                    cursorPos.x = (ImGui::GetWindowSize().x - dispSize.x) * 0.5f;
                    cursorPos.y = (ImGui::GetWindowSize().y - dispSize.y) * 0.5f;

                    if (cursorPos.x < 0) cursorPos.x = 0;
                    if (cursorPos.y < 0) cursorPos.y = 0;

                    // Calculate new scroll position to keep mouse point fixed during zoom
                    float newMouseImageX = mouseXRatio * activeImages[selIm].width;
                    float newMouseImageY = mouseYRatio * activeImages[selIm].height;

                    scroll.x = (newMouseImageX * dispScale) - (ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x) + cursorPos.x;
                    scroll.y = (newMouseImageY * dispScale) - (ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y) + cursorPos.y;

                    ImGui::SetScrollX(scroll.x);
                    ImGui::SetScrollY(scroll.y);
                    currentlyInteracting = true;
                }

                if ((ImGui::GetIO().MouseWheel != 0 || ImGui::GetIO().MouseWheelH != 0) && !ImGui::GetIO().KeyShift) {
                    scroll.x = ImGui::GetScrollX() - (ImGui::GetIO().MouseWheelH * 12);
                    scroll.y = ImGui::GetScrollY() - (ImGui::GetIO().MouseWheel * 12);
                    ImGui::SetScrollX(scroll.x);
                    ImGui::SetScrollY(scroll.y);
                    currentlyInteracting = true;
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                    // Pressed the z key, reset zoom
                    float scaleX = ImGui::GetWindowSize().x / (activeImages[selIm].width + ((float)activeImages[selIm].width * 0.1f));
                    float scaleY = ImGui::GetWindowSize().y / (activeImages[selIm].height + ((float)activeImages[selIm].height * 0.1f));

                    dispScale = scaleX > scaleY ? scaleY : scaleX;
                    currentlyInteracting = true;
                }

                // Only allow panning if we're not dragging a corner and not making a selection
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && !dragging && !isSelecting) {
                    scroll.x = ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x;
                    scroll.y = ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y;
                    ImGui::SetScrollX(scroll.x);
                    ImGui::SetScrollY(scroll.y);
                    currentlyInteracting = true;
                }
            }

            if (currentlyInteracting) {
                interactionTimer = 0.0f;
                isInteracting = true;
            } else if (isInteracting) {
                interactionTimer += ImGui::GetIO().DeltaTime;
                if (interactionTimer > INTERACTION_TIMEOUT) {
                    renderCall = true;
                    isInteracting = false;
                    interactCall = true;
                }
            }
            calculateVisible();
        }

        ImGui::End();

        /* CONTROLS WIN */
        ImGui::SetNextWindowPos(ImVec2(winWidth * 0.65,25));
        ImGui::SetNextWindowSize(ImVec2(winWidth * 0.35,winHeight - (25 + 240)));
        ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        {
            /* PRESETS */
            if (ImGui::Button("Load Preset")) {
                loadPreset();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Preset")) {
                if (activeImages.size() > 0 && selIm >= 0 && selIm < activeImages.size()) {
                    //activeImages[selIm].writeEXR();
                    savePreset();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Toggle Selection")) {
                if (validIm()) {
                    activeImages[selIm].sampleVisible = !activeImages[selIm].sampleVisible;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                if (validIm()) {
                    renderCall = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Image")) {
                if (validIm()) {
                    saveImage();
                }
            }
            ImGui::Separator();

            // Base color (presets/averages?)
            // Analysis (how many options?)
            // -Blur setting
            // -Crop display (widget for points?)
            // -Button to analyze
            // -Black point (display?)
            // -White point (display?)
            //
            // Grade (Use 4-color picker, the 4th color is all colors (maintain differences?))
            // -BP/WP
            // -Lift
            // -Gamma
            // -Gain
            // -Offset
            // -Multiply
            //
            // Color pipeline
            // -Raw image comes in as linear AP0 (convert to AP1 in store to float buf)
            // -Two image kernels. One just for blur, the other for image processing
            // -On analysis run the blur kernel, then run the min/max finder based on the crop selection
            // -Always process the output image with the OCIO Transform (renders in-place)
            //
            // Raw image buffer, and display buffer, nothing else
            // Upon image selection, the kernel is run on a single temp buffer (available to all images), then processed to the display buffer
            //
            // Upon rendering, the images are run through the same process.
            //
            // For the analysis, blur the image, copy the image back, and the run the min/max using tiles. Split the image horizontally
            // Based on the number of threads available, join those threads, and then run one last operation to determine the true min/max (use a struct)
            //
            // Set the black/white point based on that
            //
            // Debayer image to half res (for whole pipeline)
            // Thumbnail image buffer 640x480?
            //  -Processed on load in for each image
            // Single active image buffer
            // Single blur image buffer
            // Upon image selection, image is debayered (half-res?) and
            // sent to image buf.
            //
            // How does the multi-blur buf work?
            if (validIm()) {

            }

            /* INVERSION PARAMS */
            if (validIm()) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode("Analysis")) {
                    ImGui::ColorEdit3("Base Color", (float*)activeImages[selIm].baseColor, ImGuiColorEditFlags_Float);
                    ImGui::SliderFloat("Analysis Blur", &activeImages[selIm].blurAmount, 0.5f, 20.0f);
                    ImGui::Checkbox("Display Analysis Region", &cropDisplay);
                    ImGui::Separator();
                    if (ImGui::Button("Analyze")) {
                        analyzeImage();
                        cropDisplay = false;
                        minMaxDisp = true;
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("Display Min/Max Points", &minMaxDisp);
                    renderCall |= ImGui::ColorEdit4("Black Point", (float*)activeImages[selIm].blackPoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("White Point", (float*)activeImages[selIm].whitePoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    ImGui::TreePop();
                }
            }



            /* GRADE PARAMS */
            if (validIm()) {
                if (ImGui::TreeNode("Grade")) {
                    renderCall |= ImGui::SliderFloat("Temperature", &activeImages[selIm].temp, -1.0f, 1.0f);
                    renderCall |= ImGui::SliderFloat("Tint", &activeImages[selIm].tint, -1.0f, 1.0f);
                    renderCall |= ImGui::ColorEdit4("Black Point", (float*)activeImages[selIm].g_blackpoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("White Point", (float*)activeImages[selIm].g_whitepoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("Lift", (float*)activeImages[selIm].g_lift, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("Gain", (float*)activeImages[selIm].g_gain, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("Multiply", (float*)activeImages[selIm].g_mult, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("Offset", (float*)activeImages[selIm].g_offset, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    renderCall |= ImGui::ColorEdit4("Gamma", (float*)activeImages[selIm].g_gamma, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                    //renderCall |= ImGui::SliderFloat("Sigma Filter", activeImages.size() > 0 && selIm >= 0 && selIm < activeImages.size() ? &activeImages[selIm].grain.grainSigma : &grain.grainSigma, 0.1f, 4.0f, "%.3f");
                    ImGui::TreePop();
                }
            }

            if (validIm()) {
                renderCall |= ImGui::Checkbox("Bypass Render", &activeImages[selIm].renderBypass);
            }



            ImGui::Text("FPS: %04f, Time: %04fms", mtlGPU->rdTimer.fps, mtlGPU->rdTimer.renderTime);
            ImVec2 winSize = ImGui::GetWindowSize();
            ImGui::SetCursorPosY(winSize.y - 30);
            if (isRendering)
                ImGui::Text("Rendering...");
        }

        if (renderCall && !isRendering && !isExporting) {
            imgRender();
            if (validIm()) {
                //activeImages[selIm].grain.formatMeta();
            }
        }



        ImGui::End();

        /* THUMBNAILS WIN */
        ImGui::SetNextWindowPos(ImVec2(0,winHeight - 240));
        ImGui::SetNextWindowSize(ImVec2(winWidth,240));
        ImGui::Begin("Thumbnails", 0, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        {
            //SDL_GetWindowSize(window, &winWidth, &winHeight);

            //Multi-Select
            ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, activeImages.size());
            selection.ApplyRequests(ms_io);

            for (int i = 0; i < activeImages.size(); i++) {
                if (!activeImages[i].texture) {
                    updateSDLTexture(&activeImages[i]);
                }
                if (activeImages[i].sdlUpdate) {
                    updateSDLTexture(&activeImages[i]);
                }

                    ImVec2 maxAvailable = {320,240};
                    ImVec2 displaySize;
                    ImVec2 elGuideSize;

                    // Example image dimensions (replace with actual)
                    CalculateDisplaySize(activeImages[i].width, activeImages[i].height, maxAvailable, displaySize);

                    auto pos = ImGui::GetCursorPos();
                    ImGui::PushID(i);
                    //Multi-select
                    bool item_is_selected = selection.Contains((ImGuiID)i);
                    activeImages[i].selected = item_is_selected;
                    ImGui::SetNextItemSelectionUserData(i);
                    if (ImGui::Selectable("###", activeImages[i].selected, 0, displaySize)) {
                        if (!ImGui::GetIO().KeySuper) {
                            selIm = i;
                            //clearSelection();
                            // Clear selection when CTRL is not held
                        }
                        //activeImages[i].selected ^= 1; // Toggle current item
                    }
                    ImGui::SetItemAllowOverlap();
                    ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
                    ImGui::Image(reinterpret_cast<ImTextureID>(activeImages[i].texture), displaySize);
                    ImGui::PopID();
                    ImGui::SameLine();
            }
            ms_io = ImGui::EndMultiSelect();
            selection.ApplyRequests(ms_io);

            // Copy + Paste
            if (ImGui::IsKeyChordPressed(ImGuiKey_C | ImGuiMod_Ctrl)) {
                // Copy
                LOG_INFO("Copy from: {}", selIm);
                if (validIm()) {
                    copyIntoParams();
                }
            }
            if (ImGui::IsKeyChordPressed(ImGuiKey_V | ImGuiMod_Ctrl)) {
                // Paste
                pasteTrigger = true;
                if (!isRendering && !isExporting) {
                    //imgRender();
                }
            }
        }
        ImGui::End();


        if (calcBaseColor) {
            if (validIm()) {
                activeImages[selIm].processBaseColor();
            }
            calcBaseColor = true;
        }

        batchRenderPopup();
        pastePopup();

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

// Function to calculate display dimensions while maintaining aspect ratio
void mainWindow::CalculateDisplaySize(int imageWidth, int imageHeight, ImVec2 maxAvailable, ImVec2& outDisplaySize) {
    float aspectRatio = (float)imageWidth / imageHeight;

    // Calculate possible sizes based on width and height constraints
    outDisplaySize.x = maxAvailable.y * aspectRatio;
    if (outDisplaySize.x > maxAvailable.x)
        outDisplaySize.x = maxAvailable.x;

    outDisplaySize.y = maxAvailable.x / aspectRatio;
    if (outDisplaySize.y > maxAvailable.y)
        outDisplaySize.y = maxAvailable.y;

    // Ensure the display size does not exceed the available space
    outDisplaySize.x = std::min(outDisplaySize.x, (float)maxAvailable.x);
    outDisplaySize.y = std::min(outDisplaySize.y, (float)maxAvailable.y);
}

void mainWindow::calculateVisible() {
    // Calculate visible region of the image
    ImVec2 visibleTopLeft, visibleBottomRight;

    // Get the actual position of the image on screen (accounting for scroll and cursor position)
    ImVec2 imageScreenPos;
    imageScreenPos.x = ImGui::GetWindowPos().x + cursorPos.x - scroll.x;
    imageScreenPos.y = ImGui::GetWindowPos().y + cursorPos.y - scroll.y;

    // Get window visible boundaries in screen space
    ImVec2 windowTopLeft = ImGui::GetWindowPos();
    ImVec2 windowBottomRight = ImVec2(
        windowTopLeft.x + ImGui::GetWindowSize().x,
        windowTopLeft.y + ImGui::GetWindowSize().y
    );

    // Calculate the visible portion of the image in screen space
    ImVec2 visibleScreenTopLeft, visibleScreenBottomRight;
    visibleScreenTopLeft.x = std::max(windowTopLeft.x, imageScreenPos.x);
    visibleScreenTopLeft.y = std::max(windowTopLeft.y, imageScreenPos.y);
    visibleScreenBottomRight.x = std::min(windowBottomRight.x, imageScreenPos.x + dispSize.x);
    visibleScreenBottomRight.y = std::min(windowBottomRight.y, imageScreenPos.y + dispSize.y);

    // Convert from screen space to image space
    visibleTopLeft.x = (visibleScreenTopLeft.x - imageScreenPos.x) / dispScale;
    visibleTopLeft.y = (visibleScreenTopLeft.y - imageScreenPos.y) / dispScale;
    visibleBottomRight.x = (visibleScreenBottomRight.x - imageScreenPos.x) / dispScale;
    visibleBottomRight.y = (visibleScreenBottomRight.y - imageScreenPos.y) / dispScale;

    // Add some padding to the image for scaling
    visibleTopLeft.x -= 4;
    visibleTopLeft.y -= 4;
    visibleBottomRight.x += 4;
    visibleBottomRight.y += 4;

    // Clamp to actual image dimensions

}

void mainWindow::openImages() {
    auto selection = pfd::open_file("Select a file", "/Users/timothymontoya/Desktop/CLAi_OFX/2/3/4/5",
                                    { "Image Files", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.exr *.dpx",
                                      "All Files", "*" },
                                    pfd::opt::multiselect).result();
    // Do something with selection
    int activePos = activeImages.size();
    std::thread renThread = std::thread{ [this, selection, activePos]() {
        for (auto const &filename : selection) {
            LOG_INFO("Selected file: {}", filename);
            bool success = false;
            try {
                activeImages.emplace_back(
                    std::get<image>(readImage(filename)) );
                success = true;
            } catch (const std::bad_variant_access& ex) {
                LOG_ERROR("Error processing image, trying libraw");
            }

            try {
                activeImages.emplace_back(
                    std::get<image>(readRawImage(filename)) );
                success = true;
            } catch (const std::bad_variant_access& ex) {
                LOG_ERROR("Error processing image");

            }
            if (!success)
                continue;
                imgRender(&activeImages[activeImages.size() - 1]);

        }
        //if (activePos != activeImages.size()) {
        //    initRender(activePos, activeImages.size());
        //}
    }};
    renThread.detach();


}
void mainWindow::saveImage() {
    auto destination = pfd::save_file("Select a file", ".",
                                      {"All Files", "*" },
                                      pfd::opt::force_overwrite).result();
    // Do something with destination
    if (!destination.empty()) {
        if (validIm()) {
            // Re-render the image first
            if (!isRendering) {
                isRendering = true;
                mtlGPU->renderImage(&activeImages[selIm]);
                activeImages[selIm].procDispImg();
                activeImages[selIm].sdlUpdate = true;
                isRendering = false;
            }
            exportPopup = true;

        }
    }
}

void mainWindow::openDirectory() {
    auto selection = pfd::select_folder("Select a destination").result();
    if (!selection.empty())
        outPath = selection;
}

void mainWindow::loadPreset() {
    auto selection = pfd::open_file("Select a file", "/Users/timothymontoya/Desktop/CLAi_OFX/2/3/4/5",
                                    { "tGrain Preset", "*.tgrain",
                                      "Image Files", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.exr *.dpx",
                                      "All Files", "*" },
                                    pfd::opt::none).result();
    if (!selection.empty()) {
        std::cout << "User selected file " << selection[0] << "\n";
        if (validIm()) {

        }
    }

}
void mainWindow::savePreset() {
    auto destination = pfd::save_file("Select a file", ".",
                                      { "tGrain Preset", "*.tgrain",
                                        "All Files", "*" },
                                      pfd::opt::force_overwrite).result();
    // Do something with destination
    if (!destination.empty()) {
        if (validIm()) {

        }
    }

}

void mainWindow::clearSelection() {
    for (int i = 0; i < activeImages.size(); i++) {
        activeImages[i].selected = false;
    }
}

bool mainWindow::validIm() {
    return activeImages.size() > 0 && selIm >= 0 && selIm < activeImages.size();
}

void mainWindow::createSDLTexture(image* actImage) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, actImage->width, actImage->height);
    if (texture)
      actImage->texture = texture;
    else
      LOG_WARN("Unable to create SDL texture for image: {}", actImage->srcFilename);
}

void mainWindow::updateSDLTexture(image* actImage) {
    if (actImage->texture == nullptr) {
        createSDLTexture(actImage);
    }

    void* pixelData = nullptr;
    int pitch = 0;
    if (SDL_LockTexture((SDL_Texture*)actImage->texture, nullptr, &pixelData, &pitch) != 0) {
        LOG_ERROR("Unable to lock SDL texture for image: {}", actImage->srcFilename);
        return;
    }
    const int height = actImage->height;
    const int rowBytes = actImage->width * 4; // Assuming RGBA with 4 bytes per pixel

    for (int row = 0; row < height; ++row) {
        const void* srcRow = static_cast<const uint8_t*>(actImage->dispImgData) + (row * rowBytes);
        void* dstRow = static_cast<uint8_t*>(pixelData) + (row * pitch);
        memcpy(dstRow, srcRow, rowBytes);
    }

    SDL_UnlockTexture((SDL_Texture*)actImage->texture);
    actImage->sdlUpdate = false;

    return;
}

void mainWindow::initRender(int start, int end) {
    std::thread renThread = std::thread{ [this, start, end]() {
        isRendering = true;
        for (int i = start; i < end; i++) {
            if (i >= activeImages.size())
                break;
            mtlGPU->renderImage(&activeImages[i]);
            activeImages[i].procDispImg();
            activeImages[i].sdlUpdate = true;
            //updateSDLTexture(&activeImages[i]);
        }
        isRendering = false;
    }};
    renThread.detach();

}

void mainWindow::imgRender() {
    if (isRendering)
        return;

    if (activeImages.size() > 0 && selIm >= 0 && selIm < activeImages.size()) {

        std::thread renThread = std::thread{ [this]() {
            isRendering = true;
            mtlGPU->renderImage(&activeImages[selIm]);
            activeImages[selIm].procDispImg();
            activeImages[selIm].sdlUpdate = true;
            //updateSDLTexture(&activeImages[selIm]);
            isRendering = false;
            renderCall = false;
	} };
        renThread.detach();

    }
}

void mainWindow::imgRender(image *img) {
    if (isRendering)
        return;

    if (img) {
        isRendering = true;
        mtlGPU->renderImage(img);
        img->procDispImg();
        img->sdlUpdate = true;

        isRendering = false;
        renderCall = false;
	}
}

void mainWindow::analyzeImage() {
    if (isRendering)
        return;
    if (validIm()) {
        LOG_INFO("Analyzing");
        isRendering = true;
        if (!activeImages[selIm].blurImgData)
            activeImages[selIm].allocBlurBuf();
        mtlGPU->renderBlurPass(&activeImages[selIm]);
        activeImages[selIm].processMinMax();
        activeImages[selIm].procDispImg();
        activeImages[selIm].sdlUpdate = true;
        activeImages[selIm].delBlurBuf();
        activeImages[selIm].renderBypass = false;
        isRendering = false;
        imgRender();
    }
}

void mainWindow::batchRenderPopup() {
    if (exportPopup)
        ImGui::OpenPopup("ExportWindow");
    if (ImGui::BeginPopupModal("ExportWindow", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Export selected images");
        ImGui::Separator();

        ImGui::Text("File Format:");
        ImGui::Combo("###FF", &outType, fileTypes.data(), fileTypes.size());
        ImGui::Text("Bit-Depth:");
        ImGui::Combo("###BD", &outDepth, bitDepths.data(), bitDepths.size());
        ImGui::SliderInt("Quality", &quality, 10, 100);
        ImGui::Checkbox("Overwrite Existing File(s)?", &overwrite);

        // Output Directory
        static char buf1[256] = "";
        ImGui::InputTextWithHint("###Path", "Save Path", buf1, IM_ARRAYSIZE(buf1));
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            openDirectory();
            strcpy(buf1, outPath.c_str());
        }
        bool disableSet = false;
        if (isExporting) {
            ImGui::BeginDisabled();
            disableSet = true;
        }

        if(ImGui::Button("Save")) {
            isExporting = true;
            exportParam params;
            params.outPath = buf1;
            params.format = outType;
            params.bitDepth = outDepth;
            params.quality = quality;
            params.overwrite = overwrite;
            elapsedTime = 0;
            numIm = 0;
            for (int i=0; i < activeImages.size(); i++) {
                if (activeImages[i].selected)
                    numIm++;
            }
            curIm = 0;
            exportThread = std::thread{[this, params]() {
                for (int i = 0; i < activeImages.size(); i++) {
                    if (!isExporting) {
                        exportPopup = false;
                        return;
                    }

                    if (activeImages[i].selected) {
                        auto start = std::chrono::steady_clock::now();
                        //TODO: Figure out better solution for this (non-blocking)
                        // but still will render out an image.
                        if (!isRendering) {
                            isRendering = true;
                            mtlGPU->renderImage(&activeImages[i]);
                            activeImages[i].procDispImg();
                            activeImages[i].sdlUpdate = true;
                            isRendering = false;
                        }
                        LOG_INFO("Exporting Image {}: {}", i, activeImages[i].srcFilename);
                        activeImages[i].writeImg(params);
                        auto end = std::chrono::steady_clock::now();
                        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                        elapsedTime += dur.count();
                        curIm++;
                    }

                }
                exportPopup = false;
                isExporting = false;
            }};
            exportThread.detach();
            //exportPopup = false;
            //ImGui::CloseCurrentPopup();
        }
        if (disableSet)
            ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            isExporting = false;
            exportPopup = false;
            ImGui::CloseCurrentPopup();
        }
        if (isExporting) {
            ImGui::Separator();
            float progress = (float)curIm / ((float)(numIm-1) + 0.5f);
            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
            unsigned int avgTime = 0;
            if (curIm !=0)
                avgTime = elapsedTime / curIm;
            unsigned int remainingTime = (numIm - (curIm + 1)) * avgTime;
            unsigned int remainingSec = remainingTime / 1000;
            unsigned int hr = remainingSec / 3600;
            remainingSec %= 3600;
            unsigned int min = remainingSec / 60;
            remainingSec %= 60;
            std::string remMsg = "";
            remMsg = fmt::format("{:3} of {:3} Images Processed. {:02}:{:02}:{:02} Remaining",
                curIm, numIm, hr, min, remainingSec);
            ImGui::Text(remMsg.c_str());
        }
        if (!exportPopup && !isExporting)
            ImGui::CloseCurrentPopup();



        ImGui::EndPopup();
    }
}

void mainWindow::pastePopup() {
    if (pasteTrigger)
        ImGui::OpenPopup("PasteWindow");
    if (ImGui::BeginPopupModal("PasteWindow", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Paste selected options");
        ImGui::Separator();
        if(ImGui::Button("All Analysis")) {
            pasteOptions.analysisGlobal();
        }
        ImGui::Checkbox("Base Color", &pasteOptions.baseColor);
        ImGui::SameLine();
        ImGui::Checkbox("Crop Points", &pasteOptions.cropPoints);

        ImGui::Checkbox("Analysis Blur", &pasteOptions.analysisBlur);
        ImGui::SameLine();
        ImGui::Checkbox("Analysis", &pasteOptions.analysis);

        ImGui::Separator();
        if (ImGui::Button("All Grade")) {
            pasteOptions.gradeGlobal();
        }
        ImGui::Checkbox("Temperature", &pasteOptions.temp);
        ImGui::SameLine();
        ImGui::Checkbox("Tint", &pasteOptions.tint);

        ImGui::Checkbox("Blackpoint", &pasteOptions.bp);
        ImGui::SameLine();
        ImGui::Checkbox("Whitepoint", &pasteOptions.wp);

        ImGui::Checkbox("Lift", &pasteOptions.lift);
        ImGui::SameLine();
        ImGui::Checkbox("Gain", &pasteOptions.gain);

        ImGui::Checkbox("Multiply", &pasteOptions.mult);
        ImGui::SameLine();
        ImGui::Checkbox("Offset", &pasteOptions.offset);

        ImGui::Checkbox("Gamma", &pasteOptions.gamma);

        if(ImGui::Button("Cancel")) {
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Paste")) {
            pasteIntoParams();
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void mainWindow::copyIntoParams() {
    if (validIm()) {
        copyParams.blurAmount = activeImages[selIm].blurAmount;
        copyParams.temp = activeImages[selIm].temp;
        copyParams.tint = activeImages[selIm].tint;
        copyParams.baseColor[0] = activeImages[selIm].baseColor[0];
        copyParams.baseColor[1] = activeImages[selIm].baseColor[1];
        copyParams.baseColor[2] = activeImages[selIm].baseColor[2];
        for (int i = 0; i < 4; i++) {
            copyParams.whitePoint[i] = activeImages[selIm].whitePoint[i];
            copyParams.blackPoint[i] = activeImages[selIm].blackPoint[i];
            copyParams.g_blackpoint[i] = activeImages[selIm].g_blackpoint[i];
            copyParams.g_whitepoint[i] = activeImages[selIm].g_whitepoint[i];
            copyParams.g_lift[i] = activeImages[selIm].g_lift[i];
            copyParams.g_gain[i] = activeImages[selIm].g_gain[i];
            copyParams.g_mult[i] = activeImages[selIm].g_mult[i];
            copyParams.g_offset[i] = activeImages[selIm].g_offset[i];
            copyParams.g_gamma[i] = activeImages[selIm].g_gamma[i];
            copyParams.cropBoxX[i] = activeImages[selIm].cropBoxX[i];
            copyParams.cropBoxY[i] = activeImages[selIm].cropBoxY[i];
        }

    }
}

void mainWindow::pasteIntoParams() {
    for (int i = 0; i < activeImages.size(); i++) {
        if (activeImages[i].selected) {
            activeImages[i].renderBypass = false;
            if (pasteOptions.baseColor) {
                activeImages[i].baseColor[0] = copyParams.baseColor[0];
                activeImages[i].baseColor[1] = copyParams.baseColor[1];
                activeImages[i].baseColor[2] = copyParams.baseColor[2];
            }
            if (pasteOptions.analysisBlur) {
                activeImages[i].blurAmount = copyParams.blurAmount;
            }
            if (pasteOptions.temp) {
                activeImages[i].temp = copyParams.temp;
            }
            if (pasteOptions.tint) {
                activeImages[i].tint = copyParams.tint;
            }
            for (int j = 0; j < 4; j++) {
                if (pasteOptions.cropPoints) {
                    activeImages[i].cropBoxX[j] = copyParams.cropBoxX[j];
                    activeImages[i].cropBoxY[j] = copyParams.cropBoxY[j];
                }
                if (pasteOptions.analysis) {
                    activeImages[i].blackPoint[j] = copyParams.blackPoint[j];
                    activeImages[i].whitePoint[j] = copyParams.whitePoint[j];
                }
                if (pasteOptions.bp) {
                    activeImages[i].g_blackpoint[j] = copyParams.g_blackpoint[j];
                }
                if (pasteOptions.wp) {
                    activeImages[i].g_whitepoint[j] = copyParams.g_whitepoint[j];
                }
                if (pasteOptions.lift) {
                    activeImages[i].g_lift[j] = copyParams.g_lift[j];
                }
                if (pasteOptions.gain) {
                    activeImages[i].g_gain[j] = copyParams.g_gain[j];
                }
                if (pasteOptions.mult) {
                    activeImages[i].g_mult[j] = copyParams.g_mult[j];
                }
                if (pasteOptions.offset) {
                    activeImages[i].g_offset[j] = copyParams.g_offset[j];
                }
                if (pasteOptions.gamma) {
                    activeImages[i].g_gamma[j] = copyParams.g_gamma[j];
                }
            }
            imgRender(&activeImages[i]);
        }
    }
}


void mainWindow::loadMappings()
{
    localMapping["Arri LogC3 | Arri Wide Gamut v3"] = 0;
    localMapping["Arri LogC4 | Arri Wide Gamut v4"] = 1;
    localMapping["Apple Log"] = 2;
    localMapping["Blackmagic Gen 5"] = 3;
    localMapping["Canon Log | Canon Cinema Gamut"] = 4;
    localMapping["Canon Log2 | Canon Cinema Gamut"] = 5;
    localMapping["Canon Log3 | Canon Cinema Gamut"] = 6;
    localMapping["DJI D-Log | D-Gamut"] = 7;
    localMapping["GoPro ProTune Flat"] = 8;
    localMapping["Fuji F-Log | F-Gamut"] = 9;
    localMapping["Leica L-Log | L-Gamut"] = 10;
    localMapping["Nikon N-Log | N-Gamut"] = 11;
    localMapping["Panasonic VLog | VGamut"] = 12;
    localMapping["RED Log3G10 | RedWideGamutRGB"] = 13;
    localMapping["Sony S-Log | S-Gamut"] = 14;
    localMapping["Song S-Log2 | S-Gamut"] = 15;
    localMapping["Sony S-Log3 | S-Gamut3.cine"] = 16;
    localMapping["Sony S-Log3 | S-Gamut3.cine Venice"] = 17;
    localMapping["DaVinci Wide Gamut | DaVinci Intermediate"] = 18;
    localMapping["JPLog2 | AP1"] = 19;
    localMapping["REC 709 | Gamma 2.4"] = 20;
    localMapping["REC 2020 | Gamma 2.4"] = 21;
    localMapping["P3D65 | Gamma 2.6"] = 22;
    localMapping["sRGB | Gamma 2.2"] = 23;
    localMapping["REC 2020 HDR | ST.2084 1000-nit"] = 24;
    localMapping["P3D65 HDR | ST.2084 1000-nit"] = 25;
    localMapping["ACEScct | AP1"] = 26;
    localMapping["ACEScc | AP1"] = 27;
    localMapping["ACEScg | AP1"] = 28;
    localMapping["ACES 2065-1 | AP0"] = 29;



    globalMapping["Arri LogC3 | Arri Wide Gamut v3"] = 100;
    globalMapping["Arri LogC4 | Arri Wide Gamut v4"] = 110;
    globalMapping["Apple Log"] = 116;
    globalMapping["Blackmagic Gen 5"] = 120;
    globalMapping["Canon Log | Canon Cinema Gamut"] = 129;
    globalMapping["Canon Log2 | Canon Cinema Gamut"] = 130;
    globalMapping["Canon Log3 | Canon Cinema Gamut"] = 140;
    globalMapping["DJI D-Log | D-Gamut"] = 150;
    globalMapping["Fuji F-Log | F-Gamut"] = 153;
    globalMapping["Leica L-Log | L-Gamut"] = 155;
    globalMapping["Nikon N-Log | N-Gamut"] = 156;
    globalMapping["Panasonic VLog | VGamut"] = 160;
    globalMapping["RED Log3G10 | RedWideGamutRGB"] = 170;
    globalMapping["Sony S-Log | S-Gamut"] = 178;
    globalMapping["Song S-Log2 | S-Gamut"] = 179;
    globalMapping["Sony S-Log3 | S-Gamut3.cine Venice"] = 180;
    globalMapping["Sony S-Log3 | S-Gamut3.cine"] = 190;
    globalMapping["GoPro ProTune Flat"] = 200;
    globalMapping["DaVinci Wide Gamut | DaVinci Intermediate"] = 210;
    globalMapping["JPLog2 | AP1"] = 215;
    globalMapping["REC 709 | Gamma 2.4"] = 220;
    globalMapping["REC 2020 | Gamma 2.4"] = 230;
    globalMapping["P3D65 | Gamma 2.6"] = 240;
    globalMapping["sRGB | Gamma 2.2"] = 250;
    globalMapping["REC 2020 HDR | ST.2084 1000-nit"] = 260;
    globalMapping["P3D65 HDR | ST.2084 1000-nit"] = 270;
    globalMapping["ACEScct | AP1"] = 280;
    globalMapping["ACEScc | AP1"] = 290;
    globalMapping["ACEScg | AP1"] = 300;
    globalMapping["ACES 2065-1 | AP0"] = 310;

    items.resize(localMapping.size());
    for (int i = 0; i < items.size(); ++i) {
      // Find the key corresponding to the current value
      std::string key = find_key_by_value(localMapping, i);

      // Allocate memory for the char array and copy the string into it
      items[i] = new char[key.length() + 1];
      std::strcpy(items[i], key.c_str());
    }

    fileTypes.push_back("DPX");
    fileTypes.push_back("EXR");
    fileTypes.push_back("JPEG");
    fileTypes.push_back("PNG");
    fileTypes.push_back("TIFF");


    bitDepths.push_back("8");
    bitDepths.push_back("16");
    bitDepths.push_back("32");
}
