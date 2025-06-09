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

    // OCIO
    std::string ocioPath;
    bool ocioExt = false;

    // Auto-sort on import
    bool autoSort = true;

    float proxyRes = 0.35f;

    unsigned long renderTimeout = 90000;



    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(preferenceSet, undoLevels,
        autoSave, autoSFreq, histInt, histEnable, trackpadMode,
        perfMode, maxRes, rollTimeout, debayerMode, ocioPath,
        ocioExt, autoSort, proxyRes, renderTimeout);
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
