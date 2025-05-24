#include "logger.h"
#include "ocioProcessor.h"
#include "structs.h"
#include "window.h"
#include <cstring>
#include <imgui.h>

//--- Menu Bar ---//
/*
    Main menu bar routine
*/
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
                    activeRoll()->saveAll();
                    activeRoll()->exportRollMetaJSON();
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
                //exportOCIO.display = dispOCIO.display;
                //exportOCIO.view = dispOCIO.view;
                expRolls = false;
                exportPopup = true;
            }
            if (ImGui::MenuItem("Export Roll(s)")) {
               // exportOCIO.display = dispOCIO.display;
               // exportOCIO.view = dispOCIO.view;
                expRolls = true;
                exportPopup = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close Selected Image(s)")) {
                if (validRoll()) {
                    if (activeRoll()->unsavedIndividual()) {
                        closeMd = c_selIm;
                        unsavedPopTrigger = true;
                    } else {
                        activeRoll()->closeSelected();
                    }

                }
            }
            if (ImGui::MenuItem("Close Roll")) {
                //closeRoll;
                //Actions needed to close a roll
                //Check if unsaved, prompt
                if (validRoll()) {
                    if (activeRoll()->unsavedImages()) {
                        // We have unsaved images, prompt user
                        closeMd = c_roll;
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
                std::strcpy(ocioPath, appPrefs.prefs.ocioPath.c_str());
                if (appPrefs.prefs.ocioExt)
                    ocioSel = 1;
                preferencesPopTrig = true;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                if (unsavedChanges()) {
                    closeMd = c_app;
                    unsavedPopTrigger = true;
                } else {
                    done = true;
                }

            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            bool uDis = false;
            bool rDis = false;
            if (!validRoll() || !activeRoll()->rollUAvail())
                uDis = true;
            if (uDis)
                ImGui::BeginDisabled();
            if (ImGui::MenuItem("Undo")) {
                if (validRoll()) {
                    activeRoll()->rollUndo();
                    stateRender();
                    renderCall = true;
                }

            }
            if (uDis)
                ImGui::EndDisabled();

            if (!validRoll() || !activeRoll()->rollRAvil())
                rDis = true;
            if (rDis)
                ImGui::BeginDisabled();
            if (ImGui::MenuItem("Redo")) {
                if (validRoll()) {
                    activeRoll()->rollRedo();
                    stateRender();
                    renderCall = true;
                }

            }
            if (rDis)
                ImGui::EndDisabled();

            ImGui::Separator();

            if (ImGui::MenuItem("Select All")) {
                // Select All
                if(validRoll()) {
                    int itemCount = activeRollSize();
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

            ImGui::Separator();

            // Refresh Roll
            if (ImGui::MenuItem("Refresh Roll")) {
                for (int i = 0; i < activeRollSize(); i++) {
                    imgRender(getImage(i));
                }
            }

            // Sort Roll by Index
            if (ImGui::MenuItem("Sort Roll by Index")) {
                if (validRoll()) {
                    if (!activeRoll()->sortRoll()) {
                        std::strcpy(ackMsg, "Unable to sort roll!");
                        std::strcpy(ackError, "One or more images is in the render queue.\nWait a moment and try again.");
                        ackPopTrig = true;
                    }
                }
            }
            // Undo/Redo?
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Metadata")) {
            // Import Roll Metadata
            if (ImGui::MenuItem("Import Roll Metadata")) {
                if (openJSON()) {
                    std::strcpy(ackMsg, "Metadata imported to Roll successfully!");
                    if (validRoll())
                        activeRoll()->rollUpState();
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
                    if(activeRoll()->exportRollMetaJSON()) {
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
                    if (activeRoll()->exportRollCSV()) {
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
                    activeRoll()->rollMetaPreEdit(&metaEdit);
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

        std::vector<const char*> rollPointers;
        for (const auto& item : activeRolls) {
            rollPointers.push_back(item.rollName.c_str());
        }
        //--- Roll Selector
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() * 0.05f));
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.25f);
                if (ImGui::Combo("###", &selRoll, rollPointers.data(), rollPointers.size())) {
                    // This is where we call the function necessary for dumping
                    // the loaded images and loading the selected images
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
                ImGui::SetItemTooltip("Roll selection");

        //--- OCIO Settings


        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() * 0.05f));
        ImGui::Text("Display:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.15f);
        renderCall |= ImGui::Combo("##01", &dispOCIO.display, ocioProc.displays.data(), ocioProc.displays.size());
        ImGui::SameLine();
        ImGui::Text("View:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20f);
        renderCall |= ImGui::Combo("##02", &dispOCIO.view, ocioProc.views[dispOCIO.display].data(), ocioProc.views[dispOCIO.display].size());

                ImGui::EndMainMenuBar();
    }
}
