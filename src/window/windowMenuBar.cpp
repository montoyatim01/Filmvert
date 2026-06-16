#include "logger.h"
//#include "metalGPU.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "structs.h"
#include "window.h"
#include <cstring>
#include <imgui.h>

//--- Menu Bar ---//
/*
    Main menu bar routine
*/
void mainWindow::menuBar() {
    #ifdef __APPLE__
        const std::string cmd = "⌘", opt = "⌥", shift = "⇧", sep = " ";
    #else
        const std::string cmd = "Ctrl", opt = "Alt", shift = "Shift", sep = "+";
    #endif

    // BeginMainMenuBar sizes itself to exactly GetFrameHeight() pixels tall.
    // Measure it here so every hardcoded offset below tracks DPI/font changes.
    const float mainBarH = ImGui::GetFrameHeight();

    if (ImGui::BeginMainMenuBar()) {
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.75f, 0.75f, 0.75f, 0.90f));
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Image(s)", (cmd + sep + "O").c_str())) {
                if (validRoll())
                    impRoll = selRoll;
                openImages();
            }
            if (ImGui::MenuItem("Open Roll(s)", (cmd + sep + shift + sep + "O").c_str())) {
                openRolls();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Image", (cmd + sep + "S").c_str())) {
                if (validIm())
                    activeImage()->writeXMPFile();
            }
            if (altHeld) {
                if (ImGui::MenuItem("Save Roll Only", (cmd + sep + shift + sep + "S").c_str())) {
                    if (validRoll()) {
                        activeRoll()->exportRollMetaJSON();
                    }

                }
            } else {
                if (ImGui::MenuItem("Save Roll", (cmd + sep + shift + sep + "S").c_str())) {
                    if (validRoll()) {
                        activeRoll()->saveAll();
                        activeRoll()->exportRollMetaJSON();
                    }

                }
            }

            if (ImGui::MenuItem("Save All", (opt + sep + shift + sep + "S").c_str())) {
                for (int r = 0; r < activeRolls.size(); r++) {
                    activeRolls[r].saveAll();
                    activeRolls[r].exportRollMetaJSON();
                }
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Export Image(s)")) {
                if (validRoll()) {
                    exportOCIO.display = dispOCIO.display;
                    exportOCIO.view = dispOCIO.view;
                    expRolls = false;
                    exportPopup = true;
                }

            }
            if (ImGui::MenuItem("Export Roll(s)")) {
                if (activeRollSize() > 0) {
                    exportOCIO.display = dispOCIO.display;
                    exportOCIO.view = dispOCIO.view;
                    expRolls = true;
                    exportPopup = true;
                }

            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close Selected Image(s)", (cmd + sep + "W").c_str())) {
                if (validRoll()) {
                    if (activeRoll()->imagesLoading) {
                        std::strcpy(ackMsg, "Cannot close selected image(s) while there are images loading!");
                        ackPopTrig = true;
                    } else {
                        if (activeRoll()->unsavedIndividual()) {
                            closeMd = c_selIm;
                            unsavedPopTrigger = true;
                        } else {
                            activeRoll()->closeSelected();
                        }
                    }
                }
            }
            if (ImGui::MenuItem("Close Roll", (cmd + sep + shift + sep + "W").c_str())) {

                if (validRoll()) {
                    // Check if roll is loading first
                    if (activeRoll()->imagesLoading) {
                        std::strcpy(ackMsg, "Cannot close roll while images are loading!");
                        ackPopTrig = true;
                    } else {
                        // Check if unsaved, prompt
                        if (activeRoll()->unsavedImages()) {
                            // We have unsaved images, prompt user
                            closeMd = c_roll;
                            unsavedPopTrigger = true;

                        } else {
                            removeRoll();
                        }
                    }

                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences", (cmd + sep + + ",").c_str())) {
                badOcioText = false;
                std::strcpy(ocioPath, appPrefs.prefs.ocioPath.c_str());
                ocioSel = ocioProc.selectedConfig;
                preferencesPopTrig = true;
                tmpPrefs = appPrefs.prefs;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", (cmd + sep + "Q").c_str())) {
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
            if (ImGui::MenuItem("Undo", (cmd + sep + "Z").c_str())) {
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
            if (ImGui::MenuItem("Redo", (cmd + sep + shift + sep + "Z").c_str())) {
                if (validRoll()) {
                    activeRoll()->rollRedo();
                    stateRender();
                    renderCall = true;
                }

            }
            if (rDis)
                ImGui::EndDisabled();

            ImGui::Separator();

            if (ImGui::MenuItem("Select All", (cmd + sep + "A").c_str())) {
                // Select All
                if(validRoll()) {
                    int itemCount = activeRollSize();
                    for (int i = 0; i < itemCount; i++) {
                            selection.SetItemSelected(selection.GetStorageIdFromIndex(i), true);
                        }
                }
            }

            // Copy
            if (ImGui::MenuItem("Copy", (cmd + sep + "C").c_str())) {
                if (validIm()) {
                    copyIntoParams();
                }
            }
            // Paste
            if (ImGui::MenuItem("Paste", (cmd + sep + "V").c_str())) {
                if (validRoll()) {
                    pasteTrigger = true;
                }

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Metadata")) {
            // Import Roll Metadata
            if (ImGui::MenuItem("Import Roll Metadata")) {
                if (validRoll()) {
                    if (openJSON()) {
                        imMatchPopTrig = true;
                        ImMatchRoll = true;
                    } else {
                        std::string err = ackError;
                        if (!err.empty()) {
                            std::strcpy(ackMsg, "Metadata failed to import:");
                            ackPopTrig = true;
                        }
                    }
                }
            }

            // Import Image Metadata
            if (ImGui::MenuItem("Import Image Metadata")) {
                if (validIm()) {
                    if (openImageMeta()) {
                        imMatchPopTrig = true;
                    } else {
                        std::string err = ackError;
                        if (!err.empty()) {
                            std::strcpy(ackMsg, "Metadata failed to import:");
                            ackPopTrig = true;
                        }
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

            // Export Single Image JSON
            if (ImGui::MenuItem("Export Image Metadata")) {
                if (validIm()) {
                    if (activeImage()->writeJSONFile()) {
                        std::strcpy(ackMsg, "Image metadata exported to Roll directory.");
                        ackPopTrig = true;
                    } else {
                        std::strcpy(ackMsg, "Image metadata failed to export!");
                        ackPopTrig = true;
                    }
                }
            }
            ImGui::Separator();

            // Edit Roll Metadata
            if (ImGui::MenuItem("Edit Roll Metadata", (cmd + sep + "G").c_str())) {
                if (validRoll()) {
                    std::memset(&metaEdit, 0, sizeof(metaBuff));
                    globalMetaPopTrig = true;
                    activeRoll()->rollMetaPreEdit(&metaEdit);
                }

            }

            // Edit Image Metadata
            if (ImGui::MenuItem("Edit Image Metadata", (cmd + sep + "E").c_str())) {
                //localMetaPopup = true;
                if (validIm()) {
                    std::memset(&metaEdit, 0, sizeof(metaBuff));
                    localMetaPopTrig = true;
                    imageMetaPreEdit();
                }
            }


            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Roll")) {
            if (ImGui::MenuItem("Sync Roll")) {
                if (validIm()) {
                    copyIntoParams();
                    int itemCount = activeRollSize();
                    for (int i = 0; i < itemCount; i++) {
                            selection.SetItemSelected(selection.GetStorageIdFromIndex(i), true);
                    }
                    pasteTrigger = true;
                }
            }
            // Refresh Roll
            if (ImGui::MenuItem("Refresh Roll")) {
                for (int i = 0; i < activeRollSize(); i++) {
                    imgRender(getImage(i), r_bg);
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

            if (ImGui::MenuItem("Generate Contact Sheet")) {
                if (validRoll()) {
                    // Set output format to jpeg
                    expSetting.format = 2;
                    contactPopTrig = true;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Keyboard Shortcuts")) {
                shortPopTrig = true;
            }

            if (ImGui::MenuItem("Release Notes")) {
                relNotesPopTrig = true;
            }

            if (ImGui::MenuItem("About")) {
                aboutPopTrig = true;
            }

            ImGui::EndMenu();
        }
        ImGui::PopStyleColor();

        std::vector<std::string> rollNames;  // Store the actual strings
        std::vector<const char*> rollPointers;

        for (const auto& item : activeRolls) {
            std::string rollName = "";
            if (item.unsavedImages())
                rollName += "*";
            rollName += item.rollName;
            rollNames.push_back(rollName);  // Store the string
        }

        // Now create pointers to the stored strings
        for (const auto& name : rollNames) {
            rollPointers.push_back(name.c_str());
        }
        //--- Roll Selector
        if (!activeRolls.empty()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() * 0.05f));
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.25f);
            if (ImGui::Combo("###", &selRoll, rollPointers.data(), rollPointers.size())) {
                rollChange = true;
                // This is where we call the function necessary for dumping
                // the loaded images and loading the selected images
                for (int i = 0; i < activeRolls.size(); i++) {
                    if (selRoll != i) {
                        activeRolls[i].selected = false;
                        if (appPrefs.prefs.perfMode)
                            clearRoll(&activeRolls[i]);
                    }
                    if (selRoll == i) {
                        activeRollLoading = true;
                        activeRolls[i].loadBuffers();
                    }
                }
                flagVisibleImage();
            }
            ImGui::SetItemTooltip("Roll selection");
        }


        //--- RGBC Buttons

        ImGui::SameLine();
        float groupWidth = ImGui::CalcTextSize("R").x * 4;
        groupWidth += (ImGui::GetStyle().FramePadding.x * 8);
        groupWidth += (ImGui::GetStyle().ItemInnerSpacing.x * 3);
        float paramSize = winWidth - imageWinSize.x;
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.8f);// + (paramSize / 2) + (groupWidth / 2));

        if (channelView == 0 || channelView == 1) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.20f, 0.20f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.10f, 0.10f, 1.00f));
        }

        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.1f, 0.1f, 1.00f));
        }

        if (ImGui::Button("R")) {
            if (validIm()) {
                channelView = channelView != 1 ? 1 : 0;
                activeImage()->channelView = channelView;
                renderCall = true;
            }
        }
        ImGui::SetItemTooltip("Solo the red color channel (R)");
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        if (channelView == 0 || channelView == 2) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.9f, 0.20f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 1.0f, 0.10f, 1.00f));
        }

        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 1.0f, 0.1f, 1.00f));
        }
        if (ImGui::Button("G")) {
            if (validIm()) {
                channelView = channelView != 2 ? 2 : 0;
                activeImage()->channelView = channelView;
                renderCall = true;
            }
        }
        ImGui::SetItemTooltip("Solo the green color channel (G)");
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        if (channelView == 0 || channelView == 3) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.90f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.10f, 1.00f, 1.00f));
        }

        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 1.0f, 1.00f));
        }
        if (ImGui::Button("B")) {
            if (validIm()) {
                channelView = channelView != 3 ? 3 : 0;
                activeImage()->channelView = channelView;
                renderCall = true;
            }
        }
        ImGui::SetItemTooltip("Solo the blue color channel (B)");
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        if (showClip) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.10f, 0.80f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.00f, 1.00f, 1.00f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 1.0f, 1.00f));
        }
        if (ImGui::Button("K")) {
            if (validIm()) {
                showClip = !showClip;
                renderCall = true;
            }
        }
        ImGui::SetItemTooltip("Show clipping in the viewer (K)");
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        if (gradeBypass) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.80f, 0.10f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.00f, 0.00f, 1.00f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 1.0f, 1.00f));
        }
        if (ImGui::Button("D")) {
            if (validIm()) {
                gradeBypass = !gradeBypass;
                renderCall = true;
            }
        }
        ImGui::SetItemTooltip("Disable Grades (D)");
        ImGui::PopStyleColor(2);
        ImGui::SameLine();


        ImGui::SetCursorPosX((ImGui::GetWindowWidth() * 0.98f));
        std::string dropPopDownButton = dispPopDownFlag ? "v" : "<";
        if (ImGui::Button(dropPopDownButton.c_str())) {
            dispPopDownFlag = !dispPopDownFlag;
        }
        /*
        ImGui::Text("Display:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.15f);
        if (ImGui::Combo("##01", &dispOCIO.display, ocioProc.activeConfig()->displays.data(), ocioProc.activeConfig()->displays.size())) {
            renderCall |= true;
            rollRender();
        }
        ImGui::SameLine();
        ImGui::Text("View:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20f);
        if(ImGui::Combo("##02", &dispOCIO.view, ocioProc.activeConfig()->views[dispOCIO.display].data(), ocioProc.activeConfig()->views[dispOCIO.display].size())) {
            renderCall |= true;
            rollRender();
            }*/

                ImGui::EndMainMenuBar();
    }

    if (dispPopDownFlag) {
        // Use WindowPadding.y (not FramePadding.y) for vertical breathing room.
        // FramePadding.y would stretch combo/checkbox heights to fill the bar;
        // WindowPadding.y adds space around the content without touching widget sizes.
        // We skip BeginMenuBar entirely and match the background to MenuBarBg instead.
        const float padY = 6.0f;
        float barHeight = ImGui::GetFrameHeight() + padY * 2.0f;
        menuHeight = (int)ceilf(mainBarH + barHeight);
        ImGui::SetNextWindowPos(ImVec2(0, mainBarH));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, barHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
            ImVec2(ImGui::GetStyle().WindowPadding.x, padY));
        ImGui::PushStyleColor(ImGuiCol_WindowBg,
            ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        if (ImGui::Begin("##SecondaryBar", nullptr, flags)) {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() * 0.01f));
            ImGui::Text("Display:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.15f);
            std::vector<const char*> dispPtrs;
            dispPtrs.reserve(ocioProc.activeConfig()->displays.size());
            for (auto& s : ocioProc.activeConfig()->displays) dispPtrs.push_back(s.c_str());
            if (ImGui::Combo("##01", &dispOCIO.display, dispPtrs.data(), dispPtrs.size())) {
                renderCall |= true;
                rollRender();
            }
            ImGui::SetItemTooltip("OpenColorIO Display");
            ImGui::SameLine();
            ImGui::Text("  View:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20f);
            std::vector<const char*> viewPtrs;
            viewPtrs.reserve(ocioProc.activeConfig()->views[dispOCIO.display].size());
            for (auto& s : ocioProc.activeConfig()->views[dispOCIO.display]) viewPtrs.push_back(s.c_str());
            if(ImGui::Combo("##02", &dispOCIO.view, viewPtrs.data(), viewPtrs.size())) {
                renderCall |= true;
                rollRender();
            }
            ImGui::SetItemTooltip("OpenColorIO View");
            ImGui::SameLine();
            ImGui::Text("  |  ");
            if (validIm()) {
                ImGui::SameLine();
                if (ImGui::Checkbox("Bypass Render", &activeImage()->renderBypass)) {
                    renderCall |= true;
                }
                ImGui::SetItemTooltip("Disable inversion");
                ImGui::SameLine();
                ImGui::Text("  |  ");
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Show Histogram", &appPrefs.prefs.histEnable)) {
                renderCall |= true;
                uiChanges = true;
            }
            if (appPrefs.prefs.histEnable) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.10);
                if (ImGui::SliderFloat("###int", &appPrefs.prefs.histInt, 0.0f, 1.0f)) {
                    renderCall |= true;
                    uiChanges = true;
                }
                ImGui::SetItemTooltip("Histogram intensity");
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(); // WindowBg
        ImGui::PopStyleVar();   // WindowPadding
    } else {
        menuHeight = (int)ceilf(mainBarH);
    }
}
