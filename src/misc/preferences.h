#ifndef _preferences_h_
#define _preferences_h_

#if defined (WIN32)
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#endif

#include "nlohmann/json.hpp"
#include "updateCheck.h"
#include <array>
#include <string>

#define THREAD_LIMIT 128

struct aspectPreset {
    std::string name;
    float       value = 1.0f;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(aspectPreset, name, value);
};

struct preferenceSet {

    //--- General ---//

    // Max Undo States
    int undoLevels = 200;

    // Autosave
    bool autoSave = false;
    int autoSFreq = 5;

    // Mouse settings
    bool trackpadMode = false;

    bool checkUpdates = true;


    //--- Display ---//

    // Histogram Settings - in-app
    float histInt = 0.75;
    bool histEnable = true;

    std::array<float, 4> imageBGColor= {0.0588f, 0.0588f, 0.0588f, 0.9411f};
    std::array<float, 4> paramBGColor= {0.0588f, 0.0588f, 0.0588f, 0.9411f};
    std::array<float, 4> thumbBGColor= {0.0588f, 0.0588f, 0.0588f, 0.9411f};

    // Show/hide colors with alt key
    bool altGrades = true;

    // CMYK Sliders
    bool cmykSliders = false;

    // Color Picker Option
    int colorPicker = 0;

    // Viewer Setting (0 for Linear, 1 for nearest)
    int viewerSetting = 0;

    // Pixel Display Scale
    int pixelScale = 2;

    // Display Stats
    bool showStats = false;

    //--- System ---//

    // Analysis Minimum Offset
    float minOffset = 0.002f;

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
    bool gamutComp = false;

    //--- HIDDEN ---//

    // Auto-sort on import
    bool autoSort = true;

    // Resolution of proxy buffer
    // (only buffer left when roll is unloaded)
    float proxyRes = 0.35f;

    unsigned long renderTimeout = 90000;

    // Contact Sheet border size
    float contactSheetBorder = 0.02f;

    // Version String
    std::string verString;

    // CPU renders (full renders only)
    bool cpuRender = false;

    // Hold onto files
    bool holdFilesinRAM = false;

    // Aspect ratio presets (shown in crop panel)
    std::array<aspectPreset, 6> cropPresets = {{
        {"6x7",  1.200f},
        {"3:2",  1.500f},
        {"645",  1.333f},
        {"16:9", 1.777f},
        {"XPan", 2.708f},
        {"2.39", 2.390f}
    }};

    // Update Check
    bool clickThrough = false;
    uint64_t lastCheck = 1777335785;
    Version lastFound;


    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(preferenceSet, undoLevels,
        autoSave, autoSFreq, histInt, histEnable, trackpadMode,
        viewerSetting, pixelScale, perfMode, maxRes, rollTimeout, debayerMode, maxSimExports,
        ocioPath, ocioExt, gamutComp, showStats, altGrades, cmykSliders, colorPicker,
        autoSort, proxyRes, renderTimeout, contactSheetBorder, verString, cpuRender,
        minOffset, imageBGColor, paramBGColor, thumbBGColor, holdFilesinRAM, cropPresets,
        clickThrough, lastCheck, lastFound);
};

class userPreferences {

    public:
    userPreferences(){};
    ~userPreferences(){};

    void initPrefs();
    void loadFromFile();
    void saveToFile();
    bool displayReleaseNotes();


    bool tmpAutoSave = false;
    preferenceSet prefs;

    private:
    std::string getPrefFile();


};

extern userPreferences appPrefs;

#endif
