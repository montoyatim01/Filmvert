#include "preferences.h"
#include "structs.h"
#include "window.h"
#include <cstring>
#include <imgui.h>


//--- Preferences Popup ---//
/*
    Popup for editing user preferences
*/
void mainWindow::preferencesPopup() {
    bool prefPush = false;
    if (preferencesPopTrig) {
        // We want to disable the bg dimming so the interface is clearly
        // visible when changing the background colors
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        prefPush = true;
        ImGui::OpenPopup("Preferences");
    }

    if (ImGui::BeginPopupModal("Preferences", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Dummy(ImVec2(480, 2));

        if (ImGui::BeginTabBar("PreferencesTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("General")) {

                ImGui::Spacing();
                // Undo Levels
                ImGui::Text("Undo Levels");
                ImGui::InputInt("###00a", &tmpPrefs.undoLevels);
                tmpPrefs.undoLevels = tmpPrefs.undoLevels < 10 ? 10 :
                    tmpPrefs.undoLevels > 10000 ? 10000 : tmpPrefs.undoLevels;

                // Auto-save
                ImGui::Text("Auto-save");
                ImGui::Checkbox("###01", &tmpPrefs.autoSave);
                if (tmpPrefs.autoSave) {
                    ImGui::SameLine();
                    ImGui::InputInt("Frequency (seconds)", &tmpPrefs.autoSFreq);
                    tmpPrefs.autoSFreq = tmpPrefs.autoSFreq < 1 ? 1 :
                        tmpPrefs.autoSFreq > 10000 ? 10000 : tmpPrefs.autoSFreq;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Trackpad Mode");
                ImGui::Checkbox("###CB", &tmpPrefs.trackpadMode);
                ImGui::SetItemTooltip("When enabled, two finger scroll will navigate the image viewer.\nUse Shift or Alt/Option to zoom in/out.");
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Check for updates");
                ImGui::Checkbox("###UP", &tmpPrefs.checkUpdates);
                ImGui::SetItemTooltip("Periodically check for new updates to the program");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                //ImGui::Checkbox("Demo Mode", &demoWin);

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Display")) {

                // Background Color
                // We're using the actual struct values here such that the
                // interface actively updates while changing
                int colorPickerFormat = tmpPrefs.colorPicker == 0 ? ImGuiColorEditFlags_PickerHueWheel : ImGuiColorEditFlags_PickerHueBar;
                ImGui::Spacing();
                ImGui::SeparatorText("Interface Colors");
                ImGui::Spacing();
                ImGui::Text("Image Viewer Background");
                ColorEdit4WithFineTune("###bgcol", appPrefs.prefs.imageBGColor.data(), false, false, colorPickerFormat);
                //ImGui::ColorEdit4("###bgcol", appPrefs.prefs.imageBGColor.data(), ImGuiColorEditFlags_Float | colorPickerFormat);
                ImGui::SameLine();
                if (ImGui::Button("Reset##01")) {
                    appPrefs.prefs.imageBGColor[0] = 0.0588f;
                    appPrefs.prefs.imageBGColor[1] = 0.0588f;
                    appPrefs.prefs.imageBGColor[2] = 0.0588f;
                    appPrefs.prefs.imageBGColor[3] = 0.9411f;
                }

                ImGui::Spacing();
                ImGui::Text("Parameter Background");
                ColorEdit4WithFineTune("###pacol", appPrefs.prefs.paramBGColor.data(), false, false, colorPickerFormat);
                //ImGui::ColorEdit4("###pacol", appPrefs.prefs.paramBGColor.data(), ImGuiColorEditFlags_Float | colorPickerFormat);
                ImGui::SameLine();
                if (ImGui::Button("Reset##02")) {
                    appPrefs.prefs.paramBGColor[0] = 0.0588f;
                    appPrefs.prefs.paramBGColor[1] = 0.0588f;
                    appPrefs.prefs.paramBGColor[2] = 0.0588f;
                    appPrefs.prefs.paramBGColor[3] = 0.9411f;
                }

                ImGui::Spacing();
                ImGui::Text("Thumbnail Background");
                ColorEdit4WithFineTune("###tmcol", appPrefs.prefs.thumbBGColor.data(), false, false, colorPickerFormat);
                //ImGui::ColorEdit4("###tmcol", appPrefs.prefs.thumbBGColor.data(), ImGuiColorEditFlags_Float | colorPickerFormat);
                ImGui::SameLine();
                if (ImGui::Button("Reset##03")) {
                    appPrefs.prefs.thumbBGColor[0] = 0.0588f;
                    appPrefs.prefs.thumbBGColor[1] = 0.0588f;
                    appPrefs.prefs.thumbBGColor[2] = 0.0588f;
                    appPrefs.prefs.thumbBGColor[3] = 0.9411f;
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::SeparatorText("Interface Controls");
                ImGui::Spacing();

                // Alt-Grades
                ImGui::Text("Hide color controls");
                ImGui::Checkbox("###cc", &tmpPrefs.altGrades);
                ImGui::SetItemTooltip("When enabled, hold the alt key to display individual color channels\nin the grade controls.");

                ImGui::Spacing();

                // CMYK Sliders
                ImGui::Text("CMYK Sliders");
                ImGui::Checkbox("###cms", &tmpPrefs.cmykSliders);
                ImGui::SetItemTooltip("Operate sliders in CMYK mode instead of RGB");

                ImGui::Spacing();

                // Show Stats
                ImGui::Text("Show Stats");
                ImGui::Checkbox("###stat", &tmpPrefs.showStats);
                ImGui::SetItemTooltip("Show application statistics in the image viewer");

                ImGui::Spacing();

                // Color Picker
                ImGui::Text("Color Picker Style");
                ImGui::Combo("###colp", &tmpPrefs.colorPicker, colorPickers.data(), colorPickers.size());

                ImGui::Spacing();

                // Sampling mode
                ImGui::Text("Viewer Sampling");
                ImGui::Combo("###samp", &tmpPrefs.viewerSetting, scalingModes.data(), scalingModes.size());
                ImGui::SetItemTooltip("Linear: Interpolate between pixels\nNearest: Show individual pixels");

                ImGui::Spacing();

                // Pixel display value
                ImGui::Text("Pixel Display Scale");
                ImGui::Combo("###scl", &tmpPrefs.pixelScale, viewPixelDepth.data(), viewPixelDepth.size());
                ImGui::SetItemTooltip("Set the display scale when viewing pixel values\nin the image viewer");

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::SeparatorText("Aspect Ratio Presets");
                ImGui::Spacing();
                ImGui::Spacing();

                // Measure the widest label input so both columns stay aligned
                const float availW   = ImGui::GetContentRegionAvail().x;
                const float sp       = ImGui::GetStyle().ItemSpacing.x;
                const float fp       = ImGui::GetStyle().FramePadding.x * 2.0f;
                const float valW     = 80.0f;
                const float rstW     = ImGui::CalcTextSize("Reset").x + fp;
                const float labelW   = availW - valW - rstW - sp * 2.0f;

                // Default presets for the Reset button
                const std::array<aspectPreset, 6> defaults = {{
                    {"6x7",  1.200f}, {"3:2",  1.500f}, {"645",  1.333f},
                    {"16:9", 1.777f}, {"XPan", 2.708f}, {"2.39", 2.390f}
                }};

                for (int i = 0; i < 6; i++) {
                    ImGui::PushID(i);

                    // Name input
                    char buf[32];
                    std::strncpy(buf, tmpPrefs.cropPresets[i].name.c_str(), sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = '\0';
                    ImGui::SetNextItemWidth(labelW);
                    if (ImGui::InputText("##name", buf, sizeof(buf)))
                        tmpPrefs.cropPresets[i].name = buf;

                    ImGui::SameLine();

                    // Value drag
                    ImGui::SetNextItemWidth(valW);
                    float v = tmpPrefs.cropPresets[i].value;
                    if (ImGui::DragFloat("##val", &v, 0.001f, 0.1f, 100.0f, "%.3f"))
                        tmpPrefs.cropPresets[i].value = std::max(0.1f, v);

                    ImGui::SameLine();

                    // Reset to default
                    if (ImGui::Button("Reset")) {
                        tmpPrefs.cropPresets[i] = defaults[i];
                    }

                    ImGui::PopID();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Spacing();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("System")) {

                ImGui::Spacing();
                // Analysis Minimum Offset
                ImGui::Text("Analysis Minimum Offset");
                ImGui::DragFloat("###aly", &tmpPrefs.minOffset, 0.001f, -0.1f, 0.1f, "%.3f", 0);
                ImGui::SetItemTooltip("Offset for the analysis black point to prevent clipping");

                ImGui::Spacing();
                ImGui::SeparatorText("Proxy Mode");
                ImGui::Spacing();
                // Performance Mode
                ImGui::Text("Enable Proxy Mode");
                ImGui::Checkbox("###02", &tmpPrefs.perfMode);
                ImGui::SetItemTooltip("Scale resolution down for optimal interaction speed.\nWill also unload non-active rolls to save memory usage.\nUnloaded rolls are re-loaded when active.\nIt is recommended to keep this enabled.");
                if (tmpPrefs.perfMode) {
                    ImGui::Spacing();
                    ImGui::Text("Max Res");
                    ImGui::DragInt("##mxr", &tmpPrefs.maxRes, 10.0f, 1000, 5000, "%d", 0);
                    ImGui::SetItemTooltip("Set the maximum resolution on the long side for images\non import. Exported images are rendered in full resolution.");
                    ImGui::Text("Roll Timeout");
                    ImGui::InputInt("###pf1", &tmpPrefs.rollTimeout);
                    ImGui::SetItemTooltip("Rolls sent to the background will wait this long before unloading.\nThis prevents excessive disk/CPU usage when jumping between rolls.");
                    tmpPrefs.rollTimeout = tmpPrefs.rollTimeout < 1 ? 1 :
                        tmpPrefs.rollTimeout > 10000 ? 10000 : tmpPrefs.rollTimeout;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Debayer Mode");
                ImGui::InputInt("###db1", &tmpPrefs.debayerMode);
                ImGui::SetItemTooltip("Set the debayer mode on export/when not using Proxy Mode.\n0: Bilinear\n1: VNG\n2: PPG\n3: AHD\n4: DCB\n5: PL_AHD\n6: AFD\n7: VCD\n8: VCD + AHD\n9: LMMSE\n10: AMaZE\n11: DHT\n12: AP_AHD");
                tmpPrefs.debayerMode = tmpPrefs.debayerMode < 0 ? 0 :
                    tmpPrefs.debayerMode > 12 ? 12 : tmpPrefs.debayerMode;

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Max Simultaneous Exports");
                ImGui::InputInt("###SM1", &tmpPrefs.maxSimExports);
                ImGui::SetItemTooltip("Set the maximum number of simultaneous images to\nprocess when importing/exporting.");
                tmpPrefs.maxSimExports = tmpPrefs.maxSimExports < 1 ? 1 :
                    tmpPrefs.maxSimExports > THREAD_LIMIT ? THREAD_LIMIT : tmpPrefs.maxSimExports;

                ImGui::Spacing();
                ImGui::SeparatorText("OpenColorIO");
                ImGui::Spacing();

                // OCIO
                ImGui::Text("OCIO Configs:");
                std::vector<std::string> cfgNames = ocioProc.getConfigList();
                std::vector<const char*> cfgPtrs;
                cfgPtrs.reserve(cfgNames.size());
                for (auto& s : cfgNames) cfgPtrs.push_back(s.c_str());
                ImGui::Combo("###04", &ocioSel, cfgPtrs.data(), cfgPtrs.size());

                ImGui::Text("Custom OCIO Config:");
                ImGui::InputText("###03", ocioPath, IM_ARRAYSIZE(ocioPath));
                ImGui::SameLine();
                if (ImGui::Button("Open")) {
                    auto selection = ShowFileOpenDialog(false);
                    if (!selection.empty()) {
                        if (!ocioProc.initExtConfig(selection[0])) {
                            //We've supplied a bad config
                            badOcioText = true;
                        } else {
                            badOcioText = false;
                            tmpPrefs.ocioPath = selection[0];
                            std::strcpy(ocioPath, tmpPrefs.ocioPath.c_str());
                            ocioProc.setActiveConfig(-1);
                        }
                    }
                }
                if (badOcioText) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
                    ImGui::Text("Invalid Config!");
                    ImGui::PopStyleColor();
                }
                ImGui::Text("Gamut Compression");
                ImGui::Checkbox("###gam", &tmpPrefs.gamutComp);
                ImGui::SetItemTooltip("Compress the gamut to contain out of range colors");

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();

        if (ImGui::Button("Cancel")) {
            std::memset(ocioPath, 0, sizeof(ocioPath));
            // We made copies of the original values into the tmpPrefs struct
            // copy those unchanged values back to the in-use struct
            std::memcpy(appPrefs.prefs.imageBGColor.data(), tmpPrefs.imageBGColor.data(), sizeof(tmpPrefs.imageBGColor));
            std::memcpy(appPrefs.prefs.paramBGColor.data(), tmpPrefs.paramBGColor.data(), sizeof(tmpPrefs.paramBGColor));
            std::memcpy(appPrefs.prefs.thumbBGColor.data(), tmpPrefs.thumbBGColor.data(), sizeof(tmpPrefs.thumbBGColor));
            preferencesPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            if (ocioSel != ocioProc.selectedConfig) {
                ocioProc.setActiveConfig(ocioSel);
            }
            // The tmpPrefs struct contains unchanged values. Copy the
            // accepted/changed values into the tmp so when we copy back to
            // main the values hold with what was selected
            std::memcpy(tmpPrefs.imageBGColor.data(), appPrefs.prefs.imageBGColor.data(), sizeof(tmpPrefs.imageBGColor));
            std::memcpy(tmpPrefs.paramBGColor.data(), appPrefs.prefs.paramBGColor.data(), sizeof(tmpPrefs.paramBGColor));
            std::memcpy(tmpPrefs.thumbBGColor.data(), appPrefs.prefs.thumbBGColor.data(), sizeof(tmpPrefs.thumbBGColor));
            appPrefs.prefs = tmpPrefs;
            appPrefs.prefs.ocioExt = ocioSel;
            std::memset(ocioPath, 0, sizeof(ocioPath));
            appPrefs.saveToFile();
            preferencesPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            std::memset(ocioPath, 0, sizeof(ocioPath));
            // We made copies of the original values into the tmpPrefs struct
            // copy those unchanged values back to the in-use struct
            std::memcpy(appPrefs.prefs.imageBGColor.data(), tmpPrefs.imageBGColor.data(), sizeof(tmpPrefs.imageBGColor));
            std::memcpy(appPrefs.prefs.paramBGColor.data(), tmpPrefs.paramBGColor.data(), sizeof(tmpPrefs.paramBGColor));
            std::memcpy(appPrefs.prefs.thumbBGColor.data(), tmpPrefs.thumbBGColor.data(), sizeof(tmpPrefs.thumbBGColor));
            preferencesPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::EndPopup();
    }
    if (prefPush)
        ImGui::PopStyleColor();
}
