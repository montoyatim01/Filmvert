#include "preferences.h"
#include "roll.h"
#include "window.h"
#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <imgui.h>

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

	ImVec4* colors = ImGui::GetStyle().Colors;
colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
colors[ImGuiCol_FrameBg]                = ImVec4(0.31f, 0.31f, 0.31f, 0.54f);
colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.60f, 0.60f, 0.60f, 0.40f);
colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.21f, 0.31f, 1.00f);
colors[ImGuiCol_CheckMark]              = ImVec4(0.89f, 0.89f, 0.89f, 1.00f);
colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.47f, 0.88f, 1.00f);
colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
colors[ImGuiCol_ButtonHovered]          = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
colors[ImGuiCol_ButtonActive]           = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
colors[ImGuiCol_Header]                 = ImVec4(0.53f, 0.53f, 0.53f, 0.00f);
colors[ImGuiCol_HeaderHovered]          = ImVec4(0.24f, 0.40f, 0.60f, 1.00f);
colors[ImGuiCol_Separator]              = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
colors[ImGuiCol_ResizeGrip]             = ImVec4(0.98f, 0.98f, 0.98f, 0.20f);
colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.82f, 0.83f, 0.83f, 0.67f);
colors[ImGuiCol_Tab]                    = ImVec4(0.58f, 0.58f, 0.58f, 0.86f);



}

void setTempColor(float tempVal) {
    ImVec4 FrameBG = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
    float tempColor[4] = {FrameBG.x, FrameBG.y, FrameBG.z, FrameBG.w};
    float warm[4] = {2.0, 1.0, 0.0, 1.0};
    float cool[4] = {0.0, 2.0, 2.0, 1.0};
    tempVal *= -1.0f;
    for (int i = 0; i < 4; i++) {
        tempColor[i] = tempVal >= 0.0 ?
            (tempColor[i] * cool[i] * tempVal) + ((1.0 - tempVal) * tempColor[i]) :
            (tempColor[i] * warm[i] * (-1.0 * tempVal)) + ((1.0 - (-1.0 * tempVal)) * tempColor[i]);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(tempColor[0] * 1.2, tempColor[1] * 1.2, tempColor[2] * 1.2, tempColor[3]));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(tempColor[0] * 1.1, tempColor[1] * 1.1, tempColor[2] * 1.1, 1.0));
    return;
}

void setTintColor(float tintVal) {
    ImVec4 FrameBG = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
    float tempColor[4] = {FrameBG.x, FrameBG.y, FrameBG.z, FrameBG.w};
    float green[4] = {0.0, 1.5, 0.0, 1.0};
    float mag[4] = {1.5, 0.0, 1.5, 1.0};
    tintVal *= 0.75f;
    for (int i = 0; i < 4; i++) {
        tempColor[i] = tintVal >= 0.0 ?
            (tempColor[i] * mag[i] * tintVal) + ((1.0 - tintVal) * tempColor[i]) :
            (tempColor[i] * green[i] * (-1.0 * tintVal)) + ((1.0 - (-1.0 * tintVal)) * tempColor[i]);
    }
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(tempColor[0] * 1.2, tempColor[1] * 1.2, tempColor[2] * 1.2, tempColor[3]));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(tempColor[0] * 1.1, tempColor[1] * 1.1, tempColor[2] * 1.1, 1.0));
    return;
}

bool DragFloat4WithColors(const char* label, float col[4], float speed, bool cmyk) {
    bool valueChanged = false;
    std::string labelRGBW = label;
    labelRGBW += "RGBW";
    ImGui::PushID(labelRGBW.c_str());
    const float windowWidth = ImGui::GetContentRegionAvail().x;
    const float resetW    = ImGui::CalcTextSize("Reset").x
                          + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float buttonWidth = ImGui::GetFrameHeight();
    const float totalSliderW = windowWidth - resetW - buttonWidth
                                 - ImGui::GetStyle().ItemSpacing.x * 5.0f;
    const float sliderW      = std::floor(totalSliderW / 4.0f);
    const float lastSliderW  = totalSliderW - sliderW * 3.0f;

    // Red Slider
    if (cmyk) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(67,112,112,138));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(131,220,220,138));
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(112,67,67,138));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(220,131,131,138));
    }

    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragR", &col[0], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
