#ifndef _LICENSESERVERWINDOW_H
#define _LICENSESERVERWINDOW_H

//System
#include <chrono>
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

//ImGui
#include "grainParams.h"
#include "imageMeta.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>

//Logging
#include "logger.h"

#include "imageIO.h"
#include "metalGPU.h"
#include "ocioProcessor.h"
#include "preferences.h"
#include "roll.h"
#include "threadPool.h"
#include "windowUtils.h"

// Declared functions from macOSFile.mm
std::vector<std::string> ShowFileOpenDialog(bool allowMultiple = true, bool canChooseDirectories = false);
std::vector<std::string> ShowFolderSelectionDialog(bool allowMultiple = true);

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
        void setGPU(metalGPU* mtl){mtlGPU = mtl;}

    private:
        SDL_Renderer* renderer;
        metalGPU* mtlGPU;
        bool renderCall = false;

        // Window Layout
        ImVec2 imageWinSize;
        float thumbHeight = 290;
        int winWidth;
        int winHeight;
        ImVec2 cursorPos;
        float dispScale = 1.0f;
        ImVec2 dispSize;
        ImVec2 scroll;

        // Program state
        bool done = false;
        userPreferences preferences;

        std::deque<filmRoll> activeRolls;
        std::vector<char*>rollNames;
        int selRoll = 0;
        //std::vector<image> activeImages;



        //int selIm = -1;
        ImGuiSelectionBasicStorage selection;
        grainParams grain;
        unsigned int cpColorspace;

        // UI Toggles
        bool cropDisplay = true;
        bool minMaxDisp = false;
        bool sampleVisible = false;

        // Copy/Paste
        imageParams copyParams;
        imageMetadata copyMeta;
        copyPaste pasteOptions;
        bool pasteTrigger = false;
        metaBuff metaEdit;


        bool unsavedPopTrigger = false;
        bool globalMetaPopTrig = false;
        bool localMetaPopTrig = false;
        bool preferencesPopTrig = false;
        bool ackPopTrig = false;
        bool shortPopTrig = false;
        bool badOcioText = false;
        char ackMsg[512];
        char ackError[512];
        char ocioPath[1024];
        int ocioSel = 0;


        std::map<std::string, int> localMapping;
        std::map<std::string, int> globalMapping;
        std::vector<char*> items;

        // Metadata
        std::chrono::time_point<std::chrono::steady_clock> lastChange;
        bool metaRefresh = false;

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
        std::vector<std::string> importFiles;
        int impRoll = 0;
        bool newRollPopup = false;
        char rollNameBuf[64];
        char rollPath[1024];


        // Export
        std::thread exportThread;
        exportParam expSetting;
        bool exportPopup = false;
        bool expRolls = false;
        std::chrono::time_point<std::chrono::steady_clock> expStart;
        //bool overwrite = false;
        //int quality = 85;
        //std::string outPath = "";
        std::vector<char*> fileTypes;
        std::vector<char*> bitDepths;
        std::vector<char*> colorspaceSet;
        //int outType = 4;
        //int outDepth = 1;
        bool isExporting = false;
        int exportImgCount = 0;
        int exportProcCount = 0;
        unsigned int elapsedTime;

        // Views
        void menuBar();
        void imageView();
        void paramView();
        void thumbView();

        // windowUtls.cpp
        bool validRoll();
        bool validIm();
        image* activeImage();
        image* getImage(int index);
        image* getImage(int roll, int index);
        int activeRollSize();
        void paramUpdate();
        void removeRoll();

        // windowMeta.cpp
        void checkMeta();
        void imageMetaPreEdit();
        void imageMetaPostEdit();

        // windowKeys.cpp
        void checkHotkeys();


        void loadMappings();
        void actionA();
        void CalculateThumbDisplaySize(int imageWidth, int imageHeight, float maxHeight, ImVec2& outDisplaySize, int rotation);
        void calculateVisible();

        // windowIO.cpp
        void openImages();
        bool openJSON();
        void openRolls();
        void exportImages();
        void exportRolls();


        void clearSelection();


        void initRender(int start, int end);
        void imgRender();
        void imgRender(image *img);
        void rollRenderCheck();
        void rollRender();

        void analyzeImage();

        void createSDLTexture(image* actImage);
        void updateSDLTexture(image* actImage);

        // windowPopups.cpp
        void importImagePopup();
        void importRollPopup();
        void batchRenderPopup();
        void pastePopup();
        void unsavedRollPopup();
        void globalMetaPopup();
        void localMetaPopup();
        void preferencesPopup();
        void ackPopup();
        void shortcutsPopup();

        void copyIntoParams();
        void pasteIntoParams();
};


#endif
