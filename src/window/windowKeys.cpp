#include "window.h"
#include <imgui.h>

void mainWindow::checkHotkeys() {

    // Open Image(s) (Cmd + O)
    if (ImGui::IsKeyChordPressed(ImGuiKey_O | ImGuiMod_Ctrl)) {
        openImages();
    }

    // Open Roll(s) (Cmd + Shift + O)
    if (ImGui::IsKeyChordPressed(ImGuiKey_O | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
        openRolls();
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
    if (ImGui::IsKeyChordPressed(ImGuiKey_V | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
        if (validRoll())
            activeRolls[selRoll].saveAll();
    }

    // Save All (Opt + Shift + S)
    if (ImGui::IsKeyChordPressed(ImGuiKey_V | ImGuiMod_Ctrl | ImGuiMod_Shift)) {
        for (int r = 0; r < activeRolls.size(); r++) {
            activeRolls[r].saveAll();
        }
    }

    // Preferences (Cmd + ,)
    if (ImGui::IsKeyChordPressed(ImGuiKey_Comma | ImGuiMod_Ctrl)) {
        // Preferences
        badOcioText = false;
        std::strcpy(ocioPath, preferences.ocioPath.c_str());
        if (preferences.ocioExt)
            ocioSel = 1;
        preferencesPopTrig = true;
    }

    // Rotate Left (Cmd + [)
    if (ImGui::IsKeyChordPressed(ImGuiKey_LeftBracket | ImGuiMod_Ctrl)) {
        if (validIm()) {
            activeImage()->rotLeft();
            if (activeImage()->texture) {
                SDL_DestroyTexture((SDL_Texture*)activeImage()->texture);
                activeImage()->texture = nullptr;
            }
            renderCall = true;
            paramUpdate();
        }


    }

    // Rotate Right (Cmd + ])
    if (ImGui::IsKeyChordPressed(ImGuiKey_RightBracket | ImGuiMod_Ctrl)) {
        if (validIm()) {
            activeImage()->rotRight();
            if (activeImage()->texture) {
                SDL_DestroyTexture((SDL_Texture*)activeImage()->texture);
                activeImage()->texture = nullptr;
            }
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
            activeRolls[selRoll].rollMetaPreEdit(&metaEdit);
        }
    }

    // Printer Lights?


}