ImGui::SameLine();
    // Green Slider
    if (cmyk) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(112,67,112,138));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(220,131,220,138));
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(67,112,67,138));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(131,220,131,138));
    }

    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragG", &col[1], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
ImGui::SameLine();
    // Blue Slider
    if (cmyk) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(112,112,67,138));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(220,220,131,138));
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(67,67,112,138));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(131,131,220,138));
    }

    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragB", &col[2], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
ImGui::SameLine();
    // White Slider
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(112,112,112,138));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(220,220,220,138));
    ImGui::SetNextItemWidth(lastSliderW);
    valueChanged |= ImGui::DragFloat("##dragW", &col[3], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();


    ImGui::PopID();
    return valueChanged;
}

bool DragFloat3WithColors(const char* label, float col[3], float speed) {
    bool valueChanged = false;
    std::string labelRGB = label;
    labelRGB += "RGB";
    ImGui::PushID(labelRGB.c_str());
    const float windowWidth = ImGui::GetContentRegionAvail().x;
    const float resetW    = ImGui::CalcTextSize("Reset").x
                          + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float buttonWidth = ImGui::GetFrameHeight();
    const float sliderW = (windowWidth - (resetW + buttonWidth + (ImGui::GetStyle().ItemSpacing.x * 4))) / 3.0f;

    // Red Slider
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(112,67,67,138));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(220,131,131,138));
    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragR", &col[0], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
ImGui::SameLine();
    // Green Slider
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(67,112,67,138));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(131,220,131,138));
    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragG", &col[1], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
ImGui::SameLine();
    // Blue Slider
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(67,67,112,138));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(131,131,220,138));
    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragB", &col[2], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();


    ImGui::PopID();
    return valueChanged;
}

bool DragFloat1WithColors(const char* label, float col[4], float speed) {
    bool valueChanged = false;
    std::string labelRGBW = label;
    labelRGBW += "RGBW";
    ImGui::PushID(labelRGBW.c_str());
    float windowWidth = ImGui::GetContentRegionAvail().x;
    const float resetW   = ImGui::CalcTextSize("Reset").x
                         + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float buttonWidth = ImGui::GetFrameHeight();
    const float sliderW  = -(resetW + (ImGui::GetStyle().ItemSpacing.x * 2) + buttonWidth);


    // White Slider
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(112,112,112,138));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(220,220,220,138));
    ImGui::SetNextItemWidth(sliderW);
    valueChanged |= ImGui::DragFloat("##dragW", &col[3], speed, -10.0f, 10.0f);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();


    ImGui::PopID();
    return valueChanged;
}

//--- Custom Color Slider ---//
/*
    Custom color slider widget to enable
    holding shift for more fine-tune
    controls over the values
*/
bool ColorEdit4WithFineTune(const char* label, float col[4], bool cmyk, bool slow, ImGuiColorEditFlags flags) {
    bool value_changed = false;

    ImGui::PushID(label);

    float speed;
    if (appPrefs.prefs.altGrades && !slow)
        speed = ImGui::GetIO().KeyShift ? 0.0010f : 0.040f;
    else
        speed = ImGui::GetIO().KeyShift ? 0.000005f : 0.0005f;

    // Use DragFloat4 for the color components
    value_changed = DragFloat4WithColors("##drag", col, speed, cmyk);
        //ImGui::DragFloat4("##drag", col, speed, -10.0f, 10.0f);

    // Add color preview
    ImGui::SameLine();
    if (ImGui::ColorButton("##preview", ImVec4(col[0], col[1], col[2], col[3]), flags)) {
        ImGui::OpenPopup("color_picker");
    }
    if (ImGui::BeginPopup("color_picker")) {
        value_changed |= ImGui::ColorPicker4("##picker", col, flags);
        ImGui::EndPopup();
    }

    ImGui::PopID();

    return value_changed;
}

