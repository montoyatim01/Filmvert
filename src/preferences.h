#ifndef _preferences_h_
#define _preferences_h_

#if defined (WIN32)
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#endif

#include "nlohmann/json.hpp"

#include <string>

struct preferenceSet {

    // VISIBLE SETTINGS

    // Max Undo States
    int undoLevels = 200;

    // Autosave
    bool autoSave = false;
    int autoSFreq = 5;

    // Histogram Settings
    float histInt = 0.75;
    bool histEnable = true;

    // Mouse settings
    bool trackpadMode = false;

    // Performance Mode
    bool perfMode = true;
    int maxRes = 3000;
    int rollTimeout = 30;

    // Debayer Mode
    int debayerMode = 10;

    // Max simultaneous exports
    int maxSimExports = -1;

    // OCIO
    std::string ocioPath;
    int ocioExt = 0;

    // HIDDEN SETTINGS
    // Auto-sort on import
    bool autoSort = true;

    // Resolution of proxy buffer
    // (only buffer left when roll is unloaded)
    float proxyRes = 0.35f;

    unsigned long renderTimeout = 90000;

    // Viewer Setting (0 for Linear, 1 for nearest)
    int viewerSetting = 0;

    // Contact Sheet border size
    float contactSheetBorder = 0.02f;



    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(preferenceSet, undoLevels,
        autoSave, autoSFreq, histInt, histEnable, trackpadMode,
        viewerSetting, perfMode, maxRes, rollTimeout, debayerMode,
        maxSimExports, ocioPath, ocioExt, autoSort, proxyRes,
        renderTimeout, contactSheetBorder);
};

class userPreferences {

    public:
    userPreferences(){};
    ~userPreferences(){};

    void loadFromFile();
    void saveToFile();


    bool tmpAutoSave = false;
    preferenceSet prefs;

    private:
    std::string getPrefFile();


};

extern userPreferences appPrefs;

#endif
