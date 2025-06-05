#include "logger.h"
#include "window.h"
#include <imgui.h>

//--- Thumbnail View Routine ---//
void mainWindow::thumbView() {
    ImGui::SetNextWindowPos(ImVec2(0,imageWinSize.y + 25));
    ImGui::SetNextWindowSize(ImVec2(winWidth,(winHeight - imageWinSize.y) - 25));
    ImGui::Begin("Thumbnails", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        if (!validRoll()) {
            ImGui::End();
            return;
        }

        // Multi-Select setup
        ImGui::PushID(selRoll);
        ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, activeRollSize());

        // Apply any pending selection requests
        selection.ApplyRequests(ms_io);

        // Handle roll changes - clear selection and sync with active roll
        if (rollChange) {
            selection.Clear();

            // Clear all image selected flags first
            for (int i = 0; i < activeRollSize(); i++) {
                getImage(i)->selected = false;
            }
            // Set the selected image from the roll's selIm if valid
            if (activeRoll()->selIm >= 0 && activeRoll()->selIm < activeRollSize()) {
                selection.SetItemSelected(activeRoll()->selIm, true);
                getImage(activeRoll()->selIm)->selected = true;
            }

            rollChange = false;
        }


        // Sync selection state between ImGui and image selection flag
        for (int i = 0; i < activeRollSize(); i++) {
            bool imgui_selected = selection.Contains((ImGuiID)i);
            getImage(i)->selected = imgui_selected;
        }

        // Handle keyboard navigation
        bool navigationOccurred = false;
        if (ImGui::IsWindowFocused() && !ImGui::GetIO().WantCaptureKeyboard) {
            int currentSel = activeRoll()->selIm;
            int newSel = currentSel;

            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && currentSel > 0) {
                newSel = currentSel - 1;
                navigationOccurred = true;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && currentSel < activeRollSize() - 1) {
                newSel = currentSel + 1;
                navigationOccurred = true;
            }

            if (navigationOccurred) {
                // Clear current selection
                selection.Clear();
                for (int i = 0; i < activeRollSize(); i++) {
                    getImage(i)->selected = false;
                }

                // Set new selection
                activeRoll()->selIm = newSel;
                selection.SetItemSelected(newSel, true);
                getImage(newSel)->selected = true;
            }
        }

        // Render thumbnails
        for (int i = 0; i < activeRollSize(); i++) {
            ImVec2 thumbWinSize = ImGui::GetWindowSize();
            ImGui::SetWindowFontScale(std::clamp(thumbWinSize.y / 280.0f, 0.6f, 1.2f));

            float maxAvailable = thumbWinSize.y - ImGui::CalcTextSize("ABCD").y;
            float padding = ImGui::GetStyle().FramePadding.y * 4;
            float spacing = ImGui::GetStyle().ItemSpacing.y * 4;
            float spacingB = ImGui::GetStyle().ItemInnerSpacing.y * 4;
            maxAvailable -= padding;
            maxAvailable -= spacing;
            maxAvailable -= spacingB;

            ImVec2 displaySize;

            int tWidth = getImage(i)->width;
            int tHeight = getImage(i)->height;

            float proxyScale = (getImage(i)->imageLoaded && !toggleProxy && !getImage(i)->reloading) ? 1.0f : appPrefs.prefs.proxyRes;
            int displayWidth = (int)((float)tWidth * proxyScale);
            int displayHeight = (int)((float)tHeight * proxyScale);

            CalculateThumbDisplaySize(displayWidth, displayHeight, maxAvailable, displaySize, getImage(i)->imRot);

            // Start a group to contain the image and its filename
            ImGui::BeginGroup();

            auto pos = ImGui::GetCursorPos();
            ImGui::PushID(i);

            // Set up selection state for this item
            bool item_is_selected = getImage(i)->selected;
            ImGui::SetNextItemSelectionUserData(i);

            if (ImGui::Selectable("###", item_is_selected, 0, displaySize)) {
                // Handle selection change
                if (ImGui::GetIO().KeyCtrl) {
                    // Multi-select mode: toggle this item
                    if (item_is_selected) {
                        selection.SetItemSelected(i, false);
                        getImage(i)->selected = false;
                    } else {
                        selection.SetItemSelected(i, true);
                        getImage(i)->selected = true;
                        activeRoll()->selIm = i; // Update primary selection
                    }
                } else {
                    // Single select mode: clear others and select this one
                    selection.Clear();
                    for (int j = 0; j < activeRollSize(); j++) {
                        getImage(j)->selected = false;
                    }

                    selection.SetItemSelected(i, true);
                    getImage(i)->selected = true;
                    activeRoll()->selIm = i;
                }
            }

            ImGui::SetItemAllowOverlap();
            ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
            ImVec2 IMscreenPos = ImGui::GetCursorScreenPos();

            // Image rendering with rotation
            {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

                // Define UV coordinates for each corner based on rotation
                ImVec2 uv0, uv1, uv2, uv3;
                switch (getImage(i)->imRot) {
                    case 1: // Normal (0°)
                        uv0 = ImVec2(0,0); uv1 = ImVec2(1,0);
                        uv2 = ImVec2(1,1); uv3 = ImVec2(0,1);
                        break;
                    case 6: // 90° clockwise
                        uv0 = ImVec2(0,1); uv1 = ImVec2(0,0);
                        uv2 = ImVec2(1,0); uv3 = ImVec2(1,1);
                        break;
                    case 3: // 180°
                        uv0 = ImVec2(1,1); uv1 = ImVec2(0,1);
                        uv2 = ImVec2(0,0); uv3 = ImVec2(1,0);
                        break;
                    case 8: // 90° counter-clockwise
                        uv0 = ImVec2(1,0); uv1 = ImVec2(1,1);
                        uv2 = ImVec2(0,1); uv3 = ImVec2(0,0);
                        break;
                }

                // Draw the rotated quad
                if (getImage(i)->imageLoaded && !toggleProxy && !getImage(i)->reloading)
                    draw_list->AddImageQuad(
                        reinterpret_cast<ImTextureID>(getImage(i)->glTexture),
                        canvas_pos,
                        ImVec2(canvas_pos.x + displaySize.x, canvas_pos.y),
                        ImVec2(canvas_pos.x + displaySize.x, canvas_pos.y + displaySize.y),
                        ImVec2(canvas_pos.x, canvas_pos.y + displaySize.y),
                        uv0, uv1, uv2, uv3
                    );
                else
                    draw_list->AddImageQuad(
                        reinterpret_cast<ImTextureID>(getImage(i)->glTextureSm),
                        canvas_pos,
                        ImVec2(canvas_pos.x + displaySize.x, canvas_pos.y),
                        ImVec2(canvas_pos.x + displaySize.x, canvas_pos.y + displaySize.y),
                        ImVec2(canvas_pos.x, canvas_pos.y + displaySize.y),
                        uv0, uv1, uv2, uv3
                    );

                ImGui::Dummy(displaySize);
            }

            // Draw custom selection border if selected
            if (getImage(i)->selected) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 topLeft = IMscreenPos;
                ImVec2 bottomRight = ImVec2(IMscreenPos.x + displaySize.x, IMscreenPos.y + displaySize.y);

                float borderThickness = 3.0f;
                ImU32 borderColor = IM_COL32(60, 180, 255, 255);

                drawList->AddRect(topLeft, bottomRight, borderColor, 0.0f, 0, borderThickness);
            }

            // Draw orange circle if image is not loaded
            if (getImage(i) && !getImage(i)->imageLoaded) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImU32 handleColor = IM_COL32(255, 127, 0, 255);
                float pointRadius = 6.0f * std::clamp(thumbWinSize.y / 280.0f, 0.6f, 1.2f);
                drawList->AddCircleFilled(ImVec2(screenPos.x + pointRadius + 12, screenPos.y + pointRadius),
                                        pointRadius, handleColor);
            }

            // Add the filename text below the image
            ImGui::SetCursorPos(ImVec2(pos.x, pos.y + displaySize.y + 2));

            const char* filename = getImage(i)->srcFilename.c_str();
            float textWidth = ImGui::CalcTextSize(filename).x;
            float textHeight = ImGui::CalcTextSize(filename).y;
            float centerOffset = (displaySize.x - textWidth) * 0.5f;
            if (centerOffset > 0) {
                ImGui::SetCursorPosX(pos.x + centerOffset);
            }

            ImGui::TextUnformatted(filename);

            // Purple circle for metadata write needed
            if (getImage(i) && getImage(i)->needMetaWrite) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImU32 handleColor = IM_COL32(172, 2, 250, 255);
                float pointRadius = 6.0f * std::clamp(thumbWinSize.y / 280.0f, 0.6f, 1.2f);
                drawList->AddCircleFilled(ImVec2(screenPos.x + pointRadius + (centerOffset+textWidth), screenPos.y - (pointRadius+(textHeight/2))),
                                        pointRadius, handleColor);
            }

            ImGui::EndGroup();
            ImGui::PopID();
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }

        ImGui::NewLine();
        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);
        ImGui::PopID();

        // Final sync to ensure consistency
        for (int i = 0; i < activeRollSize(); i++) {
            bool imgui_selected = selection.Contains((ImGuiID)i);
            getImage(i)->selected = imgui_selected;
        }
    }
    ImGui::End();
}
