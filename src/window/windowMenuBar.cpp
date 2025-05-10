#include "window.h"
#include <imgui.h>


void mainWindow::menuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Image(s) (Cmd + O")) {
                openImages();
            }
            if (ImGui::MenuItem("Open Roll(s) (Cmd + Shift + O")) {
                openRolls();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Image (Cmd + S)")) {
                if (validIm())
                    activeImage()->writeXMPFile();
            }
            if (ImGui::MenuItem("Save Roll (Cmd + Shift + S)")) {
                if (validRoll())
                    activeRolls[selRoll].saveAll();
            }
            if (ImGui::MenuItem("Save All (Opt + Shift + S)")) {
                for (int r = 0; r < activeRolls.size(); r++) {
                    activeRolls[r].saveAll();
                }
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Export Image(s)")) {
                exportPopup = true;
            }
            if (ImGui::MenuItem("Export Roll(s)")) {
                exportPopup = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) {
                //settingPopup = true;
            }

            if (ImGui::MenuItem("Exit")) {
                done = true;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Select All (Cmd + A")) {
                // Select All
                if(validRoll()) {
                    int itemCount = activeRolls[selRoll].images.size();
                    for (int i = 0; i < itemCount; i++) {
                            selection.SetItemSelected(selection.GetStorageIdFromIndex(i), true);
                        }
                }
            }

            // Copy
            if (ImGui::MenuItem("Copy")) {
                if (validIm()) {
                    copyIntoParams();
                }
            }
            // Paste
            if (ImGui::MenuItem("Paste")) {
                pasteTrigger = true;
            }
            // Undo/Redo?
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Metadata")) {
            // Import Roll Metadata
            if (ImGui::MenuItem("Import Roll Metadata")) {
                //ImportRollJson();
            }

            // Edit Roll Metadata
            if (ImGui::MenuItem("Edit Roll Metadata")) {
                //globalMetaPopup = true;
            }

            // Edit Image Metadata
            if (ImGui::MenuItem("Edit Image Metadata")) {
                //localMetaPopup = true;
            }

            // Export Metadata
            if (ImGui::MenuItem("Export Metadata")) {
                //exportMetaPopup = true;
            }
            ImGui::EndMenu();
        }
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() * 0.14f));
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.25f);
                if (ImGui::Combo("###", &selRoll, rollNames.data(), activeRolls.size())) {
                    // This is where we call the function necessary for dumping
                    // the loaded images and loading the selected images
                    for (int i = 0; i < activeRolls.size(); i++) {
                        if (selRoll != i) {
                            activeRolls[i].selected = false;
                                activeRolls[i].clearBuffers();
                        }
                        if (selRoll == i) {
                            activeRolls[i].loadBuffers();
                        }
                    }
                }
                ImGui::EndMainMenuBar();
    }
}
