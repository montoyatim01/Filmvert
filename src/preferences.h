#ifndef _preferences_h_
#define _preferences_h_

#include <string>

class userPreferences {

    public:
    userPreferences(){};
    ~userPreferences(){};

    void loadFromFile();
    void saveToFile();

    bool autoSave = false;
    bool tmpAutoSave = false;
    int autoSFreq = 5;

    float histInt = 0.75;
    int maxRes = 3000;

    bool perfMode = true;
    std::string ocioPath;
    bool ocioExt = false;

    private:
    std::string getPrefFile();


};

extern userPreferences appPrefs;

#endif
