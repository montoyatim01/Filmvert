//#include "metalGPU.h"
#include "ocioProcessor.h"
#include "window.h"
#include <chrono>
#include <filesystem>


//--- Open Images ---//
/*
    Open a dialog to select individual
    images for import
*/
void mainWindow::openImages() {
    auto selection = ShowFileOpenDialog();

    if (selection.size() > 0) {
        dispImportPop = true;
        importFiles = selection;
        checkForRaw();
    }

}

//--- Open JSON ---//
/*
    Open dialog for selecting a roll JSON
    file for importing
*/
bool mainWindow::openJSON() {
    auto selection = ShowFileOpenDialog(false);

    if (selection.size() > 0) {
        if (validRoll()) {
            if(activeRoll()->importRollMetaJSON(selection[0])) {
                //rollRender();
                return true;
            } else {
                std::strcpy(ackError, "Failed to parse metadata file!");
            }
        } else {
            std::strcpy(ackError, "Invalid roll selected!");
        }
    }
    return false;
}

bool mainWindow::openImageMeta() {
    imgMetImp = ShowFileOpenDialog(false);

    if (imgMetImp.size() > 0) {
        return true;
    }
    return false;
}

bool mainWindow::setImpImage() {
    if (imgMetImp.size() > 0) {
        if (validIm()) {
            if (activeImage()->importImageMeta(imgMetImp[0], &metImpOpt)) {
                imgRender();
                return true;
            } else {
                std::strcpy(ackError, "Failed to parse image metadata!");
            }
        } else {
            std::strcpy(ackError, "Invalid image selected!");
        }
    }
    return false;
}

//--- Open Rolls ---//
/*
    Open a dialog for a user to open folder(s)
    as rolls
*/
void mainWindow::openRolls() {
    auto selection = ShowFolderSelectionDialog();
    // Do something with selection
    //int activePos = activeRollSize();
    if (selection.size() > 0) {
        dispImpRollPop = true;
        importFiles = selection;
        checkForRaw();
    }
}

//--- Export Images ---//
/*
    Exporting individual images. Loop through
    all images in current roll, and add the
    selected images to the thread pool to
    pre-process, gpu render, export, and post-process
*/
void mainWindow::exportImages() {
    for (int i=0; i < activeRollSize(); i++) {
        if (getImage(i) && getImage(i)->selected)
            exportImgCount++;
    }
    exportProcCount = 0;
    exportThread = std::thread{[this]() {
        expStart = std::chrono::steady_clock::now();

        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;

        for (int i = 0; i < activeRollSize(); i++) {
            futures.push_back(pool.submit([this, i]() {
                if (!isExporting) { // If user has cancelled
                    exportPopup = false; // Bail out
                    return;
                }
                if (getImage(i) && getImage(i)->selected) {

                    getImage(i)->exportPreProcess(expSetting.outPath);
                    gpu->addToRender(getImage(i), r_full, exportOCIO);
                    auto start = std::chrono::steady_clock::now();
                    while (!getImage(i)->renderReady) {
                        auto end = std::chrono::steady_clock::now();
                        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                        if (dur.count() > 15000) {
                            // Bailing out after waiting 15 seconds for Metal to finish rendering..
                            // At avg of 20-30fps it should never take this long
                            LOG_ERROR("Stuck waiting for Metal GPU render. Cannot export file: {}!", getImage(i)->srcFilename);
                            return;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
                    getImage(i)->renderReady = false;
                    //LOG_INFO("Exporting Image {}: {}", i, getImage(i)->srcFilename);
                    getImage(i)->writeImg(expSetting, exportOCIO);
                    getImage(i)->exportPostProcess();
                    exportProcCount++;
                }

            }));
        }

        for (auto& f : futures) {
            f.get();  // Wait for job to finish
        }
        activeRoll()->checkBuffers();
        exportPopup = false;
        isExporting = false;

    }};
    exportThread.detach();
}

//--- Export Rolls ---//
/*
    Loop through all selected rolls and
    add every image to the thread pool to
    pre-process, gpu render, write, and post-process
*/
void mainWindow::exportRolls() {
    for (int r=0; r < activeRolls.size(); r++) {
        if (activeRolls[r].selected) {
            for (int i = 0; i < activeRolls[r].rollSize(); i++) {
                if (getImage(r, i))
                    exportImgCount++;
            }
        }
    } // Total file count
LOG_INFO("Exporting {} Files", exportImgCount);
    exportProcCount = 0;
    exportThread = std::thread{[this]() {
        expStart = std::chrono::steady_clock::now();

        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;

        for (int r = 0; r < activeRolls.size(); r++) {
            if (activeRolls[r].selected) {
                for (int i = 0; i < activeRolls[r].rollSize(); i++) {
                    //LOG_INFO("Exporting {} Image from {} Roll", i, r);
                    futures.push_back(pool.submit([this, r, i]() {
                        if (!isExporting) { // If user has cancelled
                            exportPopup = false; // Bail out
                            return;
                        }
                        if (getImage(r, i)) {
                            getImage(r, i)->exportPreProcess(expSetting.outPath + activeRolls[r].rollName);
                            if (!std::filesystem::exists(getImage(r, i)->expFullPath)) {
                                std::filesystem::create_directories(getImage(r, i)->expFullPath);
                            }

                            gpu->addToRender(getImage(r, i), r_full, exportOCIO);
                            //LOG_INFO("Exporting Roll: {}, Image {}: {}", r, i, getImage(r, i)->srcFilename);
                            getImage(r, i)->writeImg(expSetting, exportOCIO);
                            getImage(r, i)->exportPostProcess();
                            exportProcCount++;
                        } else {
                            LOG_WARN("Could not get {} img from {} roll", i, r);
                        }
                    }));
                }
            }
        }

        for (auto& f : futures) {
            f.get();  // Wait for job to finish
        }
        activeRoll()->checkBuffers();

        exportPopup = false;
        isExporting = false;
        stateRender();
    }};
    exportThread.detach();

}
