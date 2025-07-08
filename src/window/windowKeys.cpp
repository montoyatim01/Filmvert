#include "preferences.h"
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
        imMatchPopTrig || contactPopTrig || relNotesPopTrig) {
            // Don't handle any of the key combos
            // if we have windows open already!
            return;
        }

    // Close app
    if (ImGui::IsKeyChordPressed(ImGuiKey_Q | ImGuiMod_Ctrl)) {
        wantClose = true;
    }

    // Open Image(s) (Cmd + O)
    if (ImGui::IsKeyChordPressed(ImGuiKey_O | ImGuiMod_Ctrl)) {
        if (validRoll())
            impRoll = selRoll;
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
        if (validRoll())
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
        ocioSel = ocioProc.selectedConfig;
        preferencesPopTrig = true;
        tmpPrefs = appPrefs.prefs;
    }

    // Rotate Left (Cmd + [)
    if (ImGui::IsKeyChordPressed(ImGuiKey_LeftBracket | ImGuiMod_Ctrl)) {
        if (validIm()) {
            activeImage()->rotLeft();
            renderCall = true;
            paramUpdate();
            activeRoll()->rollUpState();
        }


    }

    // Rotate Right (Cmd + ])
    if (ImGui::IsKeyChordPressed(ImGuiKey_RightBracket | ImGuiMod_Ctrl)) {
        if (validIm()) {
            activeImage()->rotRight();
            renderCall = true;
            paramUpdate();
            activeRoll()->rollUpState();
        }
    }

    // Flip Vertical (alt + shift + v)
    if (ImGui::IsKeyChordPressed(ImGuiKey_V | ImGuiMod_Shift | ImGuiMod_Alt)) {
        if (validIm()) {
            activeImage()->flipV();
            renderCall = true;
            paramUpdate();
            activeRoll()->rollUpState();
        }
    }
    // Flip Horizontal (alt + shift + h)
    if (ImGui::IsKeyChordPressed(ImGuiKey_H | ImGuiMod_Shift | ImGuiMod_Alt)) {
        if (validIm()) {
            activeImage()->flipH();
            renderCall = true;
            paramUpdate();
            activeRoll()->rollUpState();
        }
    }

    // Refresh Image
    if (ImGui::IsKeyChordPressed(ImGuiKey_R | ImGuiMod_Ctrl)) {
        if (validIm()) {
            renderCall = true;
            activeImage()->imgRst = true;
        }
    }

    // Toggle Base Color Selection
    if (ImGui::IsKeyChordPressed(ImGuiKey_B | ImGuiMod_Ctrl)) {
        if (validIm()) {
            sampleVisible = !sampleVisible;
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

    // Image rating
    if (ImGui::IsKeyChordPressed(ImGuiKey_0 | ImGuiMod_Alt)) {
        // Rating 0
        if (validIm()) {
            activeImage()->imgMeta.rating = 0;
            ratingSet = true;
            ratingFrameCount = 120;
            paramUpdate();
        }
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_1 | ImGuiMod_Alt)) {
        // Rating 1
        if (validIm()) {
            activeImage()->imgMeta.rating = 1;
            ratingSet = true;
            ratingFrameCount = 120;
            paramUpdate();
        }
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_2 | ImGuiMod_Alt)) {
        // Rating 2
        if (validIm()) {
            activeImage()->imgMeta.rating = 2;
            ratingSet = true;
            ratingFrameCount = 120;
            paramUpdate();
        }
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_3 | ImGuiMod_Alt)) {
        // Rating 3
        if (validIm()) {
            activeImage()->imgMeta.rating = 3;
            ratingSet = true;
            ratingFrameCount = 120;
            paramUpdate();
        }
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_4 | ImGuiMod_Alt)) {
        // Rating 4
        if (validIm()) {
            activeImage()->imgMeta.rating = 4;
            ratingSet = true;
            ratingFrameCount = 120;
            paramUpdate();
        }
    }
    if (ImGui::IsKeyChordPressed(ImGuiKey_5 | ImGuiMod_Alt)) {
        // Rating 5
        if (validIm()) {
            activeImage()->imgMeta.rating = 5;
            ratingSet = true;
            ratingFrameCount = 120;
            paramUpdate();
        }
    }

    if (ImGui::IsKeyChordPressed(ImGuiKey_D | ImGuiMod_Ctrl)) {
        // Metadata popup flag
        demoWin = !demoWin;
    }
}
