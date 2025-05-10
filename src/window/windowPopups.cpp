#include "window.h"



void mainWindow::importImagePopup() {
    if (dispImportPop)
        ImGui::OpenPopup("Import Images");
    if (ImGui::BeginPopupModal("Import Images", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Importing Images");
        ImGui::Separator();
        ImGui::Text("Rolls will be created (and filled) for each directory selected.");
        ImGui::Text("Images in the base directory (or selected images) will go into the selected roll.");
        ImGui::Separator();
        ImGui::Text("Roll to insert images to:");
        ImGui::Combo("###", &impRoll, rollNames.data(), activeRolls.size());
        ImGui::SameLine();
        if (ImGui::Button("New Roll")) {
            newRollPopup = true;
        }

        if (newRollPopup)
            ImGui::OpenPopup("New Roll");
        if (ImGui::BeginPopupModal("New Roll", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            ImGui::Text("Roll Name: ");
            ImGui::InputTextWithHint("###rName", "Roll Name", rollNameBuf, IM_ARRAYSIZE(rollNameBuf));
            if (ImGui::Button("Cancel")) {
                newRollPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Create Roll")) {
                activeRolls.emplace_back(filmRoll(rollNameBuf));
                rollNames.resize(rollNames.size() + 1);
                rollNames[rollNames.size() - 1] = new char[activeRolls.back().rollName.length() + 1];
                std::strcpy(rollNames[rollNames.size() - 1], activeRolls.back().rollName.c_str());
                std::memset(rollNameBuf, 0, sizeof(rollNameBuf));
                impRoll = activeRolls.size() - 1;
                newRollPopup = false;
                ImGui::CloseCurrentPopup();
            }
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
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (activeRolls.size() < 1)
            ImGui::BeginDisabled();
        if (ImGui::Button("Open")) {
            // Here's all the juicy bits

            std::thread impThread = std::thread{[this]() {
                size_t baseIndex = activeRolls[impRoll].images.size();
                completedTasks = 0;
                totalTasks = importFiles.size();
                activeRolls[impRoll].imagesLoading = true;
                ThreadPool pool(std::thread::hardware_concurrency());
                std::vector<std::future<IndexedResult>> futures;
                for (size_t i = 0; i < importFiles.size(); ++i) {
                        const std::string& file = importFiles[i];
                        futures.push_back(pool.submit([file, i, this]() -> IndexedResult {
                            auto result = readRawImage(file);
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
                for (int i = baseIndex; i < activeRolls[impRoll].images.size(); i++) {
                    imgRender(getImage(impRoll, i));
                }
                activeRolls[impRoll].imagesLoading = false;
                selRoll = impRoll;
                dispImportPop = false;
                totalTasks = 0;
                completedTasks = 0;
                impRoll = 0;
                importFiles.clear();

            }};
            impThread.detach();
        }
        if (activeRolls.size() < 1)
            ImGui::EndDisabled();
        if (!dispImportPop)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void mainWindow::importRollPopup() {

    if (dispImpRollPop)
        ImGui::OpenPopup("Import Rolls");
    if (ImGui::BeginPopupModal("Import Rolls", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Importing Rolls...");
        ImGui::Separator();
        ImGui::Text("Rolls will be created based on the folders selected");
        ImGui::Text("After the first roll, the remaining will load in the background");
        ImGui::Separator();

        if (totalTasks != 0 && importFiles.size() > 0) {
            // We're importing, show progress
            ImGui::ProgressBar((float)completedTasks / (float)totalTasks);
            ImGui::Text("Importing %i Images...", (int)totalTasks);
        }

        if (ImGui::Button("Cancel")) {
            impRoll = 0;
            dispImpRollPop = false;
            ImGui::CloseCurrentPopup();
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
                    rollNames.resize(rollNames.size() + 1);
                    rollNames[rollNames.size() - 1] = new char[activeRolls.back().rollName.length() + 1];
                    std::strcpy(rollNames[rollNames.size() - 1], activeRolls.back().rollName.c_str());
                    //impRoll = activeRolls.size() - 1;
                    activeRolls[thisRoll].imagesLoading = true;
                    // Launch the thread pool
                    ThreadPool pool(std::thread::hardware_concurrency());
                    std::vector<std::future<IndexedResult>> futures;
                    for (size_t i = 0; i < images.size(); ++i) {
                            const std::string& file = images[i];
                            futures.push_back(pool.submit([file, i, this]() -> IndexedResult {
                                auto result = readRawImage(file);
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
                    for (int i = 0; i < activeRolls[thisRoll].images.size(); i++) {
                        image* thisIm = getImage(thisRoll, i);
                        //LOG_INFO("Queueing Image: {} from Roll: {}, with iterator: {}, with pointer: {}", thisIm->srcFilename, thisRoll, i, fmt::ptr(thisIm));
                        imgRender(thisIm);
                    }

                    if (r == 0) {
                        // Only do this for the first roll
                        selRoll = thisRoll;
                        dispImpRollPop = false; //Finish the rest of the processing in BG
                        totalTasks = importFiles.size();
                        completedTasks = 1;
                    }
                    activeRolls[thisRoll].rollLoaded = true;
                    activeRolls[thisRoll].imagesLoading = false;

                }
                // After all images have finished
                totalTasks = 0;
                completedTasks = 0;
                impRoll = 0;
                importFiles.clear();

            }};
            impThread.detach();
        }
        if (!dispImpRollPop)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }


}


void mainWindow::batchRenderPopup() {
    if (exportPopup)
        ImGui::OpenPopup("ExportWindow");
    if (ImGui::BeginPopupModal("ExportWindow", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Export selected images");
        ImGui::Separator();

        ImGui::Text("File Format:");
        ImGui::Combo("###FF", &outType, fileTypes.data(), fileTypes.size());
        ImGui::Text("Bit-Depth:");
        ImGui::Combo("###BD", &outDepth, bitDepths.data(), bitDepths.size());
        ImGui::SliderInt("Quality", &quality, 10, 100);
        ImGui::Checkbox("Overwrite Existing File(s)?", &overwrite);

        // Output Directory
        static char buf1[256] = "";
        ImGui::InputTextWithHint("###Path", "Save Path", buf1, IM_ARRAYSIZE(buf1));
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            openDirectory();
            strcpy(buf1, outPath.c_str());
        }
        bool disableSet = false;
        if (isExporting) {
            ImGui::BeginDisabled();
            disableSet = true;
        }

        /*if(ImGui::Button("Save")) {
            isExporting = true;
            exportParam params;
            params.outPath = buf1;
            params.format = outType;
            params.bitDepth = outDepth;
            params.quality = quality;
            params.overwrite = overwrite;
            elapsedTime = 0;
            numIm = 0;
            for (int i=0; i < activeRollSize(); i++) {
                if (getImage(i)->selected)
                    numIm++;
            }
            curIm = 0;
            exportThread = std::thread{[this, params]() {
                for (int i = 0; i < activeRollSize(); i++) {
                    if (!isExporting) {
                        exportPopup = false;
                        return;
                    }

                    if (getImage(i)->selected) {
                        auto start = std::chrono::steady_clock::now();
                        //TODO: Figure out better solution for this (non-blocking)
                        // but still will render out an image.
                        if (!isRendering) {
                            //isRendering = true;
                            mtlGPU->addToRender(getImage(i), r_sdt);
                            //getImage(i)->procDispImg();
                            //getImage(i)->sdlUpdate = true;
                            isRendering = false;
                        }
                        LOG_INFO("Exporting Image {}: {}", i, getImage(i)->srcFilename);
                        getImage(i)->writeImg(params);
                        auto end = std::chrono::steady_clock::now();
                        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                        elapsedTime += dur.count();
                        curIm++;
                    }

                }
                exportPopup = false;
                isExporting = false;
            }};
            exportThread.detach();
            //exportPopup = false;
            //ImGui::CloseCurrentPopup();
        }*/
        if (disableSet)
            ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            isExporting = false;
            exportPopup = false;
            ImGui::CloseCurrentPopup();
        }
        if (isExporting) {
            ImGui::Separator();
            float progress = (float)curIm / ((float)(numIm-1) + 0.5f);
            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
            unsigned int avgTime = 0;
            if (curIm !=0)
                avgTime = elapsedTime / curIm;
            unsigned int remainingTime = (numIm - (curIm + 1)) * avgTime;
            unsigned int remainingSec = remainingTime / 1000;
            unsigned int hr = remainingSec / 3600;
            remainingSec %= 3600;
            unsigned int min = remainingSec / 60;
            remainingSec %= 60;
            std::string remMsg = "";
            remMsg = fmt::format("{:3} of {:3} Images Processed. {:02}:{:02}:{:02} Remaining",
                curIm, numIm, hr, min, remainingSec);
            ImGui::Text(remMsg.c_str());
        }
        if (!exportPopup && !isExporting)
            ImGui::CloseCurrentPopup();



        ImGui::EndPopup();
    }
}

void mainWindow::pastePopup() {
    if (pasteTrigger)
        ImGui::OpenPopup("PasteWindow");
    if (ImGui::BeginPopupModal("PasteWindow", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)){
        ImGui::Text("Paste selected options");
        ImGui::Separator();
        if(ImGui::Button("All Analysis")) {
            pasteOptions.analysisGlobal();
        }
        ImGui::Checkbox("Base Color", &pasteOptions.baseColor);
        ImGui::SameLine();
        ImGui::Checkbox("Crop Points", &pasteOptions.cropPoints);

        ImGui::Checkbox("Analysis Blur", &pasteOptions.analysisBlur);
        ImGui::SameLine();
        ImGui::Checkbox("Analysis", &pasteOptions.analysis);

        ImGui::Separator();
        if (ImGui::Button("All Grade")) {
            pasteOptions.gradeGlobal();
        }
        ImGui::Checkbox("Temperature", &pasteOptions.temp);
        ImGui::SameLine();
        ImGui::Checkbox("Tint", &pasteOptions.tint);

        ImGui::Checkbox("Blackpoint", &pasteOptions.bp);
        ImGui::SameLine();
        ImGui::Checkbox("Whitepoint", &pasteOptions.wp);

        ImGui::Checkbox("Lift", &pasteOptions.lift);
        ImGui::SameLine();
        ImGui::Checkbox("Gain", &pasteOptions.gain);

        ImGui::Checkbox("Multiply", &pasteOptions.mult);
        ImGui::SameLine();
        ImGui::Checkbox("Offset", &pasteOptions.offset);

        ImGui::Checkbox("Gamma", &pasteOptions.gamma);

        if(ImGui::Button("Cancel")) {
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Paste")) {
            pasteIntoParams();
            pasteTrigger = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
