#include "ocioProcessor.h"
#include "window.h"
#include <chrono>
#include <filesystem>


// Open up a folder with sub-folders
// Each subfolder is a "roll"
// Run through each folder and create vector of images
// "Roll" object vector containing images and title, other?
// Only the active roll gets their images processed
//
// On open, see if directories or files
// If any files, ask which roll.
// Otherwise process folders as rolls
//
void mainWindow::openImages() {
    auto selection = ShowFileOpenDialog();/*pfd::open_file("Select a file", "/Users/timothymontoya/Desktop/CLAi_OFX/2/3/4/5",
                                    { "Image Files", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.exr *.dpx",
                                      "All Files", "*" },
                                    pfd::opt::multiselect).result();*/
    // Do something with selection
    //int activePos = activeRollSize();
    if (selection.size() > 0) {
        dispImportPop = true;
        importFiles = selection;
        checkForRaw();
    }


}

bool mainWindow::openJSON() {
    auto selection = ShowFileOpenDialog(false);

    if (selection.size() > 0) {
        if (validRoll()) {
            if(activeRoll()->importRollMetaJSON(selection[0])) {
                rollRender();
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
                if (getImage(i) && getImage(i)->selected) {

                    getImage(i)->exportPreProcess(expSetting.outPath);
                    mtlGPU->addToRender(getImage(i), r_sdt, exportOCIO);
                    LOG_INFO("Exporting Image {}: {}", i, getImage(i)->srcFilename);
                    getImage(i)->writeImg(expSetting, exportOCIO);
                    getImage(i)->exportPostProcess();
                    exportProcCount++;
                }

            }));
        }

        for (auto& f : futures) {
            f.get();  // Wait for job to finish
        }

        exportPopup = false;
        isExporting = false;
    }};
    exportThread.detach();
}


void mainWindow::exportRolls() {
    for (int r=0; r < activeRolls.size(); r++) {
        if (activeRolls[r].selected) {
            for (int i = 0; i < activeRolls[r].images.size(); i++) {
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
                for (int i = 0; i < activeRolls[r].images.size(); i++) {
                    LOG_INFO("Exporting {} Image from {} Roll", i, r);
                    futures.push_back(pool.submit([this, r, i]() {
                        if (getImage(r, i)) {
                            getImage(r, i)->exportPreProcess(expSetting.outPath + activeRolls[r].rollName);
                            if (!std::filesystem::exists(getImage(r, i)->expFullPath)) {
                                std::filesystem::create_directories(getImage(r, i)->expFullPath);
                            }

                            mtlGPU->addToRender(getImage(r, i), r_full, exportOCIO);
                            LOG_INFO("Exporting Roll: {}, Image {}: {}", r, i, getImage(r, i)->srcFilename);
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

        exportPopup = false;
        isExporting = false;
    }};
    exportThread.detach();



}