bool ColorEdit3WithFineTune(const char* label, float col[3], ImGuiColorEditFlags flags) {
    bool value_changed = false;

    ImGui::PushID(label);

    // Base speed - when shift is held, use much smaller speed
    float speed = ImGui::GetIO().KeyShift ? 0.000005f : 0.001f;

    // Use DragFloat4 for the color components
    value_changed = DragFloat3WithColors("##drag", col, speed);
        //ImGui::DragFloat4("##drag", col, speed, -10.0f, 10.0f);
    float a = 1.0f;
    // Add color preview
    ImGui::SameLine();
    if (ImGui::ColorButton("##preview", ImVec4(col[0], col[1], col[2], a), flags)) {
        ImGui::OpenPopup("color_picker");
    }
    if (ImGui::BeginPopup("color_picker")) {
        value_changed |= ImGui::ColorPicker4("##picker", col, flags);
        ImGui::EndPopup();
    }

    ImGui::PopID();

    return value_changed;
}

bool ColorEdit1WithFineTune(const char* label, float col[4], ImGuiColorEditFlags flags) {
    bool value_changed = false;

    ImGui::PushID(label);

    // Base speed - when shift is held, use much smaller speed
    float speed = ImGui::GetIO().KeyShift ? 0.000005f : 0.001f;

    // Use DragFloat4 for the color components
    value_changed = DragFloat1WithColors("##drag", col, speed);
        //ImGui::DragFloat4("##drag", col, speed, -10.0f, 10.0f);

    // Add color preview
    ImGui::SameLine();
    if (ImGui::ColorButton("##preview", ImVec4(col[0], col[1], col[2], col[3]), flags)) {
        ImGui::OpenPopup("color_picker");
    }
    if (ImGui::BeginPopup("color_picker")) {
        value_changed |= ImGui::ColorPicker4("##picker", col, flags);
        ImGui::EndPopup();
    }

    ImGui::PopID();

    return value_changed;
}

bool TreeNodeWithLine(const char* label, bool* checked, bool altHeld, float rightPad) {
    bool open = ImGui::TreeNode(label);
    ImVec2 itemMin = ImGui::GetItemRectMin();
    ImVec2 itemMax = ImGui::GetItemRectMax();
    float lineY    = (itemMin.y + itemMax.y) * 0.5f;
    float lineX0   = itemMax.x + ImGui::GetStyle().ItemSpacing.x;
    float winRight = ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x;

    if (checked) {
        // Shorten the line to leave room for the checkbox on the right.
        float frameH = ImGui::GetFrameHeight();
        float lineX1 = winRight - rightPad - frameH - (ImGui::GetStyle().ItemSpacing.x * 4);
        if (lineX0 < lineX1)
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(lineX0, lineY), ImVec2(lineX1, lineY),
                ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

        // Draw checkbox flush with the right content edge.
        std::string chkId = std::string("##tnl_") + label;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - rightPad - frameH);

        if (!altHeld) {
            // Momentary mode: held → false, released → spring back to true,
            bool dummy = *checked;
            ImGui::Checkbox(chkId.c_str(), &dummy);
            if (ImGui::IsItemActive())
                *checked = false;           // force off while held
            else if (ImGui::IsItemDeactivated())
                *checked = true;            // spring back the moment mouse lifts
            // else: no interaction this frame — preserve whatever *checked is
        } else {
            // Alt mode: normal latching toggle.
            ImGui::Checkbox(chkId.c_str(), checked);
        }
    } else {
        // No checkbox — line extends to the full right edge.
        float lineX1 = winRight;
        if (lineX0 < lineX1)
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(lineX0, lineY), ImVec2(lineX1, lineY),
                ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
    }
    return open;
}

bool is_hidden(const std::filesystem::path& p) {
    auto filename = p.filename().string();
    return !filename.empty() && filename[0] == '.';
}



