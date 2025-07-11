#include "image.h"
#include "imageMeta.h"
//#include "metalGPU.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "structs.h"
#include "window.h"
#include "releaseNotes.h"

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
            ImGui::Combo("###CS", &importOCIO.colorspace, ocioProc.activeConfig()->colorspaces.data(), ocioProc.activeConfig()->colorspaces.size());
            importOCIO.useDisplay = false;
        } else {
            ImGui::Text("Display:");
            ImGui::Combo("###DSP", &importOCIO.display, ocioProc.activeConfig()->displays.data(), ocioProc.activeConfig()->displays.size());

            ImGui::Text("View:");
            ImGui::Combo("##VW", &importOCIO.view, ocioProc.activeConfig()->views[importOCIO.display].data(), ocioProc.activeConfig()->views[importOCIO.display].size());
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
                                dir_entry.path().stem().string() != std::filesystem::path(importFiles[r]).stem().string())
                                // Ignore files we make that are definitely not images
                                images.push_back(dir_entry.path().string());
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
        if (expRolls)
            ImGui::Text("Export selected roll(s)");
        else
            ImGui::Text("Export selected image(s)");
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
        }

        ImGui::Text("File Format:");
        ImGui::Combo("###FF", &expSetting.format, fileTypes.data(), fileTypes.size());
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Format Settings");
        if (expSetting.format != 2) {
            ImGui::Text("Bit-Depth:");
            ImGui::Combo("###BD", &expSetting.bitDepth, bitDepths.data(), bitDepths.size());
        }
        if (expSetting.format == 2) {
            ImGui::SliderInt("Quality", &expSetting.quality, 10, 100);
        } else if (expSetting.format == 3 || expSetting.format == 4) {
            ImGui::SliderInt("Compression (loseless)", &expSetting.compression, 1, 9);
        }

        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::TreeNode("Colorspace Settings")) {
            // Colorspace or display
            ImGui::Combo("###CSO", &expSetting.colorspaceOpt, colorspaceSet.data(), colorspaceSet.size());
            if (expSetting.colorspaceOpt == 0) {
                // Colorspace
                ImGui::Text("Colorspace:");
                ImGui::Combo("###CS", &exportOCIO.colorspace, ocioProc.activeConfig()->colorspaces.data(), ocioProc.activeConfig()->colorspaces.size());
                exportOCIO.useDisplay = false;
            } else {
                ImGui::Text("Display:");
                ImGui::Combo("###DSP", &exportOCIO.display, ocioProc.activeConfig()->displays.data(), ocioProc.activeConfig()->displays.size());

                ImGui::Text("View:");
                ImGui::Combo("##VW", &exportOCIO.view, ocioProc.activeConfig()->views[exportOCIO.display].data(), ocioProc.activeConfig()->views[exportOCIO.display].size());
                exportOCIO.useDisplay = true;
            }
            exportOCIO.inverse = false;
            ImGui::TreePop();
        }
        ImGui::Separator();

        ImGui::Checkbox("Bake Crop & Rotation", &expSetting.bakeRotation);
        ImGui::SetItemTooltip("Bake in all crop and rotation settings to the image.\nWhat is seen in the viewport is what will be exported");

        ImGui::Checkbox("Add Border", &expSetting.border);
        if (expSetting.border) {
            float borderSizePercent = expSetting.borderSize * 100.0f;
            if (ImGui::DragFloat("Border Size", &borderSizePercent, 0.1f, 0.0f, 25.0f, "%.1f%%")) {
                expSetting.borderSize = borderSizePercent / 100.0f;
            }
            ImGui::ColorEdit3("Border Color", (float*)&expSetting.borderColor);
        }

        ImGui::Separator();

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

        if (ImGui::Button("Cancel")) {
            isExporting = false;
            exportPopup = false;
            ImGui::CloseCurrentPopup();
        }

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
        }
        if (!exportPopup && !isExporting)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

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
        ImGui::InvisibleButton("##04a", btnSize);
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
        ImGui::InvisibleButton("##07a", btnSize);
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

