#include "logger.h"
#include "ocioProcessor.h"
#include "window.h"
#include <cstring>
#include <imgui.h>


void mainWindow::menuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Image(s)")) {
                openImages();
            }
            if (ImGui::MenuItem("Open Roll(s)")) {
                openRolls();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Image")) {
                if (validIm())
                    activeImage()->writeXMPFile();
            }
            if (ImGui::MenuItem("Save Roll")) {
                if (validRoll()) {
                    activeRolls[selRoll].saveAll();
                    activeRolls[selRoll].exportRollMetaJSON();
                }

            }
            if (ImGui::MenuItem("Save All")) {
                for (int r = 0; r < activeRolls.size(); r++) {
                    activeRolls[r].saveAll();
                    activeRolls[r].exportRollMetaJSON();
                }
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Export Image(s)")) {
                expSetting.display = ocioProc.displayOp;
                expSetting.view = ocioProc.viewOp;
                expRolls = false;
                exportPopup = true;
            }
            if (ImGui::MenuItem("Export Roll(s)")) {
                expSetting.display = ocioProc.displayOp;
                expSetting.view = ocioProc.viewOp;
                expRolls = true;
                exportPopup = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close Roll")) {
                //closeRoll;
                //Actions needed to close a roll
                //Check if unsaved, prompt
                if (validRoll()) {
                    if (activeRolls[selRoll].unsavedImages()) {
                        // We have unsaved images, prompt user
                        unsavedPopTrigger = true;

                    } else {
                        removeRoll();
                    }
                }
                //Change current active roll to previous (-1 if 0)
                //Unload the roll
                //Delete from the queue
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences")) {
                badOcioText = false;
                std::strcpy(ocioPath, preferences.ocioPath.c_str());
                if (preferences.ocioExt)
                    ocioSel = 1;
                preferencesPopTrig = true;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                done = true;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Select All")) {
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
                if (openJSON()) {
                    std::strcpy(ackMsg, "Metadata imported to Roll successfully!");
                    ackPopTrig = true;
                } else {
                    std::string err = ackError;
                    if (!err.empty()) {
                        std::strcpy(ackMsg, "Metadata failed to import:");
                        ackPopTrig = true;
                    }
                }
            }
            ImGui::Separator();

            // Export Metadata
            if (ImGui::MenuItem("Export Roll Metadata")) {
                if (validRoll()) {
                    if(activeRolls[selRoll].exportRollMetaJSON()) {
                        std::strcpy(ackMsg, "Roll Export Successful!");
                        ackPopTrig = true;
                    } else {
                        std::strcpy(ackMsg, "Roll Failed to export!");
                        ackPopTrig = true;
                    }
                }
            }

            // Export CSV
            if (ImGui::MenuItem("Export Roll CSV")) {
                if (validRoll()) {
                    if (activeRolls[selRoll].exportRollCSV()) {
                        std::strcpy(ackMsg, "CSV Exported to Roll directory.");
                        ackPopTrig = true;
                    } else {
                        std::strcpy(ackMsg, "CSV Failed to export!");
                        ackPopTrig = true;
                    }
                }
            }
            ImGui::Separator();

            // Edit Roll Metadata
            if (ImGui::MenuItem("Edit Roll Metadata")) {
                if (validRoll()) {
                    std::memset(&metaEdit, 0, sizeof(metaBuff));
                    globalMetaPopTrig = true;
                    activeRolls[selRoll].rollMetaPreEdit(&metaEdit);
                }

            }

            // Edit Image Metadata
            if (ImGui::MenuItem("Edit Image Metadata")) {
                //localMetaPopup = true;
                if (validIm()) {
                    std::memset(&metaEdit, 0, sizeof(metaBuff));
                    localMetaPopTrig = true;
                    imageMetaPreEdit();
                }
            }


            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Keyboard Shortcuts")) {
                shortPopTrig = true;
            }

            if (ImGui::MenuItem("User Guide")) {

            }
            if (ImGui::MenuItem("About")) {

            }

            ImGui::EndMenu();
        }

        std::vector<const char*> itemPointers;
        for (const auto& item : activeRolls) {
            itemPointers.push_back(item.rollName.c_str());
        }
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() * 0.05f));
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.25f);
                if (ImGui::Combo("###", &selRoll, itemPointers.data(), itemPointers.size())) {
                    // This is where we call the function necessary for dumping
                    // the loaded images and loading the selected images
                    for (int i = 0; i < activeRolls.size(); i++) {
                        if (selRoll != i) {
                            activeRolls[i].selected = false;
                            if (preferences.perfMode)
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
