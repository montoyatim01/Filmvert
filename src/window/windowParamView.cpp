#include "window.h"
#include <imgui.h>

void mainWindow::paramView() {

    bool paramChange = false;

    ImGui::SetNextWindowPos(ImVec2(imageWinSize.x,25));
    ImGui::SetNextWindowSize(ImVec2(winWidth - imageWinSize.x, imageWinSize.y));

    ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    {
        ImGui::SetWindowFontScale(std::clamp(ImGui::GetWindowWidth() / 500.0f, 0.80f, 1.15f));
        /* PRESETS */
        if (ImGui::Button("Rotate Left")) {
            if (validIm()) {
                activeImage()->rotLeft();
                if (activeImage()->texture) {
                    SDL_DestroyTexture((SDL_Texture*)activeImage()->texture);
                    activeImage()->texture = nullptr;
                }
                renderCall = true;
                paramChange = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate Right")) {
            if (validIm()) {
                activeImage()->rotRight();
                if (activeImage()->texture) {
                    SDL_DestroyTexture((SDL_Texture*)activeImage()->texture);
                    activeImage()->texture = nullptr;
                }
                renderCall = true;
                paramChange = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Toggle Selection")) {
            if (validIm()) {
                sampleVisible = !sampleVisible;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            if (validIm()) {
                renderCall = true;
            }
        }

        ImGui::Separator();
        ImGui::Text("OCIO Display:");
        renderCall |= ImGui::Combo("##01", &ocioProc.displayOp, ocioProc.displays.data(), ocioProc.displays.size());
        ImGui::Text("OCIO View:");
        renderCall |= ImGui::Combo("##02", &ocioProc.viewOp, ocioProc.views[ocioProc.displayOp].data(), ocioProc.views[ocioProc.displayOp].size());
        ImGui::Separator();

        /* INVERSION PARAMS */
        if (validIm()) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("Analysis")) {
                ImGui::Text("Base Color:");
                paramChange |= ImGui::ColorEdit3("##BC", (float*)activeImage()->imgParam.baseColor, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::Text("Analysis Blur");
                paramChange |= ImGui::SliderFloat("##AB", &activeImage()->imgParam.blurAmount, 0.5f, 20.0f);
                ImGui::Checkbox("Display Analysis Region", &cropDisplay);
                ImGui::Separator();
                if (ImGui::Button("Analyze")) {
                    analyzeImage();
                    cropDisplay = false;
                    minMaxDisp = true;
                }
                ImGui::SameLine();
                ImGui::Checkbox("Display Min/Max Points", &minMaxDisp);
                ImGui::Text("Black Point");
                paramChange |= ImGui::ColorEdit4("##BP", (float*)activeImage()->imgParam.blackPoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##01")){activeImage()->imgParam.rstBP(); paramChange = true;}
                ImGui::Text("White Point");
                paramChange |= ImGui::ColorEdit4("##WP", (float*)activeImage()->imgParam.whitePoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
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
                ImGui::SameLine();
                if (ImGui::Button("Reset##03")){activeImage()->imgParam.rstTmp(); paramChange = true;}
                ImGui::Text("Tint");
                paramChange |= ImGui::SliderFloat("##TNT", &activeImage()->imgParam.tint, -1.0f, 1.0f);
                ImGui::SameLine();
                if (ImGui::Button("Reset##04")){activeImage()->imgParam.rstTnt(); paramChange = true;}
                ImGui::Text("Black Point");
                paramChange |= ImGui::ColorEdit4("##GBP", (float*)activeImage()->imgParam.g_blackpoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##05")){activeImage()->imgParam.rst_gBP(); paramChange = true;}
                ImGui::Text("White Point");
                paramChange |= ImGui::ColorEdit4("##GWP", (float*)activeImage()->imgParam.g_whitepoint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##06")){activeImage()->imgParam.rst_gWP(); paramChange = true;}
                ImGui::Text("Lift");
                paramChange |= ImGui::ColorEdit4("##LFT", (float*)activeImage()->imgParam.g_lift, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##07")){activeImage()->imgParam.rst_gLft(); paramChange = true;}
                ImGui::Text("Gain");
                paramChange |= ImGui::ColorEdit4("##GN", (float*)activeImage()->imgParam.g_gain, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##08")){activeImage()->imgParam.rst_gGain(); paramChange = true;}
                ImGui::Text("Multiply");
                paramChange |= ImGui::ColorEdit4("##MLT", (float*)activeImage()->imgParam.g_mult, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##09")){activeImage()->imgParam.rst_gMul(); paramChange = true;}
                ImGui::Text("Offset");
                paramChange |= ImGui::ColorEdit4("##OF", (float*)activeImage()->imgParam.g_offset, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##10")){activeImage()->imgParam.rst_gOft(); paramChange = true;}
                ImGui::Text("Gamma");
                paramChange |= ImGui::ColorEdit4("##GAM", (float*)activeImage()->imgParam.g_gamma, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine();
                if (ImGui::Button("Reset##11")){activeImage()->imgParam.rst_g_Gam(); paramChange = true;}
                ImGui::TreePop();
            }
        }

        if (validIm()) {
            ImGui::Separator();
            renderCall |= ImGui::Checkbox("Bypass Grade", &activeImage()->gradeBypass);
            ImGui::SameLine();
            renderCall |= ImGui::Checkbox("Bypass Render", &activeImage()->renderBypass);
        }



        ImGui::Text("FPS: %04f, Time: %04fms", mtlGPU->rdTimer.fps, mtlGPU->rdTimer.renderTime);
        ImVec2 winSize = ImGui::GetWindowSize();
        ImGui::SetCursorPosY(winSize.y - 30);
        std::string statusText;
        if (mtlGPU->rendering)
            statusText += "Rendering... ";
        if(validIm() && activeRolls[selRoll].imagesLoading) {
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
        ImGui::Text("%s", statusText.c_str());

    }

    renderCall |= paramChange;

    if (paramChange) {
        paramUpdate();
    }


    ImGui::End();
}
