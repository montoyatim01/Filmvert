#include "preferences.h"
#include "structs.h"
#include "window.h"
#include "windowUtils.h"
#include "osMacros.h"
#include <imgui.h>
#include <algorithm>



//--- Main Parameter View Routine ---//
void mainWindow::paramView() {

    bool paramChange = false;
    const float pBGcol[4]{appPrefs.prefs.paramBGColor[0], appPrefs.prefs.paramBGColor[1], appPrefs.prefs.paramBGColor[2], appPrefs.prefs.paramBGColor[3]};
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(pBGcol[0], pBGcol[1], pBGcol[2], pBGcol[3]));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(pBGcol[0] > 0.75f ? 0.0f : 1.0f,
                                        pBGcol[1] > 0.75f ? 0.0f : 1.0f,
                                        pBGcol[2] > 0.75f ? 0.0f : 1.0f,
                                        1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(pBGcol[0] > 0.75f ? 0.6f : 0.3f,
                                        pBGcol[1] > 0.75f ? 0.6f : 0.3f,
                                        pBGcol[2] > 0.75f ? 0.6f : 0.3f,
                                        1.0f));
    ImGui::SetNextWindowPos(ImVec2(imageWinSize.x, menuHeight));
    ImGui::SetNextWindowSize(ImVec2(winWidth - imageWinSize.x, imageWinSize.y - (0)));

    ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 ctrlSize = ImGui::GetWindowSize();
        int histHeight = appPrefs.prefs.histEnable ? 128 : 0;
        int histSecHeight = histHeight + 4;
        histSecHeight += (ImGui::CalcTextSize("Text").y);
        histSecHeight += (ImGui::GetStyle().FramePadding.y * 4);
        //histSecHeight += (ImGui::GetStyle().ItemInnerSpacing.y * 4);

        ctrlSize.x = ctrlSize.x - (ImGui::GetStyle().WindowPadding.x * 2);
        ctrlSize.y = ctrlSize.y - histSecHeight;
        ctrlSize.y = ctrlSize.y - (ImGui::GetStyle().FramePadding.y * 2);
        ctrlSize.y = ctrlSize.y - (ImGui::GetStyle().WindowPadding.y * 2);

        int colorPickerFormat = appPrefs.prefs.colorPicker == 0 ? ImGuiColorEditFlags_PickerHueWheel : ImGuiColorEditFlags_PickerHueBar;

        ImGui::SetWindowFontScale(std::clamp(ImGui::GetWindowWidth() / 600.0f, 0.75f, 1.15f));

        // sideMargin: equal space on both sides of each child.
        // SetCursorPosX shifts the left edge inward by sideMargin;
        // width = availableX - sideMargin subtracts another sideMargin on the right.
        const float sideMargin   = 6.0f;
        const float childPadding = 8.0f;
        const float childRound   = 6.0f;

        const ImVec4 childBg     = ImVec4(pBGcol[0] > 0.75 ? pBGcol[0] - 0.2 : pBGcol[0] + 0.06,
                                            pBGcol[1] > 0.75 ? pBGcol[1] - 0.2 : pBGcol[1] + 0.06,
                                            pBGcol[2] > 0.75 ? pBGcol[2] - 0.2 : pBGcol[2] + 0.06,
                                            0.94f);
        const ImVec4 childBorder = ImVec4(0.43f, 0.43f, 0.43f, 0.5f);
        const float childDedent  = ImGui::GetStyle().IndentSpacing * 0.75f;
        const float resetWidth = ImGui::CalcTextSize("Reset").x + (ImGui::GetStyle().FramePadding.x * 2) + ImGui::GetStyle().ItemInnerSpacing.x;
        const bool skinnySlider = ctrlSize.x < 260.0f ? true : false;
        //---CONTROLS CHILD---//
        ImGui::BeginChild("Controls##02", ctrlSize, ImGuiChildFlags_None);
        {
        ImGui::Spacing();
        /* IMAGE GEOMETRY */
        if (validIm()) {
            if (TreeNodeWithLine("Geometry")) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
                ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
                ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
                ImGui::Unindent(childDedent);
                ImGui::BeginChild("##child_geo", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                {
                    const float innerW   = ImGui::GetContentRegionAvail().x;
                    const float sp       = ImGui::GetStyle().ItemSpacing.x;
                    const float fp       = ImGui::GetStyle().FramePadding.x * 2.0f;
                    const float rstW     = ImGui::CalcTextSize("Reset").x + fp;
                    const float sliderW  = -(rstW + sp);
                    const float sliderPx = innerW - rstW - sp;

                    // — Rotate buttons, centered —
                    const float rotLW  = ImGui::CalcTextSize("Rotate Left").x + fp;
                    const float rotRW  = ImGui::CalcTextSize("Rotate Right").x + fp;
                    ImGui::SetCursorPosX((innerW - rotLW - sp - rotRW) * 0.5f);
                    if (ImGui::Button("Rotate Left", ImVec2(rotLW, 0))) {
                        if (validIm()) { activeImage()->rotLeft(); renderCall = true; paramChange = true; activeRoll()->rollUpState(); }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Rotate Right", ImVec2(rotRW, 0))) {
                        if (validIm()) { activeImage()->rotRight(); renderCall = true; paramChange = true; activeRoll()->rollUpState(); }
                    }

                    // — Flip buttons, centered —
                    const float flipVW = ImGui::CalcTextSize("Flip Vertically").x + fp;
                    const float flipHW = ImGui::CalcTextSize("Flip Horizontally").x + fp;
                    ImGui::SetCursorPosX((innerW - flipVW - sp - flipHW) * 0.5f);
                    if (ImGui::Button("Flip Vertically", ImVec2(flipVW, 0))) {
                        if (validIm()) { activeImage()->flipV(); renderCall = true; paramChange = true; activeRoll()->rollUpState(); }
                    }
                    ImGui::SetItemTooltip("Flip the image vertically");
                    ImGui::SameLine();
                    if (ImGui::Button("Flip Horizontally", ImVec2(flipHW, 0))) {
                        if (validIm()) { activeImage()->flipH(); renderCall = true; paramChange = true; activeRoll()->rollUpState(); }
                    }
                    ImGui::SetItemTooltip("Flip the image horizontally");

                    // — Crop controls —
                    if (validIm()) {
                        ImGui::Spacing();
                        if (activeImage()->imgParam.cropEnable) {
                            // Crop applied — show "Show Crop" centered
                            const float showCropW = ImGui::CalcTextSize("Show Crop").x + fp;
                            ImGui::SetCursorPosX((innerW - showCropW) * 0.5f);
                            if (ImGui::Button("Show Crop", ImVec2(showCropW, 0))) {
                                paramChange |= true; cropVisible = true; activeImage()->imgParam.cropEnable = false;
                            }
                        } else if (cropVisible) {
                            // Crop overlay visible — Hide / Apply / Reset, centered
                            const float hideCropW  = ImGui::CalcTextSize("Hide Crop").x + fp;
                            const float applyCropW = ImGui::CalcTextSize("Apply Crop").x + fp;
                            const float rstCropW   = ImGui::CalcTextSize("Reset Crop").x + fp;
                            ImGui::SetCursorPosX((innerW - hideCropW - sp - applyCropW - sp - rstCropW) * 0.5f);
                            if (ImGui::Button("Hide Crop", ImVec2(hideCropW, 0))) {
                                paramChange |= true; cropVisible = false; activeImage()->imgParam.cropEnable = false;
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Apply Crop", ImVec2(applyCropW, 0))) {
                                paramChange |= true; cropVisible = false; activeImage()->imgParam.cropEnable = true;
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Reset Crop", ImVec2(rstCropW, 0))) {
                                paramChange |= true;
                                activeImage()->imgParam.imageCropMinX = 0.0f;
                                activeImage()->imgParam.imageCropMinY = 0.0f;
                                activeImage()->imgParam.imageCropMaxX = 1.0f;
                                activeImage()->imgParam.imageCropMaxY = 1.0f;
                                activeImage()->imgParam.arbitraryRotation = 0.0f;
                                activeImage()->imgParam.lockAspect = false;
                                activeImage()->imgParam.imageCropAspect = activeImage()->width > activeImage()->height ?
                                    (float)activeImage()->width / activeImage()->height :
                                    (float)activeImage()->height / activeImage()->width;
                            }

                            // Rotation — centered label + drag + reset
                            ImGui::Spacing();
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Rotation").x) * 0.5f);
                            ImGui::Text("Rotation");
                            ImGui::SetNextItemWidth(sliderW);
                            paramChange |= ImGui::DragFloat("##ARot", &activeImage()->imgParam.arbitraryRotation, 0.01f, -45.0f, 45.0f);
                            ImGui::SameLine();
                            if (ImGui::Button("Reset##Rot")) {
                                int rW = activeImage()->width, rH = activeImage()->height;
                                int rRot = activeImage()->imgParam.rotation;
                                if (rRot == 5 || rRot == 6 || rRot == 7 || rRot == 8) std::swap(rW, rH);
                                activeImage()->imgParam.imageCropAspect = rW > rH
                                    ? (float)rW / rH : (float)rH / rW;
                                updateCrop = true;
                                paramChange = true;
                            }

                            // Lock aspect checkbox
                            ImGui::Spacing();
                            if (ImGui::Checkbox("Lock Aspect Ratio", &activeImage()->imgParam.lockAspect)) {
                                if (activeImage()->imgParam.lockAspect) {
                                    int dispW = activeImage()->width;
                                    int dispH = activeImage()->height;
                                    int rot   = activeImage()->imgParam.rotation;
                                    if (rot == 5 || rot == 6 || rot == 7 || rot == 8)
                                        std::swap(dispW, dispH);

                                    float normW = std::abs(activeImage()->imgParam.imageCropMaxX - activeImage()->imgParam.imageCropMinX);
                                    float normH = std::abs(activeImage()->imgParam.imageCropMaxY - activeImage()->imgParam.imageCropMinY);
                                    float pixW  = normW * (float)dispW;
                                    float pixH  = normH * (float)dispH;
                                    if (pixH > 0.0001f && pixW > 0.0001f)
                                        activeImage()->imgParam.imageCropAspect = pixW > pixH ? pixW / pixH : pixH / pixW;
                                }
                            }
                            ImGui::SetItemTooltip("Lock the cropping aspect ratio to a specific value");
                            if (activeImage()->imgParam.lockAspect) {
                                ImGui::Spacing();

                                // Aspect ratio — centered label + drag (fills width)
                                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Aspect Ratio").x) * 0.5f);
                                ImGui::Text("Aspect Ratio");
                                ImGui::SetNextItemWidth(sliderW);
                                if (ImGui::DragFloat("###asp", &activeImage()->imgParam.imageCropAspect, 0.01f, 0.1f, 10.0f)) {
                                    updateCrop = true;
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Reset##Asp")) {activeImage()->imgParam.imageCropAspect = activeImage()->width > activeImage()->height ?
                                    (float)activeImage()->width / activeImage()->height :
                                    (float)activeImage()->height / activeImage()->width; paramChange = true;}

                                // Preset buttons — driven by appPrefs.prefs.cropPresets.
                                // Uniform width: widest label across all 6 presets + extra padding.
                                // Laid out as two rows of 3, centered.
                                ImGui::Spacing();
                                const float aspExtraPad = 28.0f;
                                float aspBtnW = 0.0f;
                                for (const auto& p : appPrefs.prefs.cropPresets)
                                    aspBtnW = std::max(aspBtnW, ImGui::CalcTextSize(p.name.c_str()).x);
                                aspBtnW += fp + aspExtraPad;
                                const float aspRowW = aspBtnW * 3.0f + sp * 2.0f;

                                for (int pi = 0; pi < 6; pi++) {
                                    if (pi % 3 == 0)
                                        ImGui::SetCursorPosX((innerW - aspRowW) * 0.5f);
                                    else
                                        ImGui::SameLine();
                                    const auto& p = appPrefs.prefs.cropPresets[pi];
                                    ImGui::PushID(pi);
                                    if (ImGui::Button(p.name.c_str(), ImVec2(aspBtnW, 0))) {
                                        activeImage()->imgParam.imageCropAspect = p.value;
                                        updateCrop = true;
                                    }
                                    ImGui::PopID();
                                }
                            }
                        } else {
                            // Default — "Image Crop" button centered
                            const float imgCropW = ImGui::CalcTextSize("Image Crop").x + fp;
                            ImGui::SetCursorPosX((innerW - imgCropW) * 0.5f);
                            if (ImGui::Button("Image Crop", ImVec2(imgCropW, 0))) {
                                paramChange |= true; cropVisible = true; activeImage()->imgParam.cropEnable = false;
                            }
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::Indent(childDedent);
                }

                ImGui::Spacing();

                ImGui::TreePop();
            }
        }

        if (validIm()) {
            ImGui::Spacing();
            ImGui::Spacing();
        }


            /* INVERSION PARAMS */
            if (validIm()) {
                if (!popupCheck()) {
                    if (appPrefs.prefs.cmykSliders)
                        activeImage()->imgParam.cmyk_to_rgb();
                    else
                        activeImage()->imgParam.rgb_to_cmyk();
                }


                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (TreeNodeWithLine("Analysis")) {

                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
                    ImGui::Unindent(childDedent);
                    float childWidth = ImGui::GetContentRegionAvail().x - sideMargin;
                    ImGui::BeginChild("##child_geo", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                    {
                        const float availW   = ImGui::GetContentRegionAvail().x;
                        const float sp     = ImGui::GetStyle().ItemSpacing.x;
                        const float fp     = ImGui::GetStyle().FramePadding.x * 2.0f;
                        const float btn1W  = ImGui::CalcTextSize("Analyze").x     + fp;
                        const float btn2W  = ImGui::CalcTextSize("Reset Analysis").x + (fp/2.0f);
                        const float chk1W  = ImGui::CalcTextSize("Show Analysis Region").x     + fp + ImGui::GetFrameHeight();
                        const float chk2W  = ImGui::CalcTextSize("Show Points").x + fp + ImGui::GetFrameHeight();
                        const float totalW = btn1W + sp + btn2W + (availW * 0.05f);
                        const float totalCh = chk1W + sp + chk2W + (availW * 0.05f);
                        const float resetW   = ImGui::CalcTextSize("Reset").x
                                             + ImGui::GetStyle().FramePadding.x * 2.0f;
                        const float sliderW  = -(resetW + ImGui::GetStyle().ItemSpacing.x);
                        // Pixel width the slider actually occupies (for label centering).
                        const float sliderPx = availW - resetW - ImGui::GetStyle().ItemSpacing.x;


                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Base Color").x) * 0.5f);
                        ImGui::Text("Base Color");
                        ImGui::SetNextItemWidth(sliderW);
                        paramChange |= ImGui::ColorEdit3("##BC", (float*)activeImage()->imgParam.baseColor, ImGuiColorEditFlags_Float | colorPickerFormat);
                        ImGui::SetItemTooltip("Hold " FV_MOD_C " + shift, click and drag an area in the image\nto sample the film base color.");
                        ImGui::SameLine();
                        if(ImGui::Button("Reset##a1")){activeImage()->imgParam.rstBC(); paramChange |= true;}
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Analysis Bias").x) * 0.5f);
                        ImGui::Text("Analysis Bias");
                        ImGui::SetNextItemWidth(sliderW);
                        paramChange |= ImGui::SliderFloat("##AB", &activeImage()->imgParam.blurAmount, 0.5f, 20.0f);
                        ImGui::SetItemTooltip("The amount of blur to apply to the image before analyzing.\nThis smooths out extreme pixel values for \na more ideal analysis.");
                        ImGui::SameLine();
                        if(ImGui::Button("Reset##a2")){activeImage()->imgParam.rstBLR();}
                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Analyzed Black Point").x) * 0.5f);
                        ImGui::Text("Analyzed Black Point");
                        if (!skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##BP", (float*)activeImage()->imgParam.cmykParam.cmy_A_blackPoint, true, true, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##BP", (float*)activeImage()->imgParam.blackPoint, false, true, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);

                        } else {
                            ImGui::SetNextItemWidth(sliderW);
                            paramChange |= ColorEdit1WithFineTune("##BPS", (float*)activeImage()->imgParam.blackPoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }

                        ImGui::SetItemTooltip("Adjust the analyzed black point.\nThe Alpha value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##01")){activeImage()->imgParam.rstBP(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Analyzed White Point").x) * 0.5f);
                        ImGui::Text("Analyzed White Point");
                        if (!skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##WP", (float*)activeImage()->imgParam.cmykParam.cmy_A_whitePoint, true, true, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##WP", (float*)activeImage()->imgParam.whitePoint, false, true, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        } else {
                            ImGui::SetNextItemWidth(sliderW);
                            paramChange |= ColorEdit1WithFineTune("##WPS", (float*)activeImage()->imgParam.whitePoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the analyzed white point.\nThe Alpha value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##02")){activeImage()->imgParam.rstWP(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}
                        ImGui::Spacing();
                        //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - (totalCh + resetW)) * 0.5f);
                        ImGui::Checkbox("Show Analysis Region", &cropDisplay);
                        ImGui::SetItemTooltip("Display the region used to analyze.");
                        ImGui::SameLine();
                        ImGui::Dummy(ImVec2(availW * 0.05f, 0));
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Points", &minMaxDisp);
                        ImGui::SetItemTooltip("Display the detected min/max luma points in the image\nused to sample the analyzed black point and white point.");
                        ImGui::Spacing();
                        //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - (totalW + resetW)) * 0.5f);
                        if (ImGui::Button("Analyze")) {
                            analyzeImage();
                            cropDisplay = false;
                            minMaxDisp = true;
                            activeRoll()->rollUpState();
                        }
                        ImGui::SameLine();
                        ImGui::Dummy(ImVec2(availW * 0.05f, 0));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(availW - btn2W);
                        if(ImGui::Button("Reset Analysis##a1")) {
                            activeImage()->imgParam.rstANA();
                            activeImage()->renderBypass = true;
                            paramChange |= true;
                            if (appPrefs.prefs.cmykSliders)
                                activeImage()->imgParam.rgb_to_cmyk();
                        }
                        ImGui::Spacing();
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::Indent(childDedent);



                    ImGui::TreePop();
                    ImGui::Spacing();
                }
            }

            ImGui::Spacing();

            /* GRADE PARAMS */
            if (validIm()) {
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                bool wbOn = activeImage()->secEnable & grade_wb;
                if (TreeNodeWithLine("White Balance", &wbOn, altHeld, sideMargin)) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
                    ImGui::Unindent(childDedent);
                    float childW = ImGui::GetContentRegionAvail().x - sideMargin;
                    ImGui::BeginChild("##child_wb", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                    {
                        // innerW: usable content width inside WindowPadding.
                        // resetW: width of a "Reset" button (all reset buttons share the same label).
                        // Slider uses SetNextItemWidth with a negative value so it fills from the
                        // current cursor to (right edge - resetW - ItemSpacing), keeping the pair
                        // flush with both sides and naturally centered.
                        const float innerW   = ImGui::GetContentRegionAvail().x;
                        const float resetW   = ImGui::CalcTextSize("Reset").x
                                             + ImGui::GetStyle().FramePadding.x * 2.0f;
                        const float sliderW  = -(resetW + ImGui::GetStyle().ItemSpacing.x);
                        // Pixel width the slider actually occupies (for label centering).
                        const float sliderPx = innerW - resetW - ImGui::GetStyle().ItemSpacing.x;

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Temperature").x) * 0.5f);
                        ImGui::Text("Temperature");
                        setTempColor(activeImage()->imgParam.temp);
                        ImGui::SetNextItemWidth(sliderW);
                        paramChange |= ImGui::SliderFloat("##TMP", &activeImage()->imgParam.temp, -1.0f, 1.0f);
                        ImGui::PopStyleColor(3);
                        ImGui::SetItemTooltip("Adjust the color temperature of the image.\n" FV_MOD_C " + Click to edit the value manually");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##03")){activeImage()->imgParam.rstTmp(); paramChange = true;}

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Tint").x) * 0.5f);
                        ImGui::Text("Tint");
                        setTintColor(activeImage()->imgParam.tint);
                        ImGui::SetNextItemWidth(sliderW);
                        paramChange |= ImGui::SliderFloat("##TNT", &activeImage()->imgParam.tint, -1.0f, 1.0f);
                        ImGui::PopStyleColor(3);
                        ImGui::SetItemTooltip("Adjust the tint of the image.\n" FV_MOD_C " + Click to edit the value manually");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##04")){activeImage()->imgParam.rstTnt(); paramChange = true;}

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (sliderPx - ImGui::CalcTextSize("Saturation").x) * 0.5f);
                        ImGui::Text("Saturation");
                        ImGui::SetNextItemWidth(sliderW);
                        paramChange |= ImGui::SliderFloat("##SAT", &activeImage()->imgParam.saturation, -1.0f, 1.0f);
                        ImGui::SetItemTooltip("Adjust the saturation of the image.\n" FV_MOD_C" + Click to edit the value manually");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##04s")){activeImage()->imgParam.rstSat(); paramChange = true;}
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::Indent(childDedent);
                    ImGui::Spacing();
                    ImGui::TreePop();
                }
                if (wbOn != ((activeImage()->secEnable & grade_wb) != 0)) {
                    activeImage()->secEnable ^= grade_wb;
                    paramChange |= true;
                }


                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                bool toneOn = activeImage()->secEnable & grade_tone;
                if (TreeNodeWithLine("Tone", &toneOn, altHeld, sideMargin)) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
                    ImGui::Unindent(childDedent);
                    ImGui::BeginChild("##child_tone", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                    {
                        const float innerW    = ImGui::GetContentRegionAvail().x;
                        const float resetW    = ImGui::CalcTextSize("Reset").x
                                              + ImGui::GetStyle().FramePadding.x * 2.0f;
                        const float controlW  = -(resetW + ImGui::GetStyle().ItemSpacing.x);
                        // Pixel span the color-edit control occupies (used to center labels).
                        const float controlPx = innerW - (resetW + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x) - ImGui::GetStyle().ItemSpacing.x;

                        // Lift - Shadows
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("Lift").x) * 0.5f);
                        ImGui::Text("Lift");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##LFT", (float*)activeImage()->imgParam.cmykParam.cmy_lift, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##LFT", (float*)activeImage()->imgParam.g_lift, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##LFT", (float*)activeImage()->imgParam.g_lift, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image lift (shadows).\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##07")){activeImage()->imgParam.rst_gLft(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}

                        // Gamma - Midtones
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("Gamma").x) * 0.5f);
                        ImGui::Text("Gamma");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##GAM", (float*)activeImage()->imgParam.cmykParam.cmy_gamma, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##GAM", (float*)activeImage()->imgParam.g_gamma, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##GAM", (float*)activeImage()->imgParam.g_gamma, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image gamma (midtones).\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##11")){activeImage()->imgParam.rst_g_Gam(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}

                        // Gain - Highlights
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("Gain").x) * 0.5f);
                        ImGui::Text("Gain");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##GN", (float*)activeImage()->imgParam.cmykParam.cmy_gain, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##GN", (float*)activeImage()->imgParam.g_gain, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##GN", (float*)activeImage()->imgParam.g_gain, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image gain (highlights).\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##08")){activeImage()->imgParam.rst_gGain(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}

                        // Black Point
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("Black").x) * 0.5f);
                        ImGui::Text("Black");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##GBP", (float*)activeImage()->imgParam.cmykParam.cmy_blackpoint, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##GBP", (float*)activeImage()->imgParam.g_blackpoint, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##GBP", (float*)activeImage()->imgParam.g_blackpoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image black point.\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##05")){activeImage()->imgParam.rst_gBP(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}

                        // White Point
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("White").x) * 0.5f);
                        ImGui::Text("White");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##GWP", (float*)activeImage()->imgParam.cmykParam.cmy_whitepoint, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##GWP", (float*)activeImage()->imgParam.g_whitepoint, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##GWP", (float*)activeImage()->imgParam.g_whitepoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image white point.\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##06")){activeImage()->imgParam.rst_gWP(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}

                        // Offset
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("Offset").x) * 0.5f);
                        ImGui::Text("Offset");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##OF", (float*)activeImage()->imgParam.cmykParam.cmy_offset, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##OF", (float*)activeImage()->imgParam.g_offset, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##OF", (float*)activeImage()->imgParam.g_offset, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image offset.\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##10")){activeImage()->imgParam.rst_gOft(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}

                        // Multiply
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (controlPx - ImGui::CalcTextSize("Multiply").x) * 0.5f);
                        ImGui::Text("Multiply");
                        if ((altHeld || !appPrefs.prefs.altGrades) && !skinnySlider) {
                            if (appPrefs.prefs.cmykSliders)
                                paramChange |= ColorEdit4WithFineTune("##MLT", (float*)activeImage()->imgParam.cmykParam.cmy_mult, true, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                            else
                                paramChange |= ColorEdit4WithFineTune("##MLT", (float*)activeImage()->imgParam.g_mult, false, false, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        else {
                            ImGui::SetNextItemWidth(controlW);
                            paramChange |= ColorEdit1WithFineTune("##MLT", (float*)activeImage()->imgParam.g_mult, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        }
                        ImGui::SetItemTooltip("Adjust the image multiplication.\nThe 4th value acts as a global multiplier for RGB channels");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##09")){activeImage()->imgParam.rst_gMul(); paramChange = true; if (appPrefs.prefs.cmykSliders) activeImage()->imgParam.rgb_to_cmyk();}
                        }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::Indent(childDedent);
                    ImGui::Spacing();
                    ImGui::TreePop();
                }
                if (toneOn != ((activeImage()->secEnable & grade_tone) != 0)) {
                    activeImage()->secEnable ^= grade_tone;
                    paramChange |= true;
                }

                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                bool curvesOn = activeImage()->secEnable & grade_curves;
                if (TreeNodeWithLine("Curves", &curvesOn, altHeld, sideMargin)) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
                    ImGui::Unindent(childDedent);
                    ImGui::BeginChild("##child_curves", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                    {
                        curvesEditor(paramChange);
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::Indent(childDedent);
                    ImGui::Spacing();
                    ImGui::TreePop();
                }
                if (curvesOn != ((activeImage()->secEnable & grade_curves) != 0)) {
                    activeImage()->secEnable ^= grade_curves;
                    paramChange |= true;
                }

                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                bool matrixOn = activeImage()->secEnable & grade_matrix;
                if (TreeNodeWithLine("Color Matrix", &matrixOn, altHeld, sideMargin)) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
                    ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
                    ImGui::Unindent(childDedent);
                    ImGui::BeginChild("##child_matrix", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                    {
                        paramChange |= ColorEdit3WithFineTune("##MTXR", (float*)activeImage()->imgParam.g_matrix[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        ImGui::SetItemTooltip("Adjust the color makeup of the red channel");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##12")){activeImage()->imgParam.rst_r_Matrix(); paramChange = true;}
                        paramChange |= ColorEdit3WithFineTune("##MTXG", (float*)activeImage()->imgParam.g_matrix[1], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        ImGui::SetItemTooltip("Adjust the color makeup of the green channel");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##13")){activeImage()->imgParam.rst_g_Matrix(); paramChange = true;}
                        paramChange |= ColorEdit3WithFineTune("##MTXB", (float*)activeImage()->imgParam.g_matrix[2], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | colorPickerFormat);
                        ImGui::SetItemTooltip("Adjust the color makeup of the blue channel");
                        ImGui::SameLine();
                        if (ImGui::Button("Reset##14")){activeImage()->imgParam.rst_b_Matrix(); paramChange = true;}
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);
                    ImGui::Indent(childDedent);
                    ImGui::Spacing();
                    ImGui::TreePop();
                }
                if (matrixOn != ((activeImage()->secEnable & grade_matrix) != 0)) {
                    activeImage()->secEnable ^= grade_matrix;
                    paramChange |= true;
                }


                    /* Pending Future implementaion
                    ImGui::SeparatorText("Clarity");
                    ImGui::Text("Sharpen");
                    paramChange |= ImGui::SliderFloat("##SHP", &activeImage()->imgParam.g_sharpen, -1.0f, 1.0f);
                    ImGui::SetItemTooltip("Adjust the sharpness of the image");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##15")){activeImage()->imgParam.rst_gSharp(); paramChange = true;}

                    ImGui::Text("Sharpen Radius");
                    paramChange |= ImGui::SliderFloat("##SHPR", &activeImage()->imgParam.g_sharpenRadius, 0.5f, 3.0f);
                    ImGui::SetItemTooltip("Adjust the radius of the sharpening filter");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##16")){activeImage()->imgParam.rst_gSharpRad(); paramChange = true;}
                    */
            }

        } ImGui::EndChild();


        //--- STATUS ---//
        ImGui::BeginChild("Status", ImVec2(ctrlSize.x, histSecHeight), ImGuiChildFlags_None);
        {
            /*if (validIm()) {
                ImGui::Separator();
                renderCall |= ImGui::Checkbox("Bypass Grade", &gradeBypass);
                ImGui::SameLine();
                renderCall |= ImGui::Checkbox("Bypass Render", &activeImage()->renderBypass);
                ImGui::SameLine();
                renderCall |= ImGui::Checkbox("Show Clipping", &showClip);
                ImGui::SameLine();
                if (ImGui::Checkbox("Histogram", &appPrefs.prefs.histEnable)) {
                    renderCall |= true;
                    uiChanges = true;
                }
                activeImage()->gradeBypass = gradeBypass;
                activeImage()->showClip = showClip;
                if (activeImage()->channelView != channelView) {
                    activeImage()->channelView = channelView;
                    renderCall |= true;
                }

                if (appPrefs.prefs.histEnable) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20);
                    if (ImGui::SliderFloat("###int", &appPrefs.prefs.histInt, 0.0f, 1.0f)) {
                        renderCall |= true;
                        uiChanges = true;
                    }
                    ImGui::SetItemTooltip("Histogram intensity");
                }

            }
            if (validIm()) {
                ImGui::Text("FPS: %7.2f | Raw Res: %ix%i | Working Res: %ix%i",
                    fps,
                    activeImage()->rawWidth, activeImage()->rawHeight,
                    activeImage()->width, activeImage()->height);
            }*/
            /*ImGui::Checkbox("Proxy", &toggleProxy);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20);
            renderCall |= ImGui::SliderFloat("###pxy", &appPrefs.prefs.proxyRes, 0.05f, 0.8f);
            ImGui::SameLine();*/
            if (validIm())
                ImGui::Separator();
            std::string statusText;
            if (gpu->m_rendering)
                statusText += "Rendering... ";
            if(validIm() && activeRoll()->imagesLoading) {
                if (statusText.empty())
                    statusText += "Current Roll Loading... ";
                else
                    statusText += "| Current Roll Loading...";
            }
            if (totalTasks > 0 && !dispImpRollPop) {
                if (statusText.empty())
                    statusText += "BG Rolls Loading...";
                else
                    statusText += "| BG Rolls Loading...";
            }
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(75,75,75,255));
            ImGui::Text("%s", statusText.c_str());
            ImGui::PopStyleColor();
            if (validIm()) {
                if (appPrefs.prefs.histEnable)
                    ImGui::Image(static_cast<ImTextureID>(gpu->histoTex()), ImVec2(ImGui::GetWindowWidth(), 128));
            }
        } ImGui::EndChild();

    }
    if (validIm()) {
        activeImage()->imgParam.cropVisible = cropVisible;
        if(activeImage()->gradeBypass != gradeBypass && !activeImage()->fullIm) {
            activeImage()->gradeBypass = gradeBypass;
            renderCall |= true;
        }
        if (activeImage()->showClip != showClip && !activeImage()->fullIm) {
            activeImage()->showClip = showClip;
            renderCall |= true;
        }
        if (activeImage()->channelView != channelView && !activeImage()->fullIm) {
            activeImage()->channelView = channelView;
            renderCall |= true;
        }
        if (paramChange && !popupCheck()) {
            if (appPrefs.prefs.cmykSliders)
                activeImage()->imgParam.cmyk_to_rgb();
            else
                activeImage()->imgParam.rgb_to_cmyk();
        }
    }

    renderCall |= paramChange;
    needStateUp |= paramChange;

    if (paramChange) {
        paramUpdate();
    }


    ImGui::End();
    ImGui::PopStyleColor(3);
}
