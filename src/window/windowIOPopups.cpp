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




//--- Import Raw Settings ---//
/*
    Section of the import popup window
    that appears when a "raw" file has been
    detected, to set the import settings for
    the batch of images
*/
void mainWindow::importRawSettings() {
    //ImGui::Separator();
    ImGui::Text("RAW Image Settings");

    ImGui::BeginGroup();
    ImGui::Text("Width");
    ImGui::InputScalar("###01a", ImGuiDataType_U16, &rawSet.width,  NULL, NULL, "%d", ImGuiInputFlags_None);
    ImGui::Text("Channels");
    ImGui::InputScalar("###03a", ImGuiDataType_U16, &rawSet.channels,  NULL, NULL, "%d", ImGuiInputFlags_None);
    ImGui::Text("Byte Order IBM PC");
    ImGui::Checkbox("###05a", &rawSet.littleE);
    ImGui::SetItemTooltip("Little/Big Endian. IBM PC being Little Endian:\nthe format Pakon raw saves as.");
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Height");
    ImGui::InputScalar("###02a", ImGuiDataType_U16, &rawSet.height,  NULL, NULL, "%d", ImGuiInputFlags_None);
    ImGui::Text("Bit Depth");
    ImGui::InputScalar("###04a", ImGuiDataType_U16, &rawSet.bitDepth,  NULL, NULL, "%d", ImGuiInputFlags_None);
    ImGui::Text("Planar Data");
    ImGui::Checkbox("###06a", &rawSet.planar);
    ImGui::SetItemTooltip("The color channels are Planar (RRGGBB)\nor Interleaved (RGBRGB)");
    ImGui::EndGroup();
    rawSet.bitDepth = rawSet.bitDepth < 13 ? 8 :
        rawSet.bitDepth > 12 && rawSet.bitDepth < 25 ? 16 : 32;


    if (ImGui::Button("Auto-detect")) {
        testFirstRawFile();
    }
    ImGui::SetItemTooltip("Attempt to set the settings based on\nsome preset Pakon values.");

    ImGui::Separator();


}
//--- Import IDT Setting ---//
/*
    Section of the import popup for
    selecting the image's IDT settings
*/
void mainWindow::importIDTSetting() {
    //ImGui::Separator();
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::TreeNode("Image Input Colorspace Setting")) {
        ImGui::Combo("###CSO", &ocioCS_Disp, colorspaceSet.data(), colorspaceSet.size());
        if (ocioCS_Disp == 0) {
            // Colorspace
            ImGui::Text("Colorspace:");
            std::vector<const char*> cspPtrs;
            cspPtrs.reserve(ocioProc.activeConfig()->colorspaces.size());
            for (auto& s : ocioProc.activeConfig()->colorspaces) cspPtrs.push_back(s.c_str());
            ImGui::Combo("###CS", &importOCIO.colorspace, cspPtrs.data(), cspPtrs.size());
            importOCIO.useDisplay = false;
        } else {
            ImGui::Text("Display:");
            std::vector<const char*> dispPtrs;
            dispPtrs.reserve(ocioProc.activeConfig()->displays.size());
            for (auto& s : ocioProc.activeConfig()->displays) dispPtrs.push_back(s.c_str());
            ImGui::Combo("###DSP", &importOCIO.display, dispPtrs.data(), dispPtrs.size());

            ImGui::Text("View:");
            std::vector<const char*> viewPtrs;
            viewPtrs.reserve(ocioProc.activeConfig()->views[dispOCIO.display].size());
            for (auto& s : ocioProc.activeConfig()->views[dispOCIO.display]) viewPtrs.push_back(s.c_str());
            ImGui::Combo("##VW", &importOCIO.view, viewPtrs.data(), viewPtrs.size());
            importOCIO.useDisplay = true;
        }

        ImGui::Checkbox("Overwrite Existing Setting", &importOCIO.impOverwrite);
        ImGui::SetItemTooltip("Overrides any input colorspace previously set on an image.");
        ImGui::TreePop();
    }
    importOCIO.inverse = true;


    ImGui::Separator();

}