//--- Preferences Popup ---//
/*
    Popup for editing user preferences
*/
void mainWindow::preferencesPopup() {
    if (preferencesPopTrig)
        ImGui::OpenPopup("Preferences");
    if (ImGui::BeginPopupModal("Preferences", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Application Preferences");

        ImGui::Separator();

        // Undo Levels
        ImGui::Text("Undo Levels");
        ImGui::InputInt("###00a", &tmpPrefs.undoLevels);
        tmpPrefs.undoLevels = tmpPrefs.undoLevels < 10 ? 10 :
            tmpPrefs.undoLevels > 10000 ? 10000 : tmpPrefs.undoLevels;

        ImGui::Separator();

        // Auto-save
        ImGui::Text("Auto-save");
        ImGui::Checkbox("###01", &tmpPrefs.autoSave);
        if (tmpPrefs.autoSave) {
            ImGui::SameLine();
            ImGui::InputInt("Frequency (seconds)", &tmpPrefs.autoSFreq);
            tmpPrefs.autoSFreq = tmpPrefs.autoSFreq < 1 ? 1 :
                tmpPrefs.autoSFreq > 10000 ? 10000 : tmpPrefs.autoSFreq;
        }

        ImGui::Separator();
        ImGui::Text("Trackpad Mode");
        ImGui::Checkbox("###CB", &tmpPrefs.trackpadMode);
        ImGui::SetItemTooltip("When enabled, two finger scroll will navigate the image viewer.\nUse Shift or Alt/Option to zoom in/out.");
        ImGui::Separator();

        // Performance Mode
        ImGui::Text("Roll Performance Mode");
        ImGui::Checkbox("###02", &tmpPrefs.perfMode);
        ImGui::SetItemTooltip("Scale resolution down for optimal interaction speed.\nWill also unload non-active rolls to save memory usage.\nUnloaded rolls are re-loaded when active.\nIt is recommended to keep this enabled.");
        if (tmpPrefs.perfMode) {
            ImGui::SameLine();
            ImGui::DragInt("Max Res", &tmpPrefs.maxRes, 10.0f, 1000, 5000, "%d", 0);
            ImGui::SetItemTooltip("Set the maximum resolution on the long side for images\non import. Exported images are rendered in full resolution.");
            ImGui::Text("Roll Timeout");
            ImGui::InputInt("###pf1", &tmpPrefs.rollTimeout);
            ImGui::SetItemTooltip("Rolls sent to the background will wait this amount of time before unloading.\nThis prevents excessive disk/CPU usage when jumping between rolls.");
            tmpPrefs.rollTimeout = tmpPrefs.rollTimeout < 1 ? 1 :
                tmpPrefs.rollTimeout > 10000 ? 10000 : tmpPrefs.rollTimeout;
        }
        ImGui::Separator();
        ImGui::Text("Debayer Mode");
        ImGui::InputInt("###db1", &tmpPrefs.debayerMode);
        ImGui::SetItemTooltip("Set the debayer mode on export/when not using Performance Mode.\n0: Bilinear\n1: VNG\n2: PPG\n3: AHD\n4: DCB\n5: PL_AHD\n6: AFD\n7: VCD\n8: VCD + AHD\n9: LMMSE\n10: AMaZE\n11: DHT\n12: AP_AHD");
        tmpPrefs.debayerMode = tmpPrefs.debayerMode < 0 ? 0 :
            tmpPrefs.debayerMode > 12 ? 12 : tmpPrefs.debayerMode;
        ImGui::Separator();
        ImGui::Text("Max Simultaneous Exports");
        ImGui::InputInt("###SM1", &tmpPrefs.maxSimExports);
        ImGui::SetItemTooltip("Set the maximum number of simultaneous images to\nprocess when exporting.");
        tmpPrefs.maxSimExports = tmpPrefs.maxSimExports < 1 ? 1 :
            tmpPrefs.maxSimExports > 256 ? 256 : tmpPrefs.maxSimExports;

        // OCIO
        ImGui::Text("OCIO Configs:");
        ImGui::Combo("###04", &ocioSel, ocioProc.getConfigList().data(), ocioProc.getConfigList().size());

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

        if (ImGui::Button("Cancel")) {
            std::memset(ocioPath, 0, sizeof(ocioPath));
            preferencesPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            if (ocioSel != ocioProc.selectedConfig) {
                ocioProc.setActiveConfig(ocioSel);
            }
            appPrefs.prefs = tmpPrefs;
            appPrefs.prefs.ocioExt = ocioSel;
            std::memset(ocioPath, 0, sizeof(ocioPath));
            appPrefs.saveToFile();
            preferencesPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            std::memset(ocioPath, 0, sizeof(ocioPath));
            preferencesPopTrig = false;
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

//--- Shortcuts Popup ---//
/*
    Popup for displaying all available
    hotkeys in the application.
*/
void mainWindow::shortcutsPopup() {
    if (shortPopTrig)
        ImGui::OpenPopup("Keyboard Shortcuts");
    if (ImGui::BeginPopupModal("Keyboard Shortcuts", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        std::string s_cmd = "";
        std::string s_opt = "";
        #ifdef __APPLE__
        s_cmd = "⌘";
        s_opt = "⌥";
        #else
        s_cmd = "Ctrl";
        s_opt = "Alt";
        #endif
        ImGui::BeginGroup();
        ImGui::Text("%s + O ", s_cmd.c_str());
        ImGui::Text("%s + Shift + O ", s_cmd.c_str());
        ImGui::Separator();
        ImGui::Text("%s + S ", s_cmd.c_str());
        ImGui::Text("%s + Shift + S ", s_cmd.c_str());
        ImGui::Text("%s + Shift + S ", s_opt.c_str());
        ImGui::Separator();
        ImGui::Text("%s + W ", s_cmd.c_str());
        ImGui::Text("%s + Shift + W ", s_cmd.c_str());
        ImGui::Separator();
        ImGui::Text("%s + Z ", s_cmd.c_str());
        ImGui::Text("%s + Shift + Z ", s_cmd.c_str());
        ImGui::Separator();
        ImGui::Text("%s + E ", s_cmd.c_str());
        ImGui::Text("%s + G ", s_cmd.c_str());
        ImGui::Text("%s + Num ", s_opt.c_str());
        ImGui::Separator();
        ImGui::Text("H");
        ImGui::Text("%s + Scroll", s_opt.c_str());
        ImGui::Text("Shift + Scroll");
        ImGui::Text("%s + Left Arrow", s_opt.c_str());
        ImGui::Text("%s + Right Arrow", s_opt.c_str());
        ImGui::Text("%s + R", s_cmd.c_str());
        ImGui::Text("%s + B", s_cmd.c_str());
        ImGui::Separator();
        ImGui::Text("%s + [ ", s_cmd.c_str());
        ImGui::Text("%s + ] ", s_cmd.c_str());
        ImGui::Text("%s + H ", s_opt.c_str());
        ImGui::Text("%s + V ", s_opt.c_str());
        ImGui::Separator();
        ImGui::Text("%s + A ", s_cmd.c_str());
        ImGui::Text("%s + C ", s_cmd.c_str());
        ImGui::Text("%s + V ", s_cmd.c_str());
        ImGui::Separator();
        ImGui::Text("%s + , ", s_cmd.c_str());
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Open Image(s)");
        ImGui::Text("Open Roll(s)");
        ImGui::Spacing();
        ImGui::Text("Save Image");
        ImGui::Text("Save Roll");
        ImGui::Text("Save All Rolls");
        ImGui::Spacing();
        ImGui::Text("Close Selected Image(s)");
        ImGui::Text("Close Roll");
        ImGui::Spacing();
        ImGui::Text("Undo");
        ImGui::Text("Redo");
        ImGui::Spacing();
        ImGui::Text("Edit Image Metadata");
        ImGui::Text("Edit Roll Metadata");
        ImGui::Text("Rate Image 0-5");
        ImGui::Spacing();
        ImGui::Text("Reset view to fit image");
        ImGui::Text("Zoom in/out of image");
        ImGui::Text("");
        ImGui::Text("Previous Image");
        ImGui::Text("Next Image");
        ImGui::Text("Refresh Image");
        ImGui::Text("Toggle Base Color Selection");
        ImGui::Spacing();
        ImGui::Text("Rotate Left");
        ImGui::Text("Rotate Right");
        ImGui::Text("Flip Horizontally");
        ImGui::Text("Flip Vertically");
        ImGui::Spacing();
        ImGui::Text("Select All");
        ImGui::Text("Copy");
        ImGui::Text("Paste");
        ImGui::Spacing();
        ImGui::Text("Open Preferences");
        ImGui::EndGroup();

        if (ImGui::Button("Okay")) {
            shortPopTrig = false;

            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            shortPopTrig = false;
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
        ImGui::InvisibleButton("##04a", btnSize);
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
        ImGui::InvisibleButton("##07a", btnSize);
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

        float windowWidth = 720;
        float windowHeight = 320;
        ImGui::Dummy(ImVec2(windowWidth, 2));

        ImGui::BeginChild("###RelN", ImVec2(windowWidth - 8, windowHeight));
        ImGui::PushID("REL");
        ImGui::SetWindowFontScale(0.75);

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
