#ifndef _LICENSESERVERWINDOW_H
#define _LICENSESERVERWINDOW_H

//System
#include <chrono>
#include <cstddef>
#include <iostream>
#include <ctime>
#include <mutex>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <map>
#include <atomic>
#include <filesystem>
#include <deque>
#include <string>
#include <thread>
#include <csignal>
#include <cmrc/cmrc.hpp>  //read embedded stuff

//ImGui

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//Logging
#include "logger.h"

#include "gpu.h"
#include "image.h"
#include "imageMeta.h"
//#include "metalGPU.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "roll.h"
#include "structs.h"
#include "threadPool.h"
#include "windowUtils.h"


// Declared functions from macOSFile.mm
std::vector<std::string> ShowFileOpenDialog(bool allowMultiple = true, bool canChooseDirectories = false);
std::vector<std::string> ShowFolderSelectionDialog(bool allowMultiple = true);

#ifdef __APPLE__
void setMacOSWindowModified(GLFWwindow* window, bool modified);
#endif

std::string find_key_by_value(const std::map<std::string, int>& my_map, int value);


// For mult-threadded imports
using ImageResult = std::variant<image, std::string>;
struct IndexedResult {
    size_t index;
    ImageResult result;
};

void imguistyle();

class mainWindow
{
    public:
        mainWindow(){};
        ~mainWindow(){};

        int openWindow();
        //void setGPU(metalGPU* mtl){mtlGPU = mtl;}

    private:
        openglGPU* gpu = nullptr;
        bool renderCall = false;
        float fps = 0;

        // Window Layout
        ImVec2 imageWinSize;
        float thumbHeight = 290;
        int winWidth = 0;
        int winHeight = 0;
        ImVec2 cursorPos;
        float dispScale = 1.0f;
        ImVec2 dispSize;
        ImVec2 scroll;
        bool draggingImageCrop = false;

        unsigned long long logoTex = 0;

        // Program state
        bool done = false;
        bool firstImage = false;
        bool rollChange = false;
        bool histRunning = false;
        bool wantClose = false;
        bool uiChanges = false;
        image* prevIm;
        preferenceSet tmpPrefs;

        int ratingFrameCount = 120;
        bool ratingSet = false;

        std::deque<filmRoll> activeRolls;
        int selRoll = 0;

        ocioSetting dispOCIO;

        std::string licText;

        //int selIm = -1;
        ImGuiSelectionBasicStorage selection;
        unsigned int cpColorspace = 0;

        // UI Toggles
        bool cropDisplay = true;
        bool minMaxDisp = false;
        bool sampleVisible = false;
        bool gradeBypass = false;
        bool fpsFlag = false;
        bool toggleProxy = false;
        bool cropVisible = false;

        // Copy/Paste
        imageParams copyParams;
        imageMetadata copyMeta;
        copyPaste pasteOptions;
        bool pasteTrigger = false;
        metaBuff metaEdit;

        // Popup flags
        bool unsavedPopTrigger = false;
        bool globalMetaPopTrig = false;
        bool localMetaPopTrig = false;
        bool preferencesPopTrig = false;
        bool ackPopTrig = false;
        bool anaPopTrig = false;
        closeMode closeMd;
        bool shortPopTrig = false;
        bool imMatchPopTrig = false;
        bool aboutPopTrig = false;
        bool contactPopTrig = false;
        bool ImMatchRoll = false;
        bool badOcioText = false;
        char ackMsg[512];
        char ackError[512];
        char ocioPath[1024];
        int ocioSel = 0;
        bool demoWin = false;




        // Metadata
        std::chrono::time_point<std::chrono::steady_clock> lastChange;
        std::chrono::time_point<std::chrono::steady_clock> lastUISave;
        bool metaRefresh = false;
        bool paramImp = true;
        bool metaImp = true;

        // Undo/redo Timing
        bool needStateUp = false;

        // Interaction timings
        float interactionTimer = 0.0f;
        const float INTERACTION_TIMEOUT = 0.25f; // seconds to wait after last interaction
        bool isInteracting = false;
        bool interactCall = false;

