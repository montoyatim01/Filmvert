#include "fmt/format.h"
#include "gpuStructs.h"
#include "image.h"
#include "structs.h"
#include "window.h"
#include "tether.h"
#include <imgui.h>
#include <string>
#include <variant>

#ifdef TETHEREN

void mainWindow::fvCamDispWidget(fvCamParam* param, fvTether* cam, std::string disp, bool sameLine) {
    if (!param)
        return;
    if (!param->validParam)
        return;
    ImGui::PushID(disp.c_str());
    ImGui::SetWindowFontScale(std::clamp(ImGui::GetWindowWidth() / 600.0f, 0.75f, 1.15f));
    if (param->paramType == GP_WIDGET_RADIO) {
        float paddingVal = 5.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(20, 20, 20, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

        ImGui::BeginChild(disp.c_str(), ImVec2(0,0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
        // Top Padding
        ImGui::Dummy(ImVec2(0.0f, paddingVal));

        // Left Side Padding Group
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(paddingVal, paddingVal));
        ImGui::EndGroup();

        ImGui::SameLine();

        // Main Content
        ImGui::BeginGroup();
        // Header
        ImGui::PushFont(ft_header);
        float availableWidth = ImGui::GetContentRegionAvail().x + (2 * paddingVal);
        float headerTextWidth = ImGui::CalcTextSize(disp.c_str()).x;
        ImGui::SetCursorPosX((availableWidth - headerTextWidth) * 0.5f);
        ImGui::Text("%s", disp.c_str());
        ImGui::PopFont();
        // Value
        ImGui::PushFont(ft_control);
        float valueTextWidth = ImGui::CalcTextSize(param->validValues[param->selVal]).x;
        ImGui::SetCursorPosX((availableWidth - valueTextWidth) * 0.5f);
        ImGui::Text("%s", param->validValues[param->selVal]);

        // Buttons
        float buttonWidth = ImGui::CalcTextSize("<").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalButtonWidth = buttonWidth * 2 + spacing;
        ImGui::SetCursorPosX((availableWidth - totalButtonWidth) * 0.5f);
        if (ImGui::Button("<")) {
            param->selVal--;
            param->selVal = param->selVal < 0 ? 0 : param->selVal;
            cam->setValue(param->paramName.c_str(), param->validValues[param->selVal]);
        }
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            param->selVal++;
            param->selVal = param->selVal >= param->validValues.size() ? param->validValues.size() - 1 : param->selVal;
            cam->setValue(param->paramName.c_str(), param->validValues[param->selVal]);
        }
        ImGui::PopFont();
        ImGui::EndGroup();
        ImGui::SameLine();

        // Right Side Padding
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(paddingVal, paddingVal));
        ImGui::EndGroup();

        // Bottom Padding
        ImGui::Dummy(ImVec2(0.0f, paddingVal));

        ImGui::EndChild();
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor();

    } else if (param->paramType == GP_WIDGET_TOGGLE) {
        if(ImGui::Button(disp.c_str())) {
            cam->setValue(param->paramName.c_str(), "1");
        }
    }

    if (sameLine)
        ImGui::SameLine();
    ImGui::PopID();

}

void mainWindow::setupTetherImage() {

    tetherPreview.srcFilename = "Live View";
    tetherPreview.imageLoaded = false;
    tetherPreview.setCrop(0.0f);

    tetherPreview.intOCIOSet.colorspace = 0;
    tetherPreview.intOCIOSet.display = 0;
    tetherPreview.intOCIOSet.view = 0;
    tetherPreview.intOCIOSet.useDisplay = 1;
    tetherPreview.intOCIOSet.inverse = 1;
}

