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

//ImGui
#include "grainParams.h"
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

struct copyPaste {

    bool baseColor = false;
    bool cropPoints = false;
    bool analysisBlur = false;
    bool analysis = false;


    bool temp = false;
    bool tint = false;
    bool bp = false;
    bool wp = false;
    bool lift = false;
    bool gain = false;
    bool mult = false;
    bool offset = false;
    bool gamma = false;

    void analysisGlobal(){
        baseColor = !(baseColor && true);
        cropPoints = !(cropPoints && true);
        analysisBlur = !(analysisBlur && true);
        analysis = !(analysis && true);
    }

    void gradeGlobal(){
        temp = !(temp && true);
        tint = !(tint && true);
        bp = !(bp && true);
        wp = !(wp && true);
        lift = !(lift && true);
        gain = !(gain && true);
        mult = !(mult && true);
        offset = !(offset && true);
        gamma = !(gamma && true);
    }
};

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
        std::vector<image> activeImages;

        int winWidth;
        int winHeight;
        ImVec2 cursorPos;
        float dispScale = 1.0f;
        ImVec2 dispSize;
        ImVec2 scroll;

        int selIm = -1;
        ImGuiSelectionBasicStorage selection;
        grainParams grain;
        unsigned int cpColorspace;

        // UI Toggles
        bool cropDisplay = true;
        bool minMaxDisp = false;

        // Copy/Paste
        imageParams copyParams;
        copyPaste pasteOptions;
        bool pasteTrigger = false;


        std::map<std::string, int> localMapping;
        std::map<std::string, int> globalMapping;
        std::vector<char*> items;

        // Render timings
        bool renderCall = false;
        bool isRendering = false;
        std::chrono::time_point<std::chrono::steady_clock> lastRender;

        // Interaction timings
        float interactionTimer = 0.0f;
        const float INTERACTION_TIMEOUT = 0.25f; // seconds to wait after last interaction
        bool isInteracting = false;
        bool interactCall = false;

        // Export
        std::thread exportThread;
        bool exportPopup = false;
        bool overwrite = false;
        int quality = 85;
        std::string outPath = "";
        std::vector<char*> fileTypes;
        std::vector<char*> bitDepths;
        int outType = 4;
        int outDepth = 1;
        bool isExporting = false;
        int numIm = 0;
        int curIm = 0;
        unsigned int elapsedTime;

        void loadMappings();
        void actionA();
        void CalculateDisplaySize(int imageWidth, int imageHeight, ImVec2 maxAvailable, ImVec2& outDisplaySize);
        void calculateVisible();
        void openImages();
        void saveImage();
        void openDirectory();

        void loadPreset();
        void savePreset();

        void clearSelection();
        bool validIm();

        void initRender(int start, int end);
        void imgRender();
        void imgRender(image *img);

        void analyzeImage();

        void createSDLTexture(image* actImage);
        void updateSDLTexture(image* actImage);

        void batchRenderPopup();
        void pastePopup();

        void copyIntoParams();
        void pasteIntoParams();
};


#endif
