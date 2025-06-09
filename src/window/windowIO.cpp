//#include "metalGPU.h"
#include "ocioProcessor.h"
#include "window.h"
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/filesystem.h>
#include <chrono>
#include <filesystem>

//--- Set ini ---//
/*
    Set the location of the imgui ini file
*/
void mainWindow::setIni() {
    ImGuiIO& io = ImGui::GetIO();
    #if defined(WIN32)
    std::string appData;
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath)))
    {
        appData = szPath;
    }
    else
    {
        LOG_CRITICAL("Cannot query the AppData folder!");
    }
    appData += "\\Filmvert\\";
    std::string prefPath = appData;
    prefPath += std::string("/imgui.ini");
    io.IniFilename = prefPath.c_str();

    #elif defined __APPLE__
    char* homeDir = getenv("HOME");
    std::string homeStr = homeDir;
    homeStr += "/Library/Preferences/Filmvert/";
    homeStr += "imgui.ini";
    io.IniFilename = homeStr.c_str();


    #else
    char* homeDir = getenv("HOME");
    std::string homeStr = homeDir;
    homeStr += "/.local/Filmvert/";
    homeStr += "imgui.ini";
    io.IniFilename = homeStr.c_str();
    #endif
}

//--- Load Logo Texture ---//
/*
    Load the logo image and create an
    OpenGL texture with it for display
    in the application
*/
void mainWindow::loadLogoTexture(std::optional<cmrc::file> logoIm) {
    // Create a buffer from the embedded data
    std::vector<unsigned char> imageData(logoIm->begin(), logoIm->end());

    auto in = OIIO::ImageInput::create("png");
    if (! in  ||  ! in->supports ("ioproxy")) {
        LOG_ERROR("OpenImageIO Logo Read Error");
        return;
    }
    OIIO::Filesystem::IOMemReader memreader(imageData);  // I/O proxy object

    auto input = OIIO::ImageInput::open("logo.png", nullptr, &memreader);
    if (!input)
    {
        LOG_ERROR("Failed to create ImageInput: " + OIIO::geterror());
        return;
    }

    // Get image specification
    const OIIO::ImageSpec& spec = input->spec();
    int width = spec.width;
    int height = spec.height;
    int channels = spec.nchannels;

    // Allocate buffer for pixel data
    std::vector<unsigned char> pixels(width * height * channels);

    // Read the image data
    if (!input->read_image(OIIO::TypeDesc::UINT8, pixels.data()))
    {
        LOG_ERROR("Failed to read image data: " + input->geterror());
        input->close();
        return;
    }

    input->close();
    // Generate OpenGL texture
    glGenTextures(1, (GLuint*)&logoTex);
    glBindTexture(GL_TEXTURE_2D, logoTex);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Determine format based on number of channels
    GLenum format;
    switch (channels)
    {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            LOG_ERROR("Unsupported number of channels: " + std::to_string(channels));
            glDeleteTextures(1, (GLuint*)&logoTex);
            logoTex = 0;
            return;
    }

    // Upload texture data to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels.data());

    // Generate mipmaps if desired
    glGenerateMipmap(GL_TEXTURE_2D);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR("OpenGL error while creating texture: " + std::to_string(error));
        glDeleteTextures(1, (GLuint*)&logoTex);
        logoTex = 0;
        return;
    }

    return;
}

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
        std::sort(selection.begin(), selection.end());
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

        std::vector<std::future<void>> futures;

        for (int i = 0; i < activeRollSize(); i++) {
            futures.push_back(tPool->submit([this, i]() {
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
                            LOG_ERROR("Stuck waiting for GPU render. Cannot export file: {}!", getImage(i)->srcFilename);
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

        std::vector<std::future<void>> futures;

        for (int r = 0; r < activeRolls.size(); r++) {
            if (activeRolls[r].selected) {
                for (int i = 0; i < activeRolls[r].rollSize(); i++) {
                    //LOG_INFO("Exporting {} Image from {} Roll", i, r);
                    futures.push_back(tPool->submit([this, r, i]() {
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
                            auto start = std::chrono::steady_clock::now();
                            while (!getImage(r, i)->renderReady) {
                                auto end = std::chrono::steady_clock::now();
                                auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                                if (dur.count() > 15000) {
                                    // Bailing out after waiting 15 seconds for Metal to finish rendering..
                                    // At avg of 20-30fps it should never take this long
                                    LOG_ERROR("Stuck waiting for GPU render. Cannot export file: {}!", getImage(r, i)->srcFilename);
                                    return;
                                }
                                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            }
                            getImage(r, i)->renderReady = false;
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