// Function to calculate display dimensions while maintaining aspect ratio
void mainWindow::CalculateThumbDisplaySize(int imageWidth, int imageHeight, float maxHeight, ImVec2& outDisplaySize, int rotation) {

    bool rot = rotation == 5 || rotation == 6 || rotation == 7 || rotation == 8 ? true : false;
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

//--- Clear Buffers ---//
/*
    Helper function to clear a roll's buffers
    and also delete the gl buffer associated with it
 */
 void mainWindow::clearRoll(filmRoll* roll) {
    if (roll) {
        bool cleared = roll->clearBuffers();
        if (cleared) {
            for (auto& img : roll->images) {
                gpu->clearImBuffer(&img);
            }
        }
    }
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
        for (auto &img : activeRolls[delRoll].images) {
            gpu->clearImBuffer(&img);
            gpu->clearSmBuffer(&img);
        }
        activeRolls.erase(activeRolls.begin() + delRoll);
        for (int i = 0; i < activeRolls.size(); i++) {
            if (selRoll != i) {
                activeRolls[i].selected = false;
                if (appPrefs.prefs.perfMode)
                    clearRoll(&activeRolls[i]);

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
        checkRawFile(rawSet, fileSize);
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

//--- Data Format ---//
/*
    Format a nice string with units
    for a given byte value
*/
std::string mainWindow::byteFormat(uint64_t v) {
    if (v >= 1ULL << 30) return fmt::format("{:.2f} GB", v / (double)(1ULL << 30));
        if (v >= 1ULL << 20) return fmt::format("{:.2f} MB", v / (double)(1ULL << 20));
        if (v >= 1ULL << 10) return fmt::format("{:.2f} KB", v / (double)(1ULL << 10));
        return fmt::format("{} B", v);
}

// Helper function to transform coordinates based on rotation
void transformCoordinates(int& x, int& y, int rotation, int width, int height) {
    int tempX = x;
    int tempY = y;

    switch (rotation) {
        case 1: // Normal
            // No transformation needed
            break;

        case 2: // Horizontal flip
            x = width - 1 - tempX;
            y = tempY;
            break;

        case 3: // Upside-down (180 degrees)
            x = width - 1 - tempX;
            y = height - 1 - tempY;
            break;

        case 4: // Vertical flip
            x = tempX;
            y = height - 1 - tempY;
            break;

        case 5: // Transpose (horizontal flip + 90 degrees counterclockwise)
            x = height - 1 - tempY;
            y = width - 1 -tempX;
            break;

        case 6: // Left (90 degrees counterclockwise)
            x = height - 1 - tempY;
            y = tempX;
            break;

        case 7: // Transverse (horizontal flip + 90 degrees clockwise)
            x = tempY;
            y = tempX;
            break;

        case 8: // Right (90 degrees clockwise)
            x = tempY;
            y = width - 1 - tempX;
            break;

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
        case 1: // Normal
            // No transformation needed
            break;

        case 2: // Horizontal flip
            x = width - 1 - tempX;
            y = tempY;
            break;

        case 3: // Upside-down (180 degrees)
            x = width - 1 - tempX;
            y = height - 1 - tempY;
            break;

        case 4: // Vertical flip
            x = tempX;
            y = height - 1 - tempY;
            break;

        case 5: // Transpose (horizontal flip + 90 degrees counterclockwise)
            x = width  - 1 - tempY;
            y = height - 1 - tempX;
            break;

        case 6: // Left (90 degrees counterclockwise)
            x = tempY;
            y = height - 1 - tempX;
            break;

        case 7: // Transverse (horizontal flip + 90 degrees clockwise)
            x = tempY;
            y = tempX;
            break;

        case 8: // Right (90 degrees clockwise)
            x = width - 1 - tempY;
            y = tempX;
            break;

        default:
            // No transformation needed
            break;
    }
}

// Transform a point from original image coordinates to cropped/rotated display coordinates
ImVec2 transformPointToCroppedDisplay(float origX, float origY, image* img, float dispScale, ImVec2 imagePos) {
    ImVec2 result;

    if (img->imgParam.cropEnable) {
        // Mirrors getCroppedRotatedUV in glslKernels.h: the view shows the crop rectangle of the
        // arbitrarily rotated image, and the EXIF rotation is applied over the crop size.
        float aspect = (float)img->width / (float)img->height;
        float rot = -img->imgParam.arbitraryRotation * M_PI / 180.0f;
        float dx = (origX - 0.5f) * aspect;
        float dy = origY - 0.5f;
        float wx = (dx * cos(rot) - dy * sin(rot)) / aspect + 0.5f;
        float wy = dx * sin(rot) + dy * cos(rot) + 0.5f;

        float cropW = img->imgParam.imageCropMaxX - img->imgParam.imageCropMinX;
        float cropH = img->imgParam.imageCropMaxY - img->imgParam.imageCropMinY;
        float u = (wx - img->imgParam.imageCropMinX) / cropW;
        float v = (wy - img->imgParam.imageCropMinY) / cropH;
        if (!(u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)) {
            // Point is outside crop, return off-screen position
            result.x = -1000;
            result.y = -1000;
            return result;
        }

        int effW = std::max(1, (int)(cropW * img->width));
        int effH = std::max(1, (int)(cropH * img->height));
        int pixelX = u * effW;
        int pixelY = v * effH;
        transformCoordinates(pixelX, pixelY, img->imgParam.rotation, effW, effH);
        result.x = imagePos.x + pixelX * dispScale;
        result.y = imagePos.y + pixelY * dispScale;
    } else {
        // No crop enabled, use standard transformation with EXIF rotation
        int pixelX = origX * img->width;
        int pixelY = origY * img->height;

        // Apply EXIF rotation transformation
        int transformedX = pixelX;
        int transformedY = pixelY;

        switch (img->imgParam.rotation) {
            case 1: // Normal
                break;
            case 2: // Flip horizontal
                transformedX = img->width - pixelX;
                break;
            case 3: // Rotate 180
                transformedX = img->width - pixelX;
                transformedY = img->height - pixelY;
                break;
            case 4: // Flip vertical
                transformedY = img->height - pixelY;
                break;
            case 5: // Rotate 90 CCW + flip horizontal
                transformedX = img->height - pixelY;
                transformedY = img->width - pixelX;
                break;
            case 6: // Rotate 90 CW
                transformedX = img->height - pixelY;
                transformedY = pixelX;
                break;
            case 7: // Rotate 90 CW + flip horizontal
                transformedX = pixelY;
                transformedY = pixelX;
                break;
            case 8: // Rotate 90 CCW
                transformedX = pixelY;
                transformedY = img->width - pixelX;
                break;
        }

        result.x = imagePos.x + transformedX * dispScale;
        result.y = imagePos.y + transformedY * dispScale;
    }

    return result;
}

// Inverse transform from display coordinates back to original image coordinates
bool inverseTransformFromCroppedDisplay(ImVec2 displayPos, ImVec2 imagePos, image* img, float dispScale, float& origX, float& origY) {
    if (img->imgParam.cropEnable) {
        // Inverse of transformPointToCroppedDisplay.
        float cropW = img->imgParam.imageCropMaxX - img->imgParam.imageCropMinX;
        float cropH = img->imgParam.imageCropMaxY - img->imgParam.imageCropMinY;
        int effW = std::max(1, (int)(cropW * img->width));
        int effH = std::max(1, (int)(cropH * img->height));
        int pixelX = (displayPos.x - imagePos.x) / dispScale;
        int pixelY = (displayPos.y - imagePos.y) / dispScale;
        inverseTransformCoordinates(pixelX, pixelY, img->imgParam.rotation, effW, effH);
        pixelX = std::clamp(pixelX, 0, effW);
        pixelY = std::clamp(pixelY, 0, effH);
        float wx = img->imgParam.imageCropMinX + (float)pixelX / effW * cropW;
        float wy = img->imgParam.imageCropMinY + (float)pixelY / effH * cropH;

        float aspect = (float)img->width / (float)img->height;
        float rot = img->imgParam.arbitraryRotation * M_PI / 180.0f;
        float dx = (wx - 0.5f) * aspect;
        float dy = wy - 0.5f;
        origX = (dx * cos(rot) - dy * sin(rot)) / aspect + 0.5f;
        origY = dx * sin(rot) + dy * cos(rot) + 0.5f;
        return origX >= 0.0f && origX <= 1.0f && origY >= 0.0f && origY <= 1.0f;
    } else {
        // No crop, use standard inverse transformation with EXIF rotation
        int transformedX = (displayPos.x - imagePos.x) / dispScale;
        int transformedY = (displayPos.y - imagePos.y) / dispScale;

        // Apply inverse EXIF rotation
        int pixelX = transformedX;
        int pixelY = transformedY;

        switch (img->imgParam.rotation) {
            case 1: // Normal
                break;
            case 2: // Flip horizontal
                pixelX = img->width - transformedX;
                break;
            case 3: // Rotate 180
                pixelX = img->width - transformedX;
                pixelY = img->height - transformedY;
                break;
            case 4: // Flip vertical
                pixelY = img->height - transformedY;
                break;
            case 5: // Rotate 90 CCW + flip horizontal
                pixelX = img->width - transformedY;
                pixelY = img->height - transformedX;
                break;
            case 6: // Rotate 90 CW
                pixelX = transformedY;
                pixelY = img->height - transformedX;
                break;
            case 7: // Rotate 90 CW + flip horizontal
                pixelX = transformedY;
                pixelY = transformedX;
                break;
            case 8: // Rotate 90 CCW
                pixelX = img->width - transformedY;
                pixelY = transformedX;
                break;
        }

        origX = (float)pixelX / img->width;
        origY = (float)pixelY / img->height;

        return (origX >= 0 && origX <= 1 && origY >= 0 && origY <= 1);
    }
}

//--- Select Forward ---//
/*
    Advance the active selection one image forward
*/
void mainWindow::selectForward() {
    int currentSel = activeRoll()->selIm;
    int newSel = currentSel;
    newSel = currentSel + 1;
    newSel = newSel < 0 ? 0 :
        newSel > activeRollSize() - 1 ? activeRollSize() - 1 : newSel;
    // Clear current selection
    selection.Clear();
    for (int i = 0; i < activeRollSize(); i++) {
        getImage(i)->selected = false;
    }

    // Set new selection
    activeRoll()->selIm = newSel;
    selection.SetItemSelected(newSel, true);
    getImage(newSel)->selected = true;
    flagVisibleImage();
}

//--- Select Backward ---//
/*
    Advance the active selection one image backward
*/
void mainWindow::selectBackward() {
    int currentSel = activeRoll()->selIm;
    int newSel = currentSel;
    newSel = currentSel - 1;

    newSel = newSel < 0 ? 0 :
        newSel > activeRollSize() - 1 ? activeRollSize() - 1 : newSel;
    // Clear current selection
    selection.Clear();
    for (int i = 0; i < activeRollSize(); i++) {
        getImage(i)->selected = false;
    }

    // Set new selection
    activeRoll()->selIm = newSel;
    selection.SetItemSelected(newSel, true);
    getImage(newSel)->selected = true;
    flagVisibleImage();
}

//--- Flag Visible Image ---//
/*
    Set's the visible flag in the active image
*/
void mainWindow::flagVisibleImage() {
    if (validRoll()) {
        activeRoll()->clearVisible();
        if (validIm()) {
            activeImage()->visible = true;
        }
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

    csBake.push_back("None");
    csBake.push_back("Flip Only");
    csBake.push_back("All");

    colorPickers.push_back("Hue Wheel");
    colorPickers.push_back("Hue Bar");

    scalingModes.push_back("Linear");
    scalingModes.push_back("Nearest");

    viewPixelDepth.push_back("8-bit");
    viewPixelDepth.push_back("10-bit");
    viewPixelDepth.push_back("Float");

}
