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
    int autoSFreq = 5;

    bool perfMode = true;
    std::string ocioPath;
    bool ocioExt = false;

    private:
    std::string getPrefFile();


};

extern userPreferences appPrefs;

#endif
