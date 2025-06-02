#include "window.h"
#include <imgui.h>

//--- Check Hotkeys ---//
/*
    Called from the main loop, check if the user has
    pressed any of the assigned hotkeys.
*/
void mainWindow::checkHotkeys() {
    if (unsavedPopTrigger || globalMetaPopTrig ||
        localMetaPopTrig || preferencesPopTrig ||
        ackPopTrig || anaPopTrig || shortPopTrig ||
        imMatchPopTrig) {
            // Don't handle any of the key combos
            // if we have windows open already!
            return;
        }

    // Open Image(s) (Cmd + O)
    if (ImGui::IsKeyChordPressed(ImGuiKey_O | ImGuiMod_Ctrl)) {
        openImages();
    }

    // Open Roll(s) (Cmd + Shift + O)
    if (ImGui::IsKeyChordPressed(ImGuiKey_O | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
        openRolls();
    }

    // Undo (Cmd + Z)
    if (ImGui::IsKeyChordPressed(ImGuiKey_Z | ImGuiMod_Ctrl)) {
        if (validRoll()) {
            activeRoll()->rollUndo();
            stateRender();
            renderCall = true;
        }
    }

    // Redo (Cmd + Shift + Z)
    if (ImGui::IsKeyChordPressed(ImGuiKey_Z | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
        if (validRoll()) {
            activeRoll()->rollRedo();
            stateRender();
            renderCall = true;
        }
    }

    // Copy (Cmd + C)
    if (ImGui::IsKeyChordPressed(ImGuiKey_C | ImGuiMod_Ctrl)) {
        // Copy
        //LOG_INFO("Copy from: {}", selIm);
        if (validIm()) {
            copyIntoParams();
        }
    }

    // Paste (Cmd + V)
    if (ImGui::IsKeyChordPressed(ImGuiKey_V | ImGuiMod_Ctrl)) {
        // Paste
        pasteTrigger = true;
    }

    // Save Image (Cmd + S)
    if (ImGui::IsKeyChordPressed(ImGuiKey_S | ImGuiMod_Ctrl)) {
        if (validIm())
            activeImage()->writeXMPFile();
    }

    // Save Roll (Cmd + Shift + S)
    if (ImGui::IsKeyChordPressed(ImGuiKey_S | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
        if (validRoll()) {
            activeRoll()->saveAll();
            activeRoll()->exportRollMetaJSON();
        }
    }

    // Save All (Opt + Shift + S)
    if (ImGui::IsKeyChordPressed(ImGuiKey_S | ImGuiMod_Alt | ImGuiMod_Shift)) {
        for (int r = 0; r < activeRolls.size(); r++) {
            activeRolls[r].saveAll();
            activeRolls[r].exportRollMetaJSON();
        }
    }

    // Close Image (Cmd + W)
    if (ImGui::IsKeyChordPressed(ImGuiKey_W | ImGuiMod_Ctrl)) {
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

    // Close Roll (Cmd + Shift + W)
    if (ImGui::IsKeyChordPressed(ImGuiKey_W | ImGuiMod_Shift | ImGuiMod_Ctrl)) {
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

    // Preferences (Cmd + ,)
    if (ImGui::IsKeyChordPressed(ImGuiKey_Comma | ImGuiMod_Ctrl)) {
        // Preferences
        badOcioText = false;
        std::strcpy(ocioPath, appPrefs.prefs.ocioPath.c_str());
        if (appPrefs.prefs.ocioExt)
            ocioSel = 1;
        preferencesPopTrig = true;
    }

    // Rotate Left (Cmd + [)
    if (ImGui::IsKeyChordPressed(ImGuiKey_LeftBracket | ImGuiMod_Ctrl)) {
        if (validIm()) {
            activeImage()->rotLeft();
            renderCall = true;
            paramUpdate();
        }


    }

    // Rotate Right (Cmd + ])
    if (ImGui::IsKeyChordPressed(ImGuiKey_RightBracket | ImGuiMod_Ctrl)) {
        if (validIm()) {
            activeImage()->rotRight();
            renderCall = true;
            paramUpdate();
        }

    }

    // Open Image Metadata (Cmd + E)
    if (ImGui::IsKeyChordPressed(ImGuiKey_E | ImGuiMod_Ctrl)) {
        // Metadata popup flag
        if (validIm()) {
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            localMetaPopTrig = true;
            imageMetaPreEdit();
        }
    }

    // Open Roll Metadata (Cmd + G)
    if (ImGui::IsKeyChordPressed(ImGuiKey_G | ImGuiMod_Ctrl)) {
        // Metadata popup flag
        if (validRoll()) {
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            globalMetaPopTrig = true;
            activeRoll()->rollMetaPreEdit(&metaEdit);
        }
    }

    // Printer Lights?


}