        // Import
        std::atomic<size_t> completedTasks = 0;
        size_t totalTasks = 0;
        bool dispImportPop = false;
        bool dispImpRollPop = false;
        bool impRawCheck = false;
        std::vector<std::string> importFiles;
        int impRoll = 0;
        bool newRollPopup = false;
        char rollNameBuf[64];
        char rollPath[1024];
        rawSetting rawSet;
        // Import OCIO Struct
        ocioSetting importOCIO;
        int ocioCS_Disp = 1;
        std::vector<std::string> imgMetImp;
        copyPaste metImpOpt;


        // Export
        std::thread exportThread;
        exportParam expSetting;
        bool exportPopup = false;
        bool expRolls = false;
        std::chrono::time_point<std::chrono::steady_clock> expStart;
        std::vector<const char*> fileTypes;
        std::vector<const char*> bitDepths;
        std::vector<const char*> colorspaceSet;
        std::vector<const char*> csBake;
        // Export OCIO Struct
        ocioSetting exportOCIO;
        int ocioEXPCS_Disp = 1;

        bool isExporting = false;
        int exportImgCount = 0;
        int exportProcCount = 0;
        unsigned int elapsedTime = 0;
        int contactSheetWidth = 6;

        // Views
        void menuBar();
        void imageView();
        void windowCrop(ImVec2 &imagePos, bool &dragging, bool &isInteracting, bool &currentlyInteracting);
        void paramView();
        void thumbView();

        // windowUtls.cpp
        bool validRoll();
        bool validIm();
        image* activeImage();
        filmRoll* activeRoll();
        image* getImage(int index);
        image* getImage(int roll, int index);
        int activeRollSize();
        void paramUpdate();
        void clearRoll(filmRoll* roll);
        void removeRoll();
        void checkForRaw();
        void testFirstRawFile();
        bool unsavedChanges();
        void selectForward();
        void selectBackward();

        // windowMeta.cpp
        void checkMeta();
        void saveUI();
        void imageMetaPreEdit();
        void imageMetaPostEdit();

        // windowKeys.cpp
        void checkHotkeys();


        void loadMappings();
        void actionA();
        void CalculateThumbDisplaySize(int imageWidth, int imageHeight, float maxHeight, ImVec2& outDisplaySize, int rotation);
        void calculateVisible();

        // windowIO.cpp
        void setIni();
        bool loadLogoImageData(std::optional<cmrc::file> logoIm, int& width, int& height, int& channels,
                               std::vector<unsigned char>& pixels);
        void loadLogoTexture(std::optional<cmrc::file> logoIm);
        void loadLogoTexture(int width, int height, int channels, const std::vector<unsigned char>& pixels);
        void openImages();
        bool openJSON();
        bool openImageMeta();
        bool setImpImage();
        void openRolls();
        void exportImages();
        void exportRolls();


        void clearSelection();


        // windowRender.cpp
        void imgRender();
        void imgRender(image *img, renderType rType = r_bg);
        void rollRenderCheck();
        void rollRender();
        void stateRender();

        void analyzeImage();

        // windowHistogram.cpp
        //void updateHistogram();
        //void createSDLTexture(image* actImage);
        //void updateSDLTexture(image* actImage);
        //void updateHistPixels(image* img, float* imgPixels, float* histPixels, int width, int height, float intensityMultiplier);

        // windowPopups.cpp
        void importRawSettings();
        void importIDTSetting();
        void importImagePopup();
        void importRollPopup();
        void batchRenderPopup();
        void pastePopup();
        void unsavedRollPopup();
        void globalMetaPopup();
        void localMetaPopup();
        void preferencesPopup();
        void ackPopup();
        void analyzePopup();
        void shortcutsPopup();
        void importImMatchPopup();
        void aboutPopup();
        void contactSheetPopup();

        void copyIntoParams();
        void pasteIntoParams();
};

#endif
