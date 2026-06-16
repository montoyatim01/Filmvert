#include "image.h"
#include "imageMeta.h"
//#include "metalGPU.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "structs.h"
#include "updateCheck.h"
#include "window.h"
#include "utils.h"
#include "releaseNotes.h"

#include <algorithm>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>


//--- Unsaved Roll Popup ---//
/*
    Popup to notify the user that there
    are images with unsaved changes. Allows
    saving the changes before closing.

    Responsible for both roll closing,
    and application closing.
*/
void mainWindow::unsavedRollPopup() {
    if (unsavedPopTrigger)
        ImGui::OpenPopup("Unsaved Changes");
    if (ImGui::BeginPopupModal("Unsaved Changes", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("There are unsaved changes!");

        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        if (ImGui::Button("Cancel")) {
            unsavedPopTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard")) {
            switch (closeMd) {
                case c_app:
                    done = true;
                    unsavedPopTrigger = false;
                    ImGui::CloseCurrentPopup();
                    break;
                case c_roll:
                    removeRoll();
                    unsavedPopTrigger = false;
                    ImGui::CloseCurrentPopup();
                    break;
                case c_selIm:
                    if (validRoll())
                        activeRoll()->closeSelected();
                    unsavedPopTrigger = false;
                    ImGui::CloseCurrentPopup();
                    break;
            }

        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            switch (closeMd) {
                case c_app:
                    for (int r = 0; r < activeRolls.size(); r++) {
                        activeRolls[r].saveAll();
                    }
                    done = true;
                    unsavedPopTrigger = false;
                    ImGui::CloseCurrentPopup();
                    break;
                case c_roll:
                    if (validRoll()) {
                        activeRoll()->saveAll();
                        removeRoll();
                    }
                    unsavedPopTrigger = false;
                    ImGui::CloseCurrentPopup();
                    break;
                case c_selIm:
                    if (validRoll()) {
                        activeRoll()->saveSelected();
                        activeRoll()->closeSelected();
                    }
                    unsavedPopTrigger = false;
                    ImGui::CloseCurrentPopup();
                    break;
            }

        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            unsavedPopTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }


}




//--- Acknowledge Popup ---//
/*
    Simple message popup with an error field
    and an okay button
*/
void mainWindow::ackPopup() {
    if (ackPopTrig)
        ImGui::OpenPopup("Alert!");
    if (ImGui::BeginPopupModal("Alert!", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

        ImGui::Text("%s", ackMsg);
        ImGui::Text("%s", ackError);
        float windowWidth = ImGui::GetWindowWidth();
        float buttonWidth = ImGui::CalcTextSize("Okay").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float xPos = (windowWidth - buttonWidth) * 0.5f;

        ImGui::SetCursorPosX(xPos);
        if (ImGui::Button("Okay")) {
            ackPopTrig = false;
            std::memset(ackMsg, 0, sizeof(ackMsg));
            std::memset(ackError, 0, sizeof(ackError));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ackPopTrig = false;
            std::memset(ackMsg, 0, sizeof(ackMsg));
            std::memset(ackError, 0, sizeof(ackError));
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}

//--- Analyze Popup ---//
/*
    Simple message while waiting for
    analysis to finish
*/
void mainWindow::analyzePopup() {
    if (anaPopTrig)
        ImGui::OpenPopup("Analyze");
    if (ImGui::BeginPopupModal("Analyze", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

        ImGui::Text("Analyzing...");

        if (!anaPopTrig) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}



//--- Roll Import Image Selection ---//
/*
    When importing an entire roll json,
    selected which matched images to apply
*/

void mainWindow::importImMatchPopup() {
    if (imMatchPopTrig)
        ImGui::OpenPopup("Apply Metadata");
    if (ImGui::BeginPopupModal("Apply Metadata", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        if (ImMatchRoll) {
            if (ImGui::BeginChild("##Basket", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 6), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY))
            {
                for (int n = 0; n < activeRoll()->metaImp.size(); n++)
                {
                    ImGui::SetNextItemSelectionUserData(n);
                    ImGui::Checkbox(activeRoll()->metaImp[n].imName.c_str(), &activeRoll()->metaImp[n].selected);
                }
            }
            ImGui::EndChild();
            if (ImGui::Button("Toggle All")) {
                if (validRoll()) {
                    if (activeRoll()->metaImp.size() > 0) {
                        bool op = activeRoll()->metaImp[0].selected;
                        for (auto &im : activeRoll()->metaImp) {
                            im.selected = !op;
                        }
                    }
                }
            }
            ImGui::Separator();
        }


        //---Analysis---//
        ImVec2 btnSize = ImGui::CalcTextSize("Blur");
        //btnSize.x += (ImGui::GetStyle().FramePadding.x*2);
        //btnSize.y += (ImGui::GetStyle().FramePadding.y*2);

        btnSize.x += (ImGui::GetStyle().ItemSpacing.x);
        btnSize.y += (ImGui::GetStyle().ItemSpacing.y + 2);
        /*ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.38);
        if(ImGui::Button("All Analysis")) {
            pasteOptions.analysisGlobal();
        }*/

        ImGui::BeginGroup();
        ImGui::InvisibleButton("##01", btnSize);
        ImGui::Checkbox("Base Color", &metImpOpt.baseColor);
        ImGui::Checkbox("Analysis", &metImpOpt.analysis);
        ImGui::Separator();
        ImGui::InvisibleButton("##02", btnSize);
        ImGui::Checkbox("Temperature", &metImpOpt.temp);
        ImGui::Checkbox("Tint", &metImpOpt.tint);
        ImGui::Checkbox("Saturation", &metImpOpt.saturation);
        ImGui::Checkbox("Blackpoint", &metImpOpt.bp);
        ImGui::Separator();
        ImGui::InvisibleButton("##03", btnSize);
        ImGui::Checkbox("Camera Make", &metImpOpt.make);
        ImGui::Checkbox("Camera Model", &metImpOpt.model);
        ImGui::Checkbox("Lens", &metImpOpt.lens);
        ImGui::Checkbox("Film Stock", &metImpOpt.stock);
        ImGui::Checkbox("Focal Length", &metImpOpt.focal);
        ImGui::Checkbox("f Number", &metImpOpt.fstop);
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        if(ImGui::Button("All Analysis")) {
            metImpOpt.analysisGlobal();
        }
        ImGui::Checkbox("Analysis Blur", &metImpOpt.analysisBlur);
        ImGui::InvisibleButton("##04", btnSize);
        ImGui::Spacing();

        if (ImGui::Button("All Grade")) {
            metImpOpt.gradeGlobal();
        }
        ImGui::Checkbox("Whitepoint", &metImpOpt.wp);
        ImGui::Checkbox("Lift", &metImpOpt.lift);
        ImGui::Checkbox("Gain", &metImpOpt.gain);
        ImGui::Checkbox("Matrix", &metImpOpt.matrix);
        //ImGui::InvisibleButton("##04a", btnSize);
        ImGui::Spacing();

        if (ImGui::Button("All Metadata")) {
            metImpOpt.metaGlobal();
        }

        ImGui::Checkbox("Exposure", &metImpOpt.exposure);
        ImGui::Checkbox("Date/Time", &metImpOpt.date);
        ImGui::Checkbox("Location", &metImpOpt.location);
        ImGui::Checkbox("GPS", &metImpOpt.gps);
        ImGui::Checkbox("Notes", &metImpOpt.notes);
        ImGui::Checkbox("Rotation", &metImpOpt.rotation);
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::InvisibleButton("##05", btnSize);
        ImGui::Checkbox("Crop Points", &metImpOpt.cropPoints);
        ImGui::InvisibleButton("##06", btnSize);
        ImGui::Spacing();

        ImGui::InvisibleButton("##07", btnSize);
        ImGui::Checkbox("Multiply", &metImpOpt.mult);
        ImGui::Checkbox("Offset", &metImpOpt.offset);
        ImGui::Checkbox("Gamma", &metImpOpt.gamma);
        ImGui::Checkbox("Curves", &metImpOpt.curves);
        ImGui::Spacing();

        ImGui::InvisibleButton("##08", btnSize);

        ImGui::Checkbox("Development Process", &metImpOpt.dev);
        ImGui::Checkbox("Chemistry Manufacturer", &metImpOpt.chem);
        ImGui::Checkbox("Development Notes", &metImpOpt.devnote);
        ImGui::Checkbox("Scanner", &metImpOpt.scanner);
        ImGui::Checkbox("Scanner Notes", &metImpOpt.scannotes);
        ImGui::Checkbox("Crop", &metImpOpt.imageCrop);
        ImGui::InvisibleButton("##09", btnSize);
        ImGui::EndGroup();


        ImGui::Separator();

        if (ImGui::Button("Cancel")) {
            imMatchPopTrig = false;
            ImMatchRoll = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            //std::raise(SIGINT);
            // Apply metadata
            if (ImMatchRoll) {
                if (validRoll()) {
                    activeRoll()->applyRollMetaJSON(paramImp, metImpOpt);
                    rollRender();
                    std::strcpy(ackMsg, "Metadata imported to image successfully!");
                    ackPopTrig = true;
                }
            } else {
                if (validRoll()){
                    if(activeRoll()->applySelMetaJSON(imgMetImp[0], metImpOpt)) {
                        std::strcpy(ackMsg, "Metadata imported to image successfully!");
                        ackPopTrig = true;
                        rollRender();
                    }
                }
            }
            if (validRoll())
                activeRoll()->rollUpState();
            imMatchPopTrig = false;
            ImMatchRoll = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            imMatchPopTrig = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        ImGui::EndPopup();
    }
}

//--- About Popup ---//
/*
    Popup for displaying all available
    hotkeys in the application.
*/
void mainWindow::aboutPopup() {
    if (aboutPopTrig)
        ImGui::OpenPopup("About Filmvert");
    if (aboutPopTrig)
        ImGui::SetNextWindowSize(ImVec2(650, 460), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("About Filmvert", NULL, ImGuiWindowFlags_NoResize)) {
        float windowWidth = ImGui::GetWindowSize().x;
        float logoWidth = 128.0f;
        ImGui::SetCursorPosX((windowWidth - logoWidth) * 0.5f);
        ImGui::Image(static_cast<ImTextureID>(logoTex), ImVec2(logoWidth,logoWidth));

        const char* filmvertText = "Filmvert";
        float filmvertTextWidth = ImGui::CalcTextSize(filmvertText).x;
        ImGui::SetCursorPosX((windowWidth - filmvertTextWidth) * 0.5f);
        ImGui::Text("Filmvert");

        std::string versionText = fmt::format("Version {}.{}.{}", VERMAJOR, VERMINOR, VERPATCH);
        float versionTextWidth = ImGui::CalcTextSize(versionText.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - versionTextWidth) * 0.5f);
        ImGui::Text("Version %i.%i.%i", VERMAJOR, VERMINOR, VERPATCH);

        std::string hashFmt = fmt::format("{:.8}",GIT_COMMIT_HASH);
        std::string buildText = fmt::format("Build {}-{}", hashFmt, BUILD_DATE);
        float buildTextWidth = ImGui::CalcTextSize(buildText.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - buildTextWidth) * 0.5f);
        ImGui::Text("Build %s-%s", hashFmt.c_str(), BUILD_DATE);

        const char* copyrightText = "Copyright 2025 Timothy Montoya";
        float copyrightTextWidth = ImGui::CalcTextSize(copyrightText).x;
        ImGui::SetCursorPosX((windowWidth - copyrightTextWidth) * 0.5f);
        ImGui::Text("Copyright 2025 Timothy Montoya");

        const char* licenseText = "Distributed under MIT license";
        float licenseTextWidth = ImGui::CalcTextSize(licenseText).x;
        ImGui::SetCursorPosX((windowWidth - licenseTextWidth) * 0.5f);
        ImGui::Text("Distributed under MIT license");
        ImGui::Spacing();

        const char* copyText = "Copyright notices for included libraries:";
        float copyWidth = ImGui::CalcTextSize(copyText).x;
        ImGui::SetCursorPosX((windowWidth - copyWidth) * 0.5f);
        ImGui::Text("Copyright notices for included libraries:");
        ImGui::BeginChild("###Cpy", ImVec2(windowWidth - 32, 128));
        ImGui::PushID("LIC");
        ImGui::SetWindowFontScale(0.75);
        ImGui::Text("%s", licText.c_str());
        ImGui::PopID();
        ImGui::EndChild();

        float okayButtonWidth = ImGui::CalcTextSize("Okay").x;
        ImGui::SetCursorPosX((windowWidth - okayButtonWidth) * 0.5f);
        if (ImGui::Button("Okay")) {
            aboutPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            aboutPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void mainWindow::contactSheetPopup() {
    if (contactPopTrig)
        ImGui::OpenPopup("Export Contact Sheet");
    if (ImGui::BeginPopupModal("Export Contact Sheet", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("File Format:");
        ImGui::Combo("###FF", &expSetting.format, fileTypes.data(), fileTypes.size());
        ImGui::SetItemTooltip("Format to save the contact sheet as. Contact sheets will\nbe saved using the roll name & directory.");


        ImGui::Text("Image Width:");
        ImGui::InputInt("###CSW", &contactSheetWidth);
        ImGui::SetItemTooltip("How many images wide the contact sheet will be.");
        contactSheetWidth = contactSheetWidth < 1 ? 1 : contactSheetWidth > 40 ? 40 : contactSheetWidth;

        ImGui::Text("Bake Orientation:");
        ImGui::Combo("###BO", &expSetting.csBakeRot, csBake.data(), csBake.size());
        ImGui::SetItemTooltip("Bake the set orientation of the image. Allows baking only \nflips/mirrors, or baking all rotations");
        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
            contactPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            activeRoll()->generateContactSheet(contactSheetWidth, expSetting);
            contactPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}


void mainWindow::releaseNotesPopup() {
    if (relNotesPopTrig)
        ImGui::OpenPopup("Release Notes");
    if (ImGui::BeginPopupModal("Release Notes", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

        float windowWidth = 760;
        float windowHeight = 320;
        ImGui::Dummy(ImVec2(windowWidth, 2));

        ImGui::BeginChild("###RelN", ImVec2(windowWidth - 8, windowHeight));
        ImGui::PushID("REL");
        ImGui::SetWindowFontScale(0.75);

        ImGui::PushFont(ft_header);
        ImGui::Text("Version 1.2.0");
        ImGui::PopFont();
        ImGui::Text("%s", relNotes_1_2_0.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushFont(ft_header);
        ImGui::Text("Version 1.1.2");
        ImGui::PopFont();
        ImGui::Text("%s", relNotes_1_1_2.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushFont(ft_header);
        ImGui::Text("Version 1.1.1");
        ImGui::PopFont();
        ImGui::Text("%s", relNotes_1_1_1.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushFont(ft_header);
        ImGui::Text("Version 1.1.0");
        ImGui::PopFont();
        ImGui::Text("%s", relNotes_1_1_0.c_str());
        ImGui::Spacing();
        ImGui::PopID();
        ImGui::EndChild();

        if (ImGui::Button("Okay") || ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            relNotesPopTrig = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        ImGui::EndPopup();
    }
}


void mainWindow::newReleasePopup() {
    if (newReleasePopTrig && !relNotesPopTrig)
        ImGui::OpenPopup("New Version Available");
    if (ImGui::BeginPopupModal("New Version Available", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        const float popupW = 800.0f;

        // Anchor the popup width
        ImGui::Dummy(ImVec2(popupW, 1));

        // --- Centered version header ---
        // PushFont + SetWindowFontScale BEFORE CalcTextSize so the measurement
        // matches what ImGui will actually render.
        ImGui::PushFont(ft_header);
        ImGui::SetWindowFontScale(1.3f);
        std::string verLabel = fmt::format("Version {}", newVersion);
        float textW = ImGui::CalcTextSize(verLabel.c_str()).x;
        ImGui::SetCursorPosX((popupW - textW) * 0.5f);
        ImGui::TextUnformatted(verLabel.c_str());
        ImGui::PopFont();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Release notes label ---
        ImGui::PushFont(ft_header);
        ImGui::TextUnformatted("Release Notes");
        ImGui::PopFont();
        ImGui::Spacing();

        // --- Scrollable release notes body ---
        ImGui::BeginChild("###newV", ImVec2(popupW - 4, 240));
        ImGui::Spacing();
        ImGui::TextWrapped("%s", newVersionBody.c_str());
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Button row: Dismiss (left) | Download Latest (right) ---
        if (ImGui::Button("Dismiss") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            newReleasePopTrig = false;
            appPrefs.prefs.clickThrough = true;
            appPrefs.prefs.lastCheck = currentEpoch();
            appPrefs.saveToFile();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        const float downloadW = ImGui::CalcTextSize("Download Latest").x
                              + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetCursorPosX(popupW - downloadW);
        if (ImGui::Button("Download Latest")) {
            #if defined(_WIN32)
                system(fmt::format("start {}", downloadURL).c_str());
            #elif defined(__APPLE__)
                system(fmt::format("open {}", downloadURL).c_str());
            #else
                system(fmt::format("xdg-open {}", downloadURL).c_str());
            #endif
            newReleasePopTrig = false;
            appPrefs.prefs.clickThrough = true;
            appPrefs.prefs.lastCheck = currentEpoch();
            appPrefs.saveToFile();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemTooltip("%s", downloadURL);

        ImGui::Spacing();
        ImGui::EndPopup();
    }
}
