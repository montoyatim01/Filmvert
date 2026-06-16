#include "image.h"
#include "imageMeta.h"

#include "ocioProcessor.h"
#include "preferences.h"
#include "structs.h"
#include "window.h"

#include <algorithm>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>


//--- Paste Popup ---//
/*
    Popup for the user to select which values
    to paste to the selected images
*/
void mainWindow::pastePopup() {
    if (pasteTrigger)
        ImGui::OpenPopup("Paste");
    if (ImGui::BeginPopupModal("Paste", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Paste selected options");
        ImGui::Separator();

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
        ImGui::Checkbox("Base Color", &pasteOptions.baseColor);
        ImGui::Checkbox("Analysis", &pasteOptions.analysis);
        ImGui::Separator();
        ImGui::InvisibleButton("##02", btnSize);
        ImGui::Checkbox("Temperature", &pasteOptions.temp);
        ImGui::Checkbox("Tint", &pasteOptions.tint);
        ImGui::Checkbox("Saturation", &pasteOptions.saturation);
        ImGui::Checkbox("Blackpoint", &pasteOptions.bp);
        ImGui::Separator();
        ImGui::InvisibleButton("##03", btnSize);
        ImGui::Checkbox("Camera Make", &pasteOptions.make);
        ImGui::Checkbox("Camera Model", &pasteOptions.model);
        ImGui::Checkbox("Lens", &pasteOptions.lens);
        ImGui::Checkbox("Film Stock", &pasteOptions.stock);
        ImGui::Checkbox("Focal Length", &pasteOptions.focal);
        ImGui::Checkbox("f Number", &pasteOptions.fstop);
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        if(ImGui::Button("All Analysis")) {
            pasteOptions.analysisGlobal();
        }
        ImGui::Checkbox("Analysis Blur", &pasteOptions.analysisBlur);
        ImGui::InvisibleButton("##04", btnSize);
        ImGui::Spacing();

        if (ImGui::Button("All Grade")) {
            pasteOptions.gradeGlobal();
        }
        ImGui::Checkbox("Whitepoint", &pasteOptions.wp);
        ImGui::Checkbox("Lift", &pasteOptions.lift);
        ImGui::Checkbox("Gain", &pasteOptions.gain);
        ImGui::Checkbox("Matrix", &pasteOptions.matrix);
        //ImGui::InvisibleButton("##04a", btnSize);
        ImGui::Spacing();

        if (ImGui::Button("All Metadata")) {
            pasteOptions.metaGlobal();
        }

        ImGui::Checkbox("Exposure", &pasteOptions.exposure);
        ImGui::Checkbox("Date/Time", &pasteOptions.date);
        ImGui::Checkbox("Location", &pasteOptions.location);
        ImGui::Checkbox("GPS", &pasteOptions.gps);
        ImGui::Checkbox("Notes", &pasteOptions.notes);
        ImGui::Checkbox("Rotation", &pasteOptions.rotation);

        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::InvisibleButton("##05", btnSize);
        ImGui::Checkbox("Crop Points", &pasteOptions.cropPoints);
        ImGui::InvisibleButton("##06", btnSize);
        ImGui::Spacing();

        ImGui::InvisibleButton("##07", btnSize);
        ImGui::Checkbox("Multiply", &pasteOptions.mult);
        ImGui::Checkbox("Offset", &pasteOptions.offset);
        ImGui::Checkbox("Gamma", &pasteOptions.gamma);
        ImGui::Checkbox("Curves", &pasteOptions.curves);
        ImGui::Spacing();

        ImGui::InvisibleButton("##08", btnSize);

        ImGui::Checkbox("Development Process", &pasteOptions.dev);
        ImGui::Checkbox("Chemistry Manufacturer", &pasteOptions.chem);
        ImGui::Checkbox("Development Notes", &pasteOptions.devnote);
        ImGui::Checkbox("Scanner", &pasteOptions.scanner);
        ImGui::Checkbox("Scanner Notes", &pasteOptions.scannotes);
        ImGui::Checkbox("Crop", &pasteOptions.imageCrop);
        ImGui::EndGroup();


        ImGui::Separator();

        if(ImGui::Button("Cancel")) {
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        if(ImGui::Button("Paste")) {
            pasteIntoParams();
            if (validRoll())
                activeRoll()->rollUpState();
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}


//--- Global Metadata Popup ---//
/*
    Popup for editing the roll-wide metadata
    Will flag to user when there are images
    with varying metadata values for any
    given field.
*/
void mainWindow::globalMetaPopup() {
    if (globalMetaPopTrig)
        ImGui::OpenPopup("Roll Metadata");
    if (ImGui::BeginPopupModal("Roll Metadata", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

        ImGui::Text("Edit roll-wide metadata");
        ImGui::Separator();

        const char imgWarn[] = "Image-specific values set!";

        ImGui::Text("Roll Path:");
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.55);
        metaEdit.a_rollPath |= ImGui::InputText("##00",
            metaEdit.rollPath,
            IM_ARRAYSIZE(metaEdit.rollPath));
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            auto folder = ShowFolderSelectionDialog(false);
            if (!folder.empty()) {
                std::strcpy(metaEdit.rollPath, folder[0].c_str());
                metaEdit.a_rollPath = true;
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Apply##00", (bool*)&metaEdit.a_rollPath);

        ImGui::Text("Roll Name:");
        metaEdit.a_rollname |= ImGui::InputTextWithHint("##01",
            metaEdit.dif_rollname ? imgWarn : "", metaEdit.rollname,
            IM_ARRAYSIZE(metaEdit.rollname));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##01", (bool*)&metaEdit.a_rollname);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginGroup();
        ImGui::Text("Camera Make:");
        metaEdit.a_camMake |= ImGui::InputTextWithHint("##02",
            metaEdit.dif_camMake ? imgWarn : "", metaEdit.camMake,
            IM_ARRAYSIZE(metaEdit.camMake));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##02", (bool*)&metaEdit.a_camMake);

        ImGui::Text("Camera Model:");
        metaEdit.a_camModel |= ImGui::InputTextWithHint("##03",
            metaEdit.dif_camModel ? imgWarn : "", metaEdit.camModel,
            IM_ARRAYSIZE(metaEdit.camModel));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##03", (bool*)&metaEdit.a_camModel);

        ImGui::Text("Lens:");
        metaEdit.a_lens |= ImGui::InputTextWithHint("##04",
            metaEdit.dif_lens ? imgWarn : "", metaEdit.lens,
            IM_ARRAYSIZE(metaEdit.lens));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##04", (bool*)&metaEdit.a_lens);

        ImGui::Text("Film Stock:");
        metaEdit.a_film |= ImGui::InputTextWithHint("##05",
            metaEdit.dif_film ? imgWarn : "", metaEdit.film,
            IM_ARRAYSIZE(metaEdit.film));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##05", (bool*)&metaEdit.a_film);

        ImGui::Text("Focal Length:");
        metaEdit.a_focal |= ImGui::InputTextWithHint("##06",
            metaEdit.dif_focal ? imgWarn : "", metaEdit.focal,
            IM_ARRAYSIZE(metaEdit.focal));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##06", (bool*)&metaEdit.a_focal);
        ImGui::Text("Date/Time:");
        metaEdit.a_date |= ImGui::InputTextWithHint("##07",
            metaEdit.dif_date ? imgWarn : "", metaEdit.date,
            IM_ARRAYSIZE(metaEdit.date));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##07", (bool*)&metaEdit.a_date);
        ImGui::Text("Location:");
        metaEdit.a_loc |= ImGui::InputTextWithHint("##08",
            metaEdit.dif_loc ? imgWarn : "", metaEdit.loc,
            IM_ARRAYSIZE(metaEdit.loc));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##08", (bool*)&metaEdit.a_loc);
        ImGui::EndGroup();


        ImGui::SameLine();
        ImVec2 bottomPos = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(bottomPos.x + 10);
        ImGui::BeginGroup();
        ImGui::Text("GPS:");
        metaEdit.a_gps |= ImGui::InputTextWithHint("##09",
            metaEdit.dif_gps ? imgWarn : "", metaEdit.gps,
            IM_ARRAYSIZE(metaEdit.gps));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##09", (bool*)&metaEdit.a_gps);

        ImGui::Text("Notes:");
        metaEdit.a_notes |= ImGui::InputTextWithHint("##10",
            metaEdit.dif_notes ? imgWarn : "", metaEdit.notes,
            IM_ARRAYSIZE(metaEdit.notes));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##10", (bool*)&metaEdit.a_notes);

        ImGui::Text("Development Process:");
        metaEdit.a_dev |= ImGui::InputTextWithHint("##11",
            metaEdit.dif_dev ? imgWarn : "", metaEdit.dev,
            IM_ARRAYSIZE(metaEdit.dev));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##11", (bool*)&metaEdit.a_dev);

        ImGui::Text("Chemistry Manufacturer:");
        metaEdit.a_chem |= ImGui::InputTextWithHint("##12",
            metaEdit.dif_chem ? imgWarn : "",  metaEdit.chem,
            IM_ARRAYSIZE(metaEdit.chem));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##12", (bool*)&metaEdit.a_chem);

        ImGui::Text("Development Notes:");
        metaEdit.a_devnotes |= ImGui::InputTextWithHint("##13",
            metaEdit.dif_devnotes ? imgWarn : "",
            metaEdit.devnotes, IM_ARRAYSIZE(metaEdit.devnotes));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##13", (bool*)&metaEdit.a_devnotes);

        ImGui::Text("Scanner:");
        metaEdit.a_scanner |= ImGui::InputTextWithHint("##14",
            metaEdit.dif_scanner ? imgWarn : "",
            metaEdit.scanner, IM_ARRAYSIZE(metaEdit.scanner));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##14", (bool*)&metaEdit.a_scanner);

        ImGui::Text("Scanner Notes:");
        metaEdit.a_scannotes |= ImGui::InputTextWithHint("##15",
            metaEdit.dif_scannotes ? imgWarn : "",
            metaEdit.scannotes, IM_ARRAYSIZE(metaEdit.scannotes));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##15", (bool*)&metaEdit.a_scannotes);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Cancel")) {
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            globalMetaPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            // Save the metadata
            if (validRoll()) {
                activeRoll()->rollMetaPostEdit(&metaEdit);
                activeRoll()->rollUpState();
                std::memset(&metaEdit, 0, sizeof(metaBuff));
            }

            globalMetaPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            globalMetaPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}

//--- Local Metadata Popup ---//
/*
    Popup for editing the individual image
    metadata.
*/
void mainWindow::localMetaPopup() {
    if (localMetaPopTrig)
        ImGui::OpenPopup("Image Metadata");
    if (ImGui::BeginPopupModal("Image Metadata", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Edit %s metadata", activeImage()->srcFilename.c_str());
        ImGui::Separator();

        const char imgWarn[] = "";
        ImGui::BeginGroup();
        ImGui::Text("Frame Number:");
        ImGui::InputInt("###", &metaEdit.frameNum);
        ImGui::SameLine();
        if (ImGui::Button("Ripple")) {
            // Ripple the frame number across
            // the remaining images in the roll
            if (validRoll()) {
                if (validIm()) {
                    int curFrame = metaEdit.frameNum;
                    for (int i = activeRoll()->selIm; i < activeRollSize(); i++) {
                        if (getImage(i)) {
                            getImage(i)->imgMeta.frameNumber = curFrame;
                            curFrame++;
                        }
                    }
                }
                activeRoll()->rollUpState();
            }
        }
        ImGui::SetItemTooltip("Ripple the frame number change across the\nremaining images in this roll.\nThis will immediately change the frame\nnumbers in this image and the following images.");
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 14);
        ImGui::BeginGroup();
        ImGui::Text("Rating:");
        ImGui::InputInt("###rat", &activeImage()->imgMeta.rating);
        activeImage()->imgMeta.rating = activeImage()->imgMeta.rating > 5 ? 5 :
            activeImage()->imgMeta.rating < 0 ? 0 :
            activeImage()->imgMeta.rating;
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginGroup();
        ImGui::Text("Camera Make:");
        metaEdit.a_camMake |= ImGui::InputTextWithHint("##02",
            metaEdit.dif_camMake ? imgWarn : "", metaEdit.camMake,
            IM_ARRAYSIZE(metaEdit.camMake));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##02", (bool*)&metaEdit.a_camMake);

        ImGui::Text("Camera Model:");
        metaEdit.a_camModel |= ImGui::InputTextWithHint("##03",
            metaEdit.dif_camModel ? imgWarn : "", metaEdit.camModel,
            IM_ARRAYSIZE(metaEdit.camModel));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##03", (bool*)&metaEdit.a_camModel);

        ImGui::Text("Lens:");
        metaEdit.a_lens |= ImGui::InputTextWithHint("##04",
            metaEdit.dif_lens ? imgWarn : "", metaEdit.lens,
            IM_ARRAYSIZE(metaEdit.lens));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##04", (bool*)&metaEdit.a_lens);

        ImGui::Text("Film Stock:");
        metaEdit.a_film |= ImGui::InputTextWithHint("##05",
            metaEdit.dif_film ? imgWarn : "", metaEdit.film,
            IM_ARRAYSIZE(metaEdit.film));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##05", (bool*)&metaEdit.a_film);

        ImGui::Text("Focal Length:");
        metaEdit.a_focal |= ImGui::InputTextWithHint("##06",
            metaEdit.dif_focal ? imgWarn : "", metaEdit.focal,
            IM_ARRAYSIZE(metaEdit.focal));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##06", (bool*)&metaEdit.a_focal);

        ImGui::Text("f Number:");
        metaEdit.a_fnum |= ImGui::InputTextWithHint("##07",
            metaEdit.dif_fnum ? imgWarn : "", metaEdit.fnum,
            IM_ARRAYSIZE(metaEdit.fnum));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##07", (bool*)&metaEdit.a_fnum);

        ImGui::Text("Exposure Time:");
        metaEdit.a_exp |= ImGui::InputTextWithHint("##08",
            metaEdit.dif_exp ? imgWarn : "", metaEdit.exp,
            IM_ARRAYSIZE(metaEdit.exp));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##08", (bool*)&metaEdit.a_exp);

        ImGui::Text("Date/Time:");
        metaEdit.a_date |= ImGui::InputTextWithHint("##09",
            metaEdit.dif_date ? imgWarn : "", metaEdit.date,
            IM_ARRAYSIZE(metaEdit.date));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##09", (bool*)&metaEdit.a_date);
        ImGui::EndGroup();

        ImGui::SameLine();
        ImVec2 bottomPos = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(bottomPos.x + 10);
        ImGui::BeginGroup();

        ImGui::Text("Location:");
        metaEdit.a_loc |= ImGui::InputTextWithHint("##10",
            metaEdit.dif_loc ? imgWarn : "", metaEdit.loc,
            IM_ARRAYSIZE(metaEdit.loc));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##1-", (bool*)&metaEdit.a_loc);

        ImGui::Text("GPS:");
        metaEdit.a_gps |= ImGui::InputTextWithHint("##11",
            metaEdit.dif_gps ? imgWarn : "", metaEdit.gps,
            IM_ARRAYSIZE(metaEdit.gps));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##11", (bool*)&metaEdit.a_gps);

        ImGui::Text("Notes:");
        metaEdit.a_notes |= ImGui::InputTextWithHint("##12",
            metaEdit.dif_notes ? imgWarn : "", metaEdit.notes,
            IM_ARRAYSIZE(metaEdit.notes));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##12", (bool*)&metaEdit.a_notes);

        ImGui::Text("Development Process:");
        metaEdit.a_dev |= ImGui::InputTextWithHint("##13",
            metaEdit.dif_dev ? imgWarn : "", metaEdit.dev,
            IM_ARRAYSIZE(metaEdit.dev));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##13", (bool*)&metaEdit.a_dev);

        ImGui::Text("Chemistry Manufacturer:");
        metaEdit.a_chem |= ImGui::InputTextWithHint("##14",
            metaEdit.dif_chem ? imgWarn : "",  metaEdit.chem,
            IM_ARRAYSIZE(metaEdit.chem));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##14", (bool*)&metaEdit.a_chem);

        ImGui::Text("Development Notes:");
        metaEdit.a_devnotes |= ImGui::InputTextWithHint("##15",
            metaEdit.dif_devnotes ? imgWarn : "",
            metaEdit.devnotes, IM_ARRAYSIZE(metaEdit.devnotes));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##15", (bool*)&metaEdit.a_devnotes);

        ImGui::Text("Scanner:");
        metaEdit.a_scanner |= ImGui::InputTextWithHint("##16",
            metaEdit.dif_scanner ? imgWarn : "",
            metaEdit.scanner, IM_ARRAYSIZE(metaEdit.scanner));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##16", (bool*)&metaEdit.a_scanner);

        ImGui::Text("Scan Notes:");
        metaEdit.a_scannotes |= ImGui::InputTextWithHint("##17",
            metaEdit.dif_scannotes ? imgWarn : "",
            metaEdit.scannotes, IM_ARRAYSIZE(metaEdit.scannotes));
        ImGui::SameLine();
        ImGui::Checkbox("Apply##17", (bool*)&metaEdit.a_scannotes);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Cancel")) {
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            localMetaPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            imageMetaPostEdit();
            activeRoll()->rollUpState();
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            localMetaPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            std::memset(&metaEdit, 0, sizeof(metaBuff));
            localMetaPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}
