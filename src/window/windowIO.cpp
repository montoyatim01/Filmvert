#include "window.h"


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
    }


}

void mainWindow::openRolls() {
    auto selection = ShowFolderSelectionDialog();
    // Do something with selection
    //int activePos = activeRollSize();
    if (selection.size() > 0) {
        dispImpRollPop = true;
        importFiles = selection;
    }
}
void mainWindow::saveImage() {
    /*auto destination = pfd::save_file("Select a file", ".",
                                      {"All Files", "*" },
                                      pfd::opt::force_overwrite).result();
    // Do something with destination
    if (!destination.empty()) {
        if (validIm()) {
            // Re-render the image first

                //isRendering = true;
                mtlGPU->addToRender(activeImage(), r_sdt);
                //activeImage()->procDispImg();
                //activeImage()->sdlUpdate = true;

            exportPopup = true;

        }
    }*/
}

void mainWindow::openDirectory() {
    /*auto selection = pfd::select_folder("Select a destination").result();
    if (!selection.empty())
        outPath = selection;*/
}

void mainWindow::loadPreset() {
    /*auto selection = pfd::open_file("Select a file", "/Users/timothymontoya/Desktop/CLAi_OFX/2/3/4/5",
                                    { "tGrain Preset", "*.tgrain",
                                      "Image Files", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.exr *.dpx",
                                      "All Files", "*" },
                                    pfd::opt::none).result();
    if (!selection.empty()) {
        std::cout << "User selected file " << selection[0] << "\n";
        if (validIm()) {

        }
        }*/

}
void mainWindow::savePreset() {
    /*auto destination = pfd::save_file("Select a file", ".",
                                      { "tGrain Preset", "*.tgrain",
                                        "All Files", "*" },
                                      pfd::opt::force_overwrite).result();
    // Do something with destination
    if (!destination.empty()) {
        if (validIm()) {

        }
        }*/

}
