#include "roll.h"
#include "window.h"
#include <filesystem>


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

bool ImRightAlign(const char* str_id)
{
    if(ImGui::BeginTable(str_id, 2, ImGuiTableFlags_SizingFixedFit, ImVec2(-1,0)))
    {
        ImGui::TableSetupColumn("a", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        return true;
    }
    return false;
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


bool mainWindow::validRoll() {
    return activeRolls.size() > 0 && selRoll >= 0 && selRoll < activeRolls.size();
}

bool mainWindow::validIm() {
    return activeRolls.size() > 0 && selRoll >= 0 && selRoll < activeRolls.size() && activeRolls[selRoll].validIm();
}

image* mainWindow::activeImage() {
    if (validIm())
        return activeRolls[selRoll].selImage();
    return nullptr;
}
filmRoll* mainWindow::activeRoll() {
    if (validRoll()) {
        return &activeRolls[selRoll];
    }
    return nullptr;
}
image* mainWindow::getImage(int index) {
    if (selRoll >= 0 && selRoll < activeRolls.size())
        return activeRolls[selRoll].getImage(index);
    return nullptr;
}
image* mainWindow::getImage(int roll, int index) {
    if (roll >= 0 && roll < activeRolls.size())
        return activeRolls[roll].getImage(index);
    LOG_WARN("Cannot get buffer for image {} in roll {}", index, roll);
    return nullptr;
}
int mainWindow::activeRollSize() {
    if (activeRolls.size() > 0 && selRoll >= 0 && selRoll < activeRolls.size())
        return activeRolls[selRoll].images.size();
    else
        return 0;
}

void mainWindow::clearSelection() {
    for (int i = 0; i < activeRollSize(); i++) {
        getImage(i)->selected = false;
    }
}

void mainWindow::paramUpdate() {
    // We've made a change to the current image
    // Update the timer and flag the changes
    lastChange = std::chrono::steady_clock::now();
    activeImage()->needMetaWrite = true;
    metaRefresh = true;
}

void mainWindow::removeRoll() {
    // Check implication of background rendering
    if (validRoll()) {
        int delRoll = selRoll;
        selRoll = activeRolls.size() > 1 ? selRoll == 0 ? 0 : selRoll-1 : selRoll-1;
        activeRolls[delRoll].clearBuffers(true);
        activeRolls.erase(activeRolls.begin() + delRoll);
    }
}

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

    colorspaceSet.push_back("Colorspace");
    colorspaceSet.push_back("Display");

    /*activeRolls.resize(2);
    rollNames.push_back("roll 1");
    rollNames.push_back("roll 2");*/
}
