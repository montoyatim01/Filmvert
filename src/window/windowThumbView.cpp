#include "window.h"

void mainWindow::thumbView() {
    ImGui::SetNextWindowPos(ImVec2(0,winHeight - 280));
            ImGui::SetNextWindowSize(ImVec2(winWidth,280));
            ImGui::Begin("Thumbnails", 0, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
            {
                //SDL_GetWindowSize(window, &winWidth, &winHeight);

                //Multi-Select
                ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
                ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, activeRollSize());
                selection.ApplyRequests(ms_io);

                if (validRoll()) {
                    for (int i = 0; i < activeRollSize(); i++) {
                        if (!getImage(i)->texture) {
                            updateSDLTexture(getImage(i));
                        }
                        if (getImage(i)->sdlUpdate) {
                            updateSDLTexture(getImage(i));
                        }

                            ImVec2 maxAvailable = {320,240};
                            ImVec2 displaySize;
                            ImVec2 elGuideSize;

                            // Example image dimensions (replace with actual)
                            CalculateDisplaySize(getImage(i)->width, getImage(i)->height, maxAvailable, displaySize, getImage(i)->imRot);

                            // Start a group to contain the image and its filename
                            ImGui::BeginGroup();

                            auto pos = ImGui::GetCursorPos();
                            ImGui::PushID(i);
                            //Multi-select
                            bool item_is_selected = selection.Contains((ImGuiID)i);
                            getImage(i)->selected = item_is_selected;
                            ImGui::SetNextItemSelectionUserData(i);
                            if (ImGui::Selectable("###", getImage(i)->selected, 0, displaySize)) {
                                if (!ImGui::GetIO().KeySuper) {
                                    activeRolls[selRoll].selIm = i;
                                }
                            }
                            ImGui::SetItemAllowOverlap();
                            ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
                            ImGui::Image(reinterpret_cast<ImTextureID>(getImage(i)->texture), displaySize);

                            // Draw red circle if image is not loaded
                            if (getImage(i) && !getImage(i)->imageLoaded) {
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                // Get the absolute screen position for the circle (top-left corner of the image)
                                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                                screenPos.x = screenPos.x - displaySize.x; // Move back to the image's left edge
                                ImU32 handleColor = IM_COL32(255, 127, 0, 255);
                                float pointRadius = 8.0f;
                                // Use absolute screen coordinates for the circle
                                drawList->AddCircleFilled(ImVec2(screenPos.x + pointRadius + 20, screenPos.y + pointRadius),
                                                        pointRadius, handleColor);
                            }

                            // Add the filename text below the image
                            ImGui::SetCursorPos(ImVec2(pos.x, pos.y + displaySize.y + 2));

                            // Get the filename from srcFilename
                            const char* filename = getImage(i)->srcFilename.c_str();

                            // Calculate text width to center it
                            float textWidth = ImGui::CalcTextSize(filename).x;
                            float centerOffset = (displaySize.x - textWidth) * 0.5f;
                            if (centerOffset > 0) {
                                ImGui::SetCursorPosX(pos.x + centerOffset);
                            }

                            // Display the filename
                            ImGui::TextUnformatted(filename);

                            ImGui::EndGroup();
                            ImGui::PopID();
                            ImGui::SameLine();
                    }
                }

                ms_io = ImGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);


    }
    ImGui::End();
}
