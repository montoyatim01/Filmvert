#include "preferences.h"
#include "window.h"
#include <imgui.h>

//--- Main Parameter View Routine ---//
void mainWindow::paramView() {

    bool paramChange = false;

    ImGui::SetNextWindowPos(ImVec2(imageWinSize.x,25));
    ImGui::SetNextWindowSize(ImVec2(winWidth - imageWinSize.x, imageWinSize.y));

    ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 ctrlSize = ImGui::GetWindowSize();
        int histHeight = appPrefs.prefs.histEnable ? 128 : 0;
        int histSecHeight = histHeight;
        histSecHeight += (ImGui::CalcTextSize("Text").y * 4);
        histSecHeight += (ImGui::GetStyle().FramePadding.y * 4);
        //histSecHeight += (ImGui::GetStyle().ItemInnerSpacing.y * 4);

        ctrlSize.x = ctrlSize.x - (ImGui::GetStyle().WindowPadding.x * 2);
        ctrlSize.y = ctrlSize.y - histSecHeight;
        ctrlSize.y = ctrlSize.y - (ImGui::GetStyle().FramePadding.y * 2);
        ctrlSize.y = ctrlSize.y - (ImGui::GetStyle().WindowPadding.y * 2);

        ImGui::SetWindowFontScale(std::clamp(ImGui::GetWindowWidth() / 600.0f, 0.75f, 1.15f));
        //---CONTROLS CHILD---//
        ImGui::BeginChild("Controls##02", ctrlSize, ImGuiChildFlags_None);
        {
            /* PRESETS */
            if (ImGui::Button("Rotate Left")) {
                if (validIm()) {
                    activeImage()->rotLeft();
                    renderCall = true;
                    paramChange = true;
                    activeRoll()->rollUpState();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Rotate Right")) {
                if (validIm()) {
                    activeImage()->rotRight();
                    renderCall = true;
                    paramChange = true;
                    activeRoll()->rollUpState();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Toggle Selection")) {
                if (validIm()) {
                    sampleVisible = !sampleVisible;
                }
            }
            ImGui::SetItemTooltip("Toggle the base color selection preview");

            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                if (validIm()) {
                    renderCall = true;
                    activeImage()->imgRst = true;
                }
            }
            ImGui::SetItemTooltip("Re-render the current image");

            ImGui::Separator();

            /* INVERSION PARAMS */
            if (validIm()) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode("Analysis")) {
                    ImGui::Text("Base Color:");
                    paramChange |= ImGui::ColorEdit3("##BC", (float*)activeImage()->imgParam.baseColor, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Hold ⌘ + shift, click and drag an area in the image\nto sample the base color.");
                    ImGui::SameLine();
                    if(ImGui::Button("Reset##a1")){activeImage()->imgParam.rstBC(); paramChange |= true;}
                    ImGui::Text("Analysis Bias");
                    paramChange |= ImGui::SliderFloat("##AB", &activeImage()->imgParam.blurAmount, 0.5f, 20.0f);
                    ImGui::SetItemTooltip("The amount of blur to apply to the image before analyzing.\nThis smooths out extreme pixel values for a \npotentially more accurate analysis.");
                    ImGui::SameLine();
                    if(ImGui::Button("Reset##a2")){activeImage()->imgParam.rstBLR();}
                    ImGui::Checkbox("Display Analysis Region", &cropDisplay);
                    ImGui::SetItemTooltip("Display the region used to analyze.");
                    ImGui::Separator();
                    if (ImGui::Button("Analyze")) {
                        analyzeImage();
                        cropDisplay = false;
                        minMaxDisp = true;
                        activeRoll()->rollUpState();
                    }
                    ImGui::SameLine();
                    ImGui::SameLine();
                    if(ImGui::Button("Reset Analysis##a1")){activeImage()->imgParam.rstANA(); activeImage()->renderBypass = true; paramChange |= true;}
                    ImGui::SameLine();
                    ImGui::Checkbox("Display Min/Max Points", &minMaxDisp);
                    ImGui::SetItemTooltip("Display the detected min/max luma points in the image\nused to sample the analyzed black point and white point.");
                    ImGui::Text("Analyzed Black Point");
                    paramChange |= ColorEdit4WithFineTune("##BP", (float*)activeImage()->imgParam.blackPoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the analyzed black point.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##01")){activeImage()->imgParam.rstBP(); paramChange = true;}
                    ImGui::Text("Analyzed White Point");
                    paramChange |= ColorEdit4WithFineTune("##WP", (float*)activeImage()->imgParam.whitePoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the analyzed white point.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##02")){activeImage()->imgParam.rstWP(); paramChange = true;}
                    ImGui::TreePop();
                }
            }



            /* GRADE PARAMS */
            if (validIm()) {
                ImGui::Separator();
                if (ImGui::TreeNode("Grade")) {
                    ImGui::Text("Temperature");
                    paramChange |= ImGui::SliderFloat("##TMP", &activeImage()->imgParam.temp, -1.0f, 1.0f);
                    ImGui::SetItemTooltip("Adjust the color temperature of the image.\n⌘ + Click to edit the value manually");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##03")){activeImage()->imgParam.rstTmp(); paramChange = true;}
                    ImGui::Text("Tint");
                    paramChange |= ImGui::SliderFloat("##TNT", &activeImage()->imgParam.tint, -1.0f, 1.0f);
                    ImGui::SetItemTooltip("Adjust the tint of the image.\n⌘ + Click to edit the value manually");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##04")){activeImage()->imgParam.rstTnt(); paramChange = true;}
                    ImGui::Text("Black Point");
                    paramChange |= ColorEdit4WithFineTune("##GBP", (float*)activeImage()->imgParam.g_blackpoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image black point.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##05")){activeImage()->imgParam.rst_gBP(); paramChange = true;}
                    ImGui::Text("White Point");
                    paramChange |= ColorEdit4WithFineTune("##GWP", (float*)activeImage()->imgParam.g_whitepoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image white point.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##06")){activeImage()->imgParam.rst_gWP(); paramChange = true;}
                    ImGui::Text("Lift");
                    paramChange |= ColorEdit4WithFineTune("##LFT", (float*)activeImage()->imgParam.g_lift, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image lift.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##07")){activeImage()->imgParam.rst_gLft(); paramChange = true;}
                    ImGui::Text("Gamma");
                    paramChange |= ColorEdit4WithFineTune("##GAM", (float*)activeImage()->imgParam.g_gamma, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image gamma.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##11")){activeImage()->imgParam.rst_g_Gam(); paramChange = true;}
                    ImGui::Text("Gain");
                    paramChange |= ColorEdit4WithFineTune("##GN", (float*)activeImage()->imgParam.g_gain, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image gain.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##08")){activeImage()->imgParam.rst_gGain(); paramChange = true;}
                    ImGui::Text("Offset");
                    paramChange |= ColorEdit4WithFineTune("##OF", (float*)activeImage()->imgParam.g_offset, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image offset.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##10")){activeImage()->imgParam.rst_gOft(); paramChange = true;}
                    ImGui::Text("Multiply");
                    paramChange |= ColorEdit4WithFineTune("##MLT", (float*)activeImage()->imgParam.g_mult, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                    ImGui::SetItemTooltip("Adjust the image multiplication.\nThe Alpha value acts as a global multiplier for RGB channels");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##09")){activeImage()->imgParam.rst_gMul(); paramChange = true;}
                    ImGui::TreePop();
                }
            }

        } ImGui::EndChild();


        //--- STATUS ---//
        ImGui::BeginChild("Status", ImVec2(ctrlSize.x, histSecHeight), ImGuiChildFlags_None);
        {
            if (validIm()) {
                ImGui::Separator();
                renderCall |= ImGui::Checkbox("Bypass Grade", &gradeBypass);
                ImGui::SameLine();
                renderCall |= ImGui::Checkbox("Bypass Render", &activeImage()->renderBypass);
                ImGui::SameLine();
                renderCall |= ImGui::Checkbox("Histogram", &appPrefs.prefs.histEnable);
                activeImage()->gradeBypass = gradeBypass;
                if (appPrefs.prefs.histEnable) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20);
                    renderCall |= ImGui::SliderFloat("###int", &appPrefs.prefs.histInt, 0.0f, 1.0f);
                    ImGui::SetItemTooltip("Histogram intensity");
                }

            }
            if (validIm()) {
                ImGui::Text("FPS: %7.2f | Raw Res: %ix%i | Working Res: %ix%i",
                    fps,
                    activeImage()->rawWidth, activeImage()->rawHeight,
                    activeImage()->width, activeImage()->height);
            }
            ImGui::Checkbox("Proxy", &toggleProxy);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.20);
            renderCall |= ImGui::SliderFloat("###pxy", &appPrefs.prefs.proxyRes, 0.05f, 0.8f);
            ImGui::SameLine();
            std::string statusText;
            if (gpu->rendering)
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

    renderCall |= paramChange;
    needStateUp |= paramChange;

    if (paramChange) {
        paramUpdate();
    }


    ImGui::End();
}