void mainWindow::tetherCamPopup() {
    if (tetherCamPop)
        ImGui::OpenPopup("Connect Camera");
    if (ImGui::BeginPopupModal("Connect Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

        ImGui::Text("Available Cameras:");
        ImGui::Combo("###CCO", &gblTether.selCam, gblTether.camList.data(), gblTether.camList.size());
        if (ImGui::Button("Refresh")) {
            gblTether.detectCameras();
        }

        ImGui::Separator();

        if (ImGui::Button("Cancel")) {
            tetherCamPop = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Connect")) {
            gblTether.connectCamera(gblTether.selCam);
            tetherCamPop = false;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            tetherCamPop = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}

void mainWindow::tetherRollPopup() {
    if (tetherRollPop)
        ImGui::OpenPopup("New Roll");
    if (ImGui::BeginPopupModal("New Roll", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Roll Name: ");
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("###rName", "Roll Name", rollNameBuf, IM_ARRAYSIZE(rollNameBuf));
        ImGui::Text("Roll Directory (for metadata saving):");
        ImGui::InputText("###rPath", rollPath, IM_ARRAYSIZE(rollPath));
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            auto result = ShowFolderSelectionDialog(false);
            if (!result.empty())
                strcpy(rollPath, result[0].c_str());
        }
        if (ImGui::Button("Cancel")) {
            tetherRollPop = false;
            std::memset(rollNameBuf, 0, sizeof(rollNameBuf));
            std::memset(rollPath, 0, sizeof(rollPath));
            ImGui::CloseCurrentPopup();
        }
        bool createDisabled = false;
        ImGui::SameLine();
        if (std::string(rollNameBuf).empty() || std::string(rollPath).empty()) {
            createDisabled = true;
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Create Roll")) {
            activeRolls.emplace_back(filmRoll(rollNameBuf));
            activeRolls.back().rollPath = rollPath;
            std::memset(rollPath, 0, sizeof(rollPath));
            std::memset(rollNameBuf, 0, sizeof(rollNameBuf));
            impRoll = activeRolls.size() - 1;

            tetherRollPop = false;
            ImGui::CloseCurrentPopup();
        }
        if (createDisabled) {
            ImGui::EndDisabled();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}

void mainWindow::tetherSettings() {

    ImGui::Text("Connected Camera: %s", gblTether.fvCamInfo.model.c_str());
    ImGui::Text("Battery Level: %s", gblTether.fvCamInfo.battery);
    if (ImGui::Button("Disconnect")) {
        gblTether.disconnectCamera();
        if (validRoll()) {
            if (activeRoll()->images.size() > 0 && activeRoll()->images.front().isTetherLive) {
                if (activeRoll()->images.front().rawImgData) {
                    delete [] activeRoll()->images.front().rawImgData;
                    activeRoll()->images.front().rawImgData = nullptr;
                }
                activeRoll()->images.pop_front();
            }
        }
    }
    ImGui::SameLine();
    if (!gblTether.streaming) {
        if (ImGui::Button("Start Live View")) {
            gblTether.startLiveView();
            if (validRoll()) {
                if (activeRoll()->images.size() < 1 || !activeRoll()->images.front().isTetherLive) {
                    activeRoll()->images.emplace_front(tetherPreview);
                    gblTether.setImage(&activeRoll()->images.front());
                    activeRoll()->images.front().imgState.setPtrs(&activeRoll()->images.front().imgMeta, &activeRoll()->images.front().imgParam, &activeRoll()->images.front().needRndr);
                    activeRoll()->images.front().imgMeta.rollName = activeRolls[impRoll].rollName;
                    activeRoll()->images.front().imgMeta.frameNumber = 0;
                    activeRoll()->images.front().rollPath = activeRolls[impRoll].rollPath;
                    activeRoll()->images.front().selected = true;
                    activeRoll()->selIm = 0;
                }

            }
        }

    } else {
        if (ImGui::Button("Stop Live View")) {
            gblTether.stopLiveView();
            if (validRoll()) {
                if (activeRoll()->images.size() > 0 && activeRoll()->images.front().isTetherLive) {
                    if (activeRoll()->images.front().rawImgData) {
                        delete [] activeRoll()->images.front().rawImgData;
                        activeRoll()->images.front().rawImgData = nullptr;
                    }
                    activeRoll()->images.pop_front();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Capture File")) {
            if (validRoll()) {
                if (gblTether.testCapture(activeRoll()->rollPath)) {
                    // Test capture succeeded
                    if (activeRoll()->images.size() > 1 && activeRoll()->images[1].srcFilename == "tempCap") {
                        // Trigger reload on temp cap
                        activeRoll()->images[1].debayerImage(0, 11);
                        activeRoll()->images[1].imgRst = true;
                        imgRender(&activeRoll()->images[1], r_sdt);
                    } else {
                        // Raw images shouldn't need these structures
                        // so initialize base and pass them anyway
                        rawSetting rawSet;
                        ocioSetting ocioSet;
                        auto newIm = readImage(activeRoll()->rollPath + "/tempCap", rawSet, ocioSet);
                        auto insertPos = activeRoll()->images.begin();
                        int pos = 0;
                        if (activeRoll()->images.size() > 0) {
                            insertPos += 1;
                            pos += 1;
                        }

                        if (std::holds_alternative<image>(newIm)) {
                            activeRoll()->images.insert(insertPos, std::get<image>(newIm));
                            image* img = getImage(pos);
                            imgRender(img, r_sdt);
                            img->imgState.setPtrs(&img->imgMeta, &img->imgParam, &img->needRndr);
                            img->imgMeta.rollName = activeRolls[impRoll].rollName;
                            img->imgMeta.frameNumber = pos;
                            img->rollPath = activeRoll()->rollPath;
                        } else
                            LOG_ERROR("Unable to add temp capture");
                    }
                }
            }
        }

        // In the loop call for another image from the live view
        if (validRoll() && activeRoll()->images.size() > 0) {
            if (activeImage() != &activeRoll()->images.front() && activeRoll()->images.front().isTetherLive) {
                // if we've selected another image
                // don't worry about updating the live view
                // Also flag to Image Viewer that we're on
                // a regular image
                tetherImage = false;
            } else {
                gblTether.capTrigger();
                // Flag to viewer we're on tether live view (disable analysis region)
                tetherImage = true;
            }
        }

    }

    // Control Buttons
    fvCamDispWidget(&gblTether.fvCamInfo.p_autoFocusDrive, &gblTether, "Autofocus", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_viewfinder, &gblTether, "Viewfinder", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_capture, &gblTether, "Capture", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_uiLock, &gblTether, "UI Lock");

    ImGui::Separator();
    // Settings
    ImGui::Text("Basic Controls:");
    fvCamDispWidget(&gblTether.fvCamInfo.p_shutterSpeed, &gblTether, "Shutter Speed", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_aperture, &gblTether, "Aperture", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_iso, &gblTether, "ISO");

    fvCamDispWidget(&gblTether.fvCamInfo.p_whiteBalance, &gblTether, "White Balance", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_colorTemp, &gblTether, "Color Temperature");

    ImGui::Text("Advanced Controls:");
    fvCamDispWidget(&gblTether.fvCamInfo.p_wbAdjA, &gblTether, "Blue/Red", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_wbAdjB, &gblTether, "Magenta/Green");
    //fvCamDispWidget(&gblTether.fvCamInfo.p_wbXa, &gblTether, "WB X A");
    //fvCamDispWidget(&gblTether.fvCamInfo.p_wbXb, &gblTether, "WB X B");

    ImGui::Separator();
    ImGui::Text("Drive Modes:");
    fvCamDispWidget(&gblTether.fvCamInfo.p_focusMode, &gblTether, "Focus Mode", true);
    /*if (gblTether.fvCamInfo.p_focusMode.validParam) {
        const std::string value = gblTether.fvCamInfo.p_focusMode.paramVal;
        if (value == "Manual") {
            ImGui::Combo("###AFS", &gblTether.fvCamInfo.p_manualFocusDrive.selVal,
                gblTether.fvCamInfo.p_manualFocusDrive.validValues.data(),
                gblTether.fvCamInfo.p_manualFocusDrive.validValues.size());
            if (ImGui::Button("Focus Step")) {
                gblTether.setValue("manualfocusdrive", gblTether.fvCamInfo.p_manualFocusDrive.validValues[gblTether.fvCamInfo.p_manualFocusDrive.selVal]);
                }
        }
        }*/
    fvCamDispWidget(&gblTether.fvCamInfo.p_continuousAF, &gblTether, "Continuous AF", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_afmethod, &gblTether, "AF Method");
    fvCamDispWidget(&gblTether.fvCamInfo.p_driveMode, &gblTether, "Drive Mode", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_meterMode, &gblTether, "Meter Mode");

    ImGui::Separator();
    ImGui::Text("Settings:");
    fvCamDispWidget(&gblTether.fvCamInfo.p_output, &gblTether, "Output", true);
    //fvCamDispWidget(&gblTether.fvCamInfo.p_evfMode, &gblTether, "EVF Mode");
    fvCamDispWidget(&gblTether.fvCamInfo.p_capturetarget, &gblTether, "Capture Target", true);
    fvCamDispWidget(&gblTether.fvCamInfo.p_imageFormat, &gblTether, "Image Format");
    fvCamDispWidget(&gblTether.fvCamInfo.p_lvSize, &gblTether, "Live-view Size");



}


#endif