//--- Import Image Popup ---//
/*
    The popup for individual/multiple image importing.
    Contains a dropdown to select which roll the images should
    go into. Allows a second popup that enables the user to
    create a new roll to add the images into.
*/
void mainWindow::importImagePopup() {
    std::vector<const char*> rollPointers;
    for (const auto& item : activeRolls) {
        rollPointers.push_back(item.rollName.c_str());
    }

    if (dispImportPop)
        ImGui::OpenPopup("Import Images");
    if (ImGui::BeginPopupModal("Import Images", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Importing Images...");
        ImGui::Separator();
        if (impRawCheck)
            importRawSettings();
        importIDTSetting();
        ImGui::Text("Roll to insert images to:");
        ImGui::Combo("###", &impRoll, rollPointers.data(), activeRolls.size());
        ImGui::SameLine();
        if (ImGui::Button("New Roll")) {
            newRollPopup = true;
        }

        if (newRollPopup)
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
                newRollPopup = false;
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

                newRollPopup = false;
                ImGui::CloseCurrentPopup();
            }
            if (createDisabled) {
                ImGui::EndDisabled();
            }
            ImGui::Spacing();
            ImGui::EndPopup();
        }



        if (totalTasks != 0 && importFiles.size() > 0) {
            // We're importing, show progress
            ImGui::ProgressBar((float)completedTasks / (float)totalTasks);
            ImGui::Text("Importing Images...");
        }

        // Check if all images are files
        // Prompt to import images to which roll
        // Work out ui for that
        // Also count images?
        if (ImGui::Button("Cancel")) {
            impRoll = 0;
            dispImportPop = false;
            impRawCheck = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        bool impDisabled = false;
        if (activeRolls.size() < 1 || totalTasks > 0) {
            ImGui::BeginDisabled();
            impDisabled = true;
        }

        if (ImGui::Button("Import")) {
            // Here's all the juicy bits

            std::thread impThread = std::thread{[this]() {
                size_t baseIndex = activeRolls[impRoll].rollSize();
                completedTasks = 0;
                totalTasks = importFiles.size();
                activeRolls[impRoll].imagesLoading = true;
                std::vector<std::future<IndexedResult>> futures;
                for (size_t i = 0; i < importFiles.size(); ++i) {
                        const std::string& file = importFiles[i];
                        futures.push_back(tPool->submit([file, i, this]() -> IndexedResult {
                            auto result = readImage(file, rawSet, importOCIO);
                            ++completedTasks; // Increment counter when done
                            return IndexedResult{i, std::move(result)};
                        }));
                }
                std::vector<IndexedResult> results;
                results.reserve(futures.size());
                for (auto& f : futures)
                    results.push_back(f.get());

                std::sort(results.begin(), results.end(),
                        [](const IndexedResult& a, const IndexedResult& b) {
                        return a.index < b.index;
                        });

                for (auto& res : results) {
                    if (std::holds_alternative<image>(res.result)) {
                        activeRolls[impRoll].images.emplace_back(std::get<image>(res.result));
                    } else {
                        LOG_ERROR("Error: {}", std::get<std::string>(res.result));
                    }
                }
                for (int i = baseIndex; i < activeRolls[impRoll].rollSize(); i++) {
                    image *img = getImage(impRoll, i);

                    if (img) {
                        imgRender(img, r_bg);
                        img->imgState.setPtrs(&img->imgMeta, &img->imgParam, &img->needRndr);
                        img->imgMeta.rollName = activeRolls[impRoll].rollName;
                        img->imgMeta.frameNumber = i + 1;
                        img->rollPath = activeRolls[impRoll].rollPath;
                    }


                }
                activeRolls[impRoll].rollLoaded = true;
                activeRolls[impRoll].imagesLoading = false;
                selRoll = impRoll;
                dispImportPop = false;
                totalTasks = 0;
                completedTasks = 0;
                impRoll = 0;
                importFiles.clear();
                rawSet.pakonHeader = false;
                impRawCheck = false;

            }};
            impThread.detach();
        }
        ImGui::Spacing();
        if (impDisabled)
            ImGui::EndDisabled();
        if (!dispImportPop)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

//--- Import Roll Popup ---//
/*
    Import popup for importing rolls. Shows
    the IDT selection for the images and
    displays the progress as the images are
    imported.
*/
void mainWindow::importRollPopup() {

    if (dispImpRollPop)
        ImGui::OpenPopup("Import Rolls");
    if (ImGui::BeginPopupModal("Import Rolls", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Importing Rolls...");
        ImGui::Separator();
        if (impRawCheck)
            importRawSettings();
        importIDTSetting();
        ImGui::Text("Rolls will be created based on the folders selected.");
        ImGui::Text("After the first roll, the remaining will load in the background.");
        ImGui::Separator();

        if (totalTasks != 0 && importFiles.size() > 0) {
            // We're importing, show progress
            ImGui::ProgressBar((float)completedTasks / (float)totalTasks);
            ImGui::Text("Importing %i Images...", (int)totalTasks);
        }

        if (ImGui::Button("Cancel")) {
            impRoll = 0;
            dispImpRollPop = false;
            impRawCheck = false;
            ImGui::CloseCurrentPopup();
        }
        bool impDisable = false;
        if(totalTasks > 0) {
            // We are importing, disallow further imports
            ImGui::BeginDisabled();
            impDisable = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Import")) {
            if (importFiles.size() < 1) {
                //Nothing to import?
                dispImpRollPop = false;
                ImGui::CloseCurrentPopup();
            }
            std::thread impThread = std::thread{[this]() {
                completedTasks = 0;
                for (int r = 0; r < importFiles.size(); r++) {
                    std::vector<std::string> images;
                    const std::filesystem::path sandbox{importFiles[r]};
                    for (auto const& dir_entry : std::filesystem::directory_iterator{sandbox}) {
                        if (dir_entry.is_regular_file()) {
                            //Found an image in root of selection
                            if (dir_entry.path().extension().string() != ".xmp" &&
                                dir_entry.path().extension().string() != ".fvi" &&
                                dir_entry.path().extension().string() != ".json" &&
                                !is_hidden(dir_entry.path()) &&
                                dir_entry.path().stem().string() != std::filesystem::path(importFiles[r]).stem().string()) {
                                    // Ignore files we make that are definitely not images
                                    images.push_back(dir_entry.path().string());

                                }

                        }
                    }
                    // Sort the images
                    std::sort(images.begin(), images.end());
                    totalTasks = images.size();
                    int thisRoll = activeRolls.size();
                    // Add the roll to the library
                    std::string newRollName = std::filesystem::path(importFiles[r]).stem().string();
                    activeRolls.emplace_back(filmRoll(newRollName));
                    //impRoll = activeRolls.size() - 1;
                    activeRolls[thisRoll].imagesLoading = true;
                    // Launch the thread pool
                    std::vector<std::future<IndexedResult>> futures;
                    for (size_t i = 0; i < images.size(); ++i) {
                            const std::string& file = images[i];
                            futures.push_back(tPool->submit([file, i, r, this]() -> IndexedResult {
                                auto result = readImage(file, rawSet, importOCIO, r == 0 ? false : true);
                                ++completedTasks; // Increment counter when done
                                return IndexedResult{i, std::move(result)};
                            }));
                    }

                    std::vector<IndexedResult> results;
                    results.reserve(futures.size());
                    for (auto& f : futures)
                        results.push_back(f.get());

                    std::sort(results.begin(), results.end(),
                            [](const IndexedResult& a, const IndexedResult& b) {
                            return a.index < b.index;
                            });

                    for (auto& res : results) {
                        if (std::holds_alternative<image>(res.result)) {
                            activeRolls[thisRoll].images.emplace_back(std::get<image>(res.result));
                        } else {
                            LOG_ERROR("Error: {}", std::get<std::string>(res.result));
                        }
                    }
                    activeRolls[thisRoll].sortRoll();
                    int maxInt = 0;
                    for (int i = 0; i < activeRolls[thisRoll].rollSize(); i++) {
                        image* thisIm = getImage(thisRoll, i);
                        if (thisIm) {
                            imgRender(thisIm, r_bg);
                            thisIm->imgMeta.rollName = activeRolls[thisRoll].rollName;
                            thisIm->rollPath = importFiles[r];
                            thisIm->imgState.setPtrs(&thisIm->imgMeta, &thisIm->imgParam, &thisIm->needRndr);
                            maxInt = thisIm->imgMeta.frameNumber != 9999 ? std::max(std::min(thisIm->imgMeta.frameNumber, 9999), maxInt) : maxInt;
                            if (thisIm->imgMeta.frameNumber == 9999)
                                maxInt++;
                            thisIm->imgMeta.frameNumber = thisIm->imgMeta.frameNumber == 9999 ? maxInt : thisIm->imgMeta.frameNumber;

                        }
                    }

                    activeRolls[thisRoll].rollLoaded = false;
                    if (r == 0) {
                        // Only do this for the first roll
                        selRoll = thisRoll;
                        dispImpRollPop = false; //Finish the rest of the processing in BG
                        totalTasks = importFiles.size();
                        completedTasks = 1;
                        activeRolls[thisRoll].rollLoaded = true;
                        if (activeRolls[thisRoll].images.size() > 0)
                            activeRolls[thisRoll].images[0].selected = true;
                    }
                    activeRolls[thisRoll].selIm = activeRolls[thisRoll].rollSize() > 0 ? 0 : -1;
                    activeRolls[thisRoll].imagesLoading = false;
                    activeRolls[thisRoll].rollPath = importFiles[r];

                }
                // After all images have finished
                totalTasks = 0;
                completedTasks = 0;
                impRoll = 0;
                importFiles.clear();
                rawSet.pakonHeader = false;
                impRawCheck = false;


            }};
            impThread.detach();
        }
        if (impDisable)
            ImGui::EndDisabled();
        ImGui::Spacing();
        if (!dispImpRollPop)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }


}

//--- Batch Render Popup ---//
/*
    The popup window for rendering out images.
    Both individual/multiple, and rolls. If
    rolls are selected, a basket with checkboxes
    is displayed for the user to select the rolls
    to export.

    Contains output format settings, as well as the
    ODT setting.
*/
void mainWindow::batchRenderPopup() {
    if (exportPopup)
        ImGui::OpenPopup("Export Image(s)");
    if (ImGui::BeginPopupModal("Export Image(s)", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        const float sideMargin   = 4.0f;
        const float childPadding = 8.0f;
        const float childRound   = 6.0f;
        const ImVec4 childBg     = ImVec4(0.11f, 0.11f ,0.11f, 0.94f);
        const ImVec4 childBorder = ImVec4(0.43f, 0.43f, 0.43f, 0.5f);
        const float childDedent  = ImGui::GetStyle().IndentSpacing * 0.75f;
        ImGui::Dummy(ImVec2(520, 2));
        if (expRolls)
            ImGui::Text("Export selected roll(s)");
        else
            ImGui::Text("Export selected image(s)");
        ImGui::Spacing();
        ImGui::Separator();
        bool disableSet = false;
        if (isExporting) {
            ImGui::BeginDisabled();
            disableSet = true;
        }
        if (expRolls) {
            // Table for selecting rolls
            if (ImGui::BeginChild("##Basket", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 6), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY))
            {
                //ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_NoAutoSelect | ImGuiMultiSelectFlags_NoAutoClear, -1, activeRolls.size());
                //ImGuiSelectionExternalStorage storage_wrapper;
                //storage_wrapper.AdapterSetItemSelected = [](ImGuiSelectionExternalStorage* self, int n, bool selected) { activeRolls[n].selected = selected; };
                //storage_wrapper.ApplyRequests(ms_io);
                for (int n = 0; n < activeRolls.size(); n++)
                {
                    ImGui::SetNextItemSelectionUserData(n);
                    ImGui::Checkbox(activeRolls[n].rollName.c_str(), &activeRolls[n].selected);
                }
                //ms_io = ImGui::EndMultiSelect();
                //storage_wrapper.ApplyRequests(ms_io);
            }
            ImGui::EndChild();
            if (ImGui::Button("Toggle All")) {
                bool op = activeRoll()->selected;
                for (filmRoll &roll : activeRolls)
                    roll.selected = !op;
            }
        }

        ImGui::Spacing();
        // File Format
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (TreeNodeWithLine("File Format")) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
            ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
            ImGui::Unindent(childDedent);
            ImGui::BeginChild("##child_fmt", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
            {
                ImGui::Text("File Format:");
                ImGui::Combo("###FF", &expSetting.format, fileTypes.data(), fileTypes.size());
                ImGui::Spacing();
                if (expSetting.format != 2) {
                    ImGui::Text("Bit-Depth:");
                    ImGui::Combo("###BD", &expSetting.bitDepth, bitDepths.data(), bitDepths.size());
                }
                if (expSetting.format == 2) {
                    ImGui::SliderInt("Quality", &expSetting.quality, 10, 100);
                } else if (expSetting.format == 3 || expSetting.format == 4) {
                    ImGui::SliderInt("Compression (loseless)", &expSetting.compression, 1, 9);
                }
                ImGui::Checkbox("Greyscale Mode", &expSetting.greyscale);
                ImGui::SetItemTooltip("Export as a single-channel greyscale image.\nUsed for black & white images.");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            ImGui::Indent(childDedent);
            ImGui::Spacing();
            ImGui::TreePop();
        }
        ImGui::Spacing();
        // Colorspace
        ImGui::SetNextItemOpen(false, ImGuiCond_Once);
        if (TreeNodeWithLine("Colorspace Settings")) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
            ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
            ImGui::Unindent(childDedent);
            ImGui::BeginChild("##child_csp", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
            {
                ImGui::Combo("###CSO", &expSetting.colorspaceOpt, colorspaceSet.data(), colorspaceSet.size());
                if (expSetting.colorspaceOpt == 0) {
                    // Colorspace
                    ImGui::Text("Colorspace:");
                    std::vector<const char*> cspPtrs;
                    cspPtrs.reserve(ocioProc.activeConfig()->colorspaces.size());
                    for (auto& s : ocioProc.activeConfig()->colorspaces) cspPtrs.push_back(s.c_str());
                    ImGui::Combo("###CS", &importOCIO.colorspace, cspPtrs.data(), cspPtrs.size());
                    exportOCIO.useDisplay = false;
                } else {
                    ImGui::Text("Display:");
                    std::vector<const char*> dispPtrs;
                    dispPtrs.reserve(ocioProc.activeConfig()->displays.size());
                    for (auto& s : ocioProc.activeConfig()->displays) dispPtrs.push_back(s.c_str());
                    ImGui::Combo("###DSP", &importOCIO.display, dispPtrs.data(), dispPtrs.size());

                    ImGui::Text("View:");
                    std::vector<const char*> viewPtrs;
                    viewPtrs.reserve(ocioProc.activeConfig()->views[dispOCIO.display].size());
                    for (auto& s : ocioProc.activeConfig()->views[dispOCIO.display]) viewPtrs.push_back(s.c_str());
                    ImGui::Combo("##VW", &importOCIO.view, viewPtrs.data(), viewPtrs.size());
                    exportOCIO.useDisplay = true;
                }
                exportOCIO.inverse = false;
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            ImGui::Indent(childDedent);
            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::Spacing();
        // Geometry
        ImGui::SetNextItemOpen(false, ImGuiCond_Once);
        if (TreeNodeWithLine("Image Geometry")) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sideMargin);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, childRound);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(childPadding, childPadding));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
            ImGui::PushStyleColor(ImGuiCol_Border, childBorder);
            ImGui::Unindent(childDedent);
            ImGui::BeginChild("##child_geo", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
            {
                ImGui::Checkbox("Resizing", &expSetting.resize);
                ImGui::SetItemTooltip("Resize exported images based on pixel value or percentage scale");
                if (expSetting.resize) {
                    ImGui::BeginChild("###rec", ImVec2(ImGui::GetContentRegionAvail().x - sideMargin, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
                    {
                        if (ImGui::BeginTabBar("###res", ImGuiTabBarFlags_None)) {
                            if (ImGui::BeginTabItem("Fixed Size")) {
                                expSetting.fixedSize = true;
                                std::string side = expSetting.longSide ? "Long Side" : "Short Side";
                                ImGui::Spacing();
                                if (ImGui::Button(side.c_str())) {
                                    expSetting.longSide = !expSetting.longSide;
                                }
                                ImGui::SetItemTooltip("Set which edge the pixel value is based on");
                                ImGui::Text("Pixels");
                                ImGui::DragInt("###pxl", (int*)&expSetting.fixedSizePx, 1.0f, 1, 65535);
                                ImGui::SetItemTooltip("Specify the pixel dimension for the set edge");
                                ImGui::EndTabItem();
                            }
                            if (ImGui::BeginTabItem("Percentage")) {
                                expSetting.fixedSize = false;
                                ImGui::Spacing();
                                ImGui::Text("Scale");
                                ImGui::DragFloat("###scl", &expSetting.scaleSize, 0.1f, 1.0f, 200.0f, "%.2f%");
                                ImGui::SetItemTooltip("Set the image scale percentage");
                                ImGui::SameLine();
                                if (ImGui::Button("Reset")) {expSetting.scaleSize = 100.0f;}
                                ImGui::EndTabItem();
                            }
                            ImGui::EndTabBar();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::Spacing();
                    ImGui::Spacing();
                }

                ImGui::Spacing();
                ImGui::Checkbox("Bake Crop & Rotation", &expSetting.bakeRotation);
                ImGui::SetItemTooltip("Bake in all crop and rotation settings to the image.\nWhat is seen in the viewport is what will be exported");
                ImGui::Spacing();
                ImGui::Checkbox("Add Border", &expSetting.border);
                ImGui::SetItemTooltip("Bake in a border to exported images");

                if (expSetting.border) {
                    float borderSizePercent = expSetting.borderSize * 100.0f;
                    if (ImGui::DragFloat("Border Size", &borderSizePercent, 0.1f, 0.0f, 25.0f, "%.1f%%")) {
                        expSetting.borderSize = borderSizePercent / 100.0f;
                    }
                    int colorPickerFormat = tmpPrefs.colorPicker == 0 ? ImGuiColorEditFlags_PickerHueWheel : ImGuiColorEditFlags_PickerHueBar;
                    ImGui::ColorEdit3("Border Color", (float*)&expSetting.borderColor, colorPickerFormat);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            ImGui::Indent(childDedent);
            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Checkbox("Overwrite Existing File(s)?", &expSetting.overwrite);

        // Output Directory
        static char buf1[256] = "";
        ImGui::InputTextWithHint("###Path", "Save Path", buf1, IM_ARRAYSIZE(buf1));
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            auto directory = ShowFolderSelectionDialog(false);
            if (!directory.empty()) {
                strcpy(buf1, directory[0].c_str());
            }

        }
        if (expRolls)
            ImGui::Text("(Rolls will save in sub-directories)");

        if (disableSet)
            ImGui::EndDisabled();

        ImGui::Spacing();
        if (ImGui::Button("Cancel")) {
            isExporting = false;
            exportPopup = false;
            ImGui::CloseCurrentPopup();
        }

        if (std::string(buf1).empty())  // If save path is empty
            disableSet = true;

        if (disableSet) {
            ImGui::BeginDisabled();
            disableSet = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Save")) {
            isExporting = true;
            exportParam params;
            expSetting.outPath = buf1;
            expSetting.outPath += "/";

            elapsedTime = 0;
            exportImgCount = 0;
            if (expRolls)
                exportRolls();
            else
                exportImages();
        }
        ImGui::Spacing();


        if (disableSet)
            ImGui::EndDisabled();


        ImGui::Spacing();
        if (isExporting) {
            ImGui::Separator();
            float progress = (float)exportProcCount / ((float)(exportImgCount-1) + 0.5f);
            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
            unsigned int avgTime = 0;
            elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - expStart).count();
            if (exportProcCount !=0)
                avgTime = elapsedTime / exportProcCount;
            unsigned int remainingTime = (exportImgCount - (exportProcCount + 1)) * avgTime;
            unsigned int remainingSec = remainingTime / 1000;
            unsigned int hr = remainingSec / 3600;
            remainingSec %= 3600;
            unsigned int min = remainingSec / 60;
            remainingSec %= 60;
            std::string remMsg = "";
            remMsg = fmt::format("{:3} of {:3} Images Processed. {:02}:{:02}:{:02} Remaining",
                exportProcCount, exportImgCount, hr, min, remainingSec);
            ImGui::Text("%s", remMsg.c_str());
            ImGui::Spacing();
            ImGui::Spacing();
        }
        if (!exportPopup && !isExporting)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}
