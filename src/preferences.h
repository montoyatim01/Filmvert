#ifndef _preferences_h_
#define _preferences_h_

#include "nlohmann/json.hpp"

#include <string>

struct preferenceSet {
    bool autoSave = false;
    int autoSFreq = 5;

    float histInt = 0.75;
    bool histEnable = true;

    bool perfMode = true;
    int maxRes = 3000;

    std::string ocioPath;
    bool ocioExt = false;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(preferenceSet, autoSave,
        autoSFreq, histInt, histEnable, perfMode, maxRes,
        ocioPath, ocioExt);
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
