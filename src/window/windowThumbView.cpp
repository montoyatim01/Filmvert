#include "window.h"
#include <imgui.h>

//--- Thumbnail View Routine ---//
void mainWindow::thumbView() {
    ImGui::SetNextWindowPos(ImVec2(0,imageWinSize.y + 25));
    ImGui::SetNextWindowSize(ImVec2(winWidth,(winHeight - imageWinSize.y) - 25));
    ImGui::Begin("Thumbnails", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
            {

                //Multi-Select
                ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
                ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, activeRollSize());
                selection.ApplyRequests(ms_io);
                if (rollChange) {
                    selection.Clear();
                    if (validRoll())
                        selection.SetItemSelected(activeRoll()->selIm, true);
                    rollChange = false;
                }

                if (validRoll()) {
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

                            // Example image dimensions (replace with actual)
                            CalculateThumbDisplaySize(getImage(i)->width, getImage(i)->height, maxAvailable, displaySize, getImage(i)->imRot);

                            // Start a group to contain the image and its filename
                            ImGui::BeginGroup();

                            auto pos = ImGui::GetCursorPos();
                            ImGui::PushID(i);
                            //Multi-select
                            selection.SetItemSelected((ImGuiID)activeRoll()->selIm, true);
                            bool item_is_selected = selection.Contains((ImGuiID)i);
                            getImage(i)->selected = item_is_selected;
                            ImGui::SetNextItemSelectionUserData(i);
                            if (ImGui::Selectable("###", getImage(i)->selected, 0, displaySize)) {
                                if (!ImGui::GetIO().KeySuper) {
                                    activeRoll()->selIm = i;
                                }
                            }
                            ImGui::SetItemAllowOverlap();
                            ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
                            ImVec2 IMscreenPos = ImGui::GetCursorScreenPos(); // Get screen pos AFTER setting cursor pos

                            //ImGui::Image(reinterpret_cast<ImTextureID>(getImage(i)->glTexture), displaySize);
                            {
                                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

                                // Define UV coordinates for each corner based on rotation
                                ImVec2 uv0, uv1, uv2, uv3; // top-left, top-right, bottom-right, bottom-left
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
                                draw_list->AddImageQuad(
                                    reinterpret_cast<ImTextureID>(getImage(i)->glTexture),
                                    canvas_pos, // top-left
                                    ImVec2(canvas_pos.x + displaySize.x, canvas_pos.y), // top-right
                                    ImVec2(canvas_pos.x + displaySize.x, canvas_pos.y + displaySize.y), // bottom-right
                                    ImVec2(canvas_pos.x, canvas_pos.y + displaySize.y), // bottom-left
                                    uv0, uv1, uv2, uv3
                                );

                                // Reserve space in ImGui layout
                                ImGui::Dummy(displaySize);
                            }

                            // Draw custom selection border if selected
                            if (getImage(i)->selected) {
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                ImVec2 topLeft = IMscreenPos; // Start at the image position
                                ImVec2 bottomRight = ImVec2(IMscreenPos.x + displaySize.x, IMscreenPos.y + displaySize.y); // Add display size

                                float borderThickness = 3.0f;
                                ImU32 borderColor = IM_COL32(60, 180, 255, 255);

                                drawList->AddRect(topLeft, bottomRight, borderColor, 0.0f, 0, borderThickness);
                            }

                            // Draw orange circle if image is not loaded
                            if (getImage(i) && !getImage(i)->imageLoaded) {
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                // Get the absolute screen position for the circle (top-left corner of the image)
                                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                                //screenPos.x = screenPos.x - displaySize.x; // Move back to the image's left edge
                                ImU32 handleColor = IM_COL32(255, 127, 0, 255);
                                float pointRadius = 6.0f * std::clamp(thumbWinSize.y / 280.0f, 0.6f, 1.2f);
                                // Use absolute screen coordinates for the circle
                                drawList->AddCircleFilled(ImVec2(screenPos.x + pointRadius + 12, screenPos.y + pointRadius),
                                                        pointRadius, handleColor);
                            }

                            // Add the filename text below the image
                            ImGui::SetCursorPos(ImVec2(pos.x, pos.y + displaySize.y + 2));

                            // Get the filename from srcFilename
                            const char* filename = getImage(i)->srcFilename.c_str();

                            // Calculate text width to center it
                            float textWidth = ImGui::CalcTextSize(filename).x;
                            float textHeight = ImGui::CalcTextSize(filename).y;
                            float centerOffset = (displaySize.x - textWidth) * 0.5f;
                            if (centerOffset > 0) {
                                ImGui::SetCursorPosX(pos.x + centerOffset);
                            }

                            // Display the filename
                            ImGui::TextUnformatted(filename);
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
                }
                ImGui::NewLine();
                ms_io = ImGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);


    }
    ImGui::End();
}
