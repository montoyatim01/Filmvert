#include "roll.h"
#include "window.h"
#include <filesystem>

//--- ImGui Style ---//
/*
    Set up the styling for the application
*/
void imguistyle()
{
  //Roboto-Regular.ttf
  //ImGui::GetStyle().FrameRounding = 6.0f;
    //ImGui::GetStyle().GrabRounding = 6.0f;

    ImGuiStyle * style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(8, 4);
	style->WindowRounding = 4.0f;
	style->FramePadding = ImVec2(6, 2);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(6, 4);
	style->ItemInnerSpacing = ImVec2(4, 4);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;
return;
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



// Function to calculate display dimensions while maintaining aspect ratio
void mainWindow::CalculateThumbDisplaySize(int imageWidth, int imageHeight, float maxHeight, ImVec2& outDisplaySize, int rotation) {

    bool rot = rotation == 6 || rotation == 8 ? true : false;
    int imWid = rot ? imageHeight : imageWidth;
    int imHei = rot ? imageWidth : imageHeight;

    float scaleRatio = (float)maxHeight / imHei;

    outDisplaySize.x = (float)imWid * scaleRatio;
    outDisplaySize.y = (float)maxHeight;
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

//--- Valid Roll ---//
/*
    Return if the user has a proper roll selected
*/
bool mainWindow::validRoll() {
    return activeRolls.size() > 0 && selRoll >= 0 && selRoll < activeRolls.size();
}

//--- Valid Image ---//
/*
    Return if the user has a proper image selected
*/
bool mainWindow::validIm() {
    return activeRolls.size() > 0 && selRoll >= 0 && selRoll < activeRolls.size() && activeRolls[selRoll].validIm();
}

//--- Active Image ---//
/*
    Return the active image.
    nullptr if no image selected
*/
image* mainWindow::activeImage() {
    if (validIm())
        return activeRolls[selRoll].selImage();
    return nullptr;
}

//--- Active Roll ---//
/*
    Return the active roll.
    nullptr if no roll active
*/
filmRoll* mainWindow::activeRoll() {
    if (validRoll()) {
        return &activeRolls[selRoll];
    }
    return nullptr;
}

//--- Get Image ---//
/*
    Get an image from the current roll
    at a given index. nullptr if image
    doesn't exist
*/
image* mainWindow::getImage(int index) {
    if (selRoll >= 0 && selRoll < activeRolls.size())
        return activeRolls[selRoll].getImage(index);
    return nullptr;
}

//--- Get Image ---//
/*
    Get an image from the specified roll
    and image index. nullptr if that roll
    and image combo doesn't exist
*/
image* mainWindow::getImage(int roll, int index) {
    if (roll >= 0 && roll < activeRolls.size())
        return activeRolls[roll].getImage(index);
    LOG_WARN("Cannot get buffer for image {} in roll {}", index, roll);
    return nullptr;
}

//--- Active Roll Size ---//
/*
    Return the number of images in
    the active roll
*/
int mainWindow::activeRollSize() {
    if (activeRolls.size() > 0 && selRoll >= 0 && selRoll < activeRolls.size())
        return activeRolls[selRoll].rollSize();
    else
        return 0;
}

//--- Clear Selection ---//
/*
    De-select all images in the active roll
*/
void mainWindow::clearSelection() {
    for (int i = 0; i < activeRollSize(); i++) {
        getImage(i)->selected = false;
    }
}

//--- Param Update ---//
/*
    A change has been made to the current
    image. Update the auto-save timeout timer
    and flag in the image the changes
*/
void mainWindow::paramUpdate() {
    lastChange = std::chrono::steady_clock::now();
    activeImage()->needMetaWrite = true;
    metaRefresh = true;
}

//--- Remove Roll ---//
/*
    Remove the current roll
*/
void mainWindow::removeRoll() {
    // Check implication of background rendering
    if (validRoll()) {
        int delRoll = selRoll;
        selRoll = activeRolls.size() > 1 ? selRoll == 0 ? 0 : selRoll-1 : selRoll-1;
        activeRolls[delRoll].clearBuffers(true);
        activeRolls.erase(activeRolls.begin() + delRoll);
        for (int i = 0; i < activeRolls.size(); i++) {
            if (selRoll != i) {
                activeRolls[i].selected = false;
                if (appPrefs.prefs.perfMode)
                    activeRolls[i].clearBuffers();
            }
            if (selRoll == i) {
                activeRolls[i].loadBuffers();
            }
        }
    }
}

//--- Check For Raw ---//
/*
    Check the active selection
    (rolls and images) for any files
    ending in .raw for prompting to
    enter the image details
*/
void mainWindow::checkForRaw() {
    impRawCheck = false;

    auto isRawFile = [](const std::filesystem::path& path) {
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        return ext == ".raw";
    };

    for (const auto& selection : importFiles) {
        try {
            std::filesystem::path selPath(selection);

            if (std::filesystem::is_directory(selPath)) {
                for (const auto& entry : std::filesystem::directory_iterator(selPath)) {
                    if (entry.is_regular_file() && isRawFile(entry.path())) {
                        impRawCheck = true;
                        return;
                    }
                }
            } else if (std::filesystem::is_regular_file(selPath) && isRawFile(selPath)) {
                impRawCheck = true;
                return;
            }
        } catch (const std::exception& e) {
            // Log error but continue checking other files
            continue;
        }
    }
}

//--- Test First Raw File ---//
/*
    Attempt to auto-detect the settings based
    on some known values for Pakon scans
*/
void mainWindow::testFirstRawFile() {
    if (!importFiles.empty()) {
        std::filesystem::path firstPath(importFiles[0]);
        long fileSize;
        if (std::filesystem::is_directory(firstPath)) {
            // Rolls/directories selected, look at first file
            const std::filesystem::path sandbox{firstPath};
            for (auto const& dir_entry : std::filesystem::directory_iterator{sandbox}) {
                if (dir_entry.is_regular_file()) {
                    //Found an image in root of selection
                    if (dir_entry.path().extension().string() == ".raw" ||
                        dir_entry.path().extension().string() == ".RAW")
                        fileSize = std::filesystem::file_size(dir_entry.path());
                }
            }

        } else {
            fileSize = std::filesystem::file_size(firstPath);

        }
        switch (fileSize) {
            case 36000000:
                // 3000x2000 Base 16
                rawSet.width = 3000;
                rawSet.height = 2000;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                break;
            case 36000016:
                // 3000x2000 Base 16 w/header
                rawSet.width = 3000;
                rawSet.height = 2000;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                rawSet.pakonHeader = true;
                break;
            case 20250000:
                // 2250x1500 Base 8
                rawSet.width = 2250;
                rawSet.height = 1500;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                break;
            case 20250016:
                // 2250x1500 Base 8 w/header
                rawSet.width = 2250;
                rawSet.height = 1500;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                rawSet.pakonHeader = true;
                break;
            case 18000000:
                // 1500x2000 Half-frame Base 16
                rawSet.width = 1500;
                rawSet.height = 2000;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                break;
            case 18000016:
                // 1500x2000 Half-frame Base 16 w/header
                rawSet.width = 1500;
                rawSet.height = 2000;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                rawSet.pakonHeader = true;
                break;
            case 9000000:
                // 1500x1000 Base 4
                rawSet.width = 1500;
                rawSet.height = 1000;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                break;
            case 9000016:
                // 1500x1000 Base 4 w/header
                rawSet.width = 1500;
                rawSet.height = 1000;
                rawSet.channels = 3;
                rawSet.bitDepth = 16;
                rawSet.littleE = true;
                rawSet.pakonHeader = true;
                break;
            default:
                // We don't know!
                break;
        }
    }
}

//--- Unsaved Changes ---//
/*
    Loop through all active rolls and look
    for any images with unsaved changes
*/
bool mainWindow::unsavedChanges() {
    bool unsaved = false;
    for (int r = 0; r < activeRolls.size(); r++) {
        unsaved |= activeRolls[r].unsavedImages();
    }
    return unsaved;
}

// Helper function to transform coordinates based on rotation
void transformCoordinates(int& x, int& y, int rotation, int width, int height) {
    int tempX = x;
    int tempY = y;

    switch (rotation) {
        case 6: // Left (90 degrees counterclockwise)
            x = height - 1 - tempY;
            y = tempX;
            break;

        case 3: // Upside-down (180 degrees)
            x = width - 1 - tempX;
            y = height - 1 - tempY;
            break;

        case 8: // Right (90 degrees clockwise)
            x = tempY;
            y = width - 1 - tempX;
            break;

        case 1: // Normal
        default:
            // No transformation needed
            break;
    }
}

// Helper function to inverse transform coordinates from rotated to original
void inverseTransformCoordinates(int& x, int& y, int rotation, int width, int height) {
    int tempX = x;
    int tempY = y;

    switch (rotation) {
        case 6: // Left (90 degrees counterclockwise)
            x = tempY;
            y = height - 1 - tempX;
            break;

        case 3: // Upside-down (180 degrees)
            x = width - 1 - tempX;
            y = height - 1 - tempY;
            break;

        case 8: // Right (90 degrees clockwise)
            x = width - 1 - tempY;
            y = tempX;
            break;

        case 1: // Normal
        default:
            // No transformation needed
            break;
    }
}


//--- Load Mappings ---//
/*
    Load in initial values for some char vectors
    for display.
*/
void mainWindow::loadMappings()
{

    fileTypes.push_back("DPX");
    fileTypes.push_back("EXR");
    fileTypes.push_back("JPEG");
    fileTypes.push_back("PNG");
    fileTypes.push_back("TIFF");


    bitDepths.push_back("8");
    bitDepths.push_back("16");
    bitDepths.push_back("32");

    colorspaceSet.push_back("Colorspace");
    colorspaceSet.push_back("Display");

}
