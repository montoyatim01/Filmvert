
#include "preferences.h"
#include "nlohmann/json.hpp"
#include "logger.h"
#include <cstdlib>
#include <fstream>

userPreferences appPrefs;

void userPreferences::loadFromFile() {
    try {
        std::ifstream f(getPrefFile().c_str());

        if (!f)
            return;
        nlohmann::json prefObj;
        prefObj = nlohmann::json::parse(f);

        if (prefObj.contains("autoSave")) {
            autoSave = prefObj["autoSave"].get<bool>();
        }
        if (prefObj.contains("saveFreq")) {
            autoSFreq = prefObj["saveFreq"].get<int>();
        }
        if (prefObj.contains("perfMode")) {
            perfMode = prefObj["perfMode"].get<bool>();
        }
        if (prefObj.contains("ocioPath")) {
            ocioPath = prefObj["ocioPath"].get<std::string>();
        }
        if (prefObj.contains("ocioExt")) {
            ocioExt = prefObj["ocioExt"].get<bool>();
        }

    }  catch (const std::exception& e) {
        LOG_WARN("Unable to import preferences file: {}", e.what());
        return;
    }

}

void userPreferences::saveToFile() {
    try {
        nlohmann::json j;
        j["autoSave"] = autoSave;
        j["saveFreq"] = autoSFreq;
        j["perfMode"] = perfMode;
        j["ocioPath"] = ocioPath;
        j["ocioExt"] = ocioExt;

        std::string jDump = j.dump(4);
        std::ofstream jsonFile(getPrefFile(), std::ios::out | std::ios::trunc);
        if (!jsonFile) {
            LOG_ERROR("Unable to open JSON file for roll metadata export!");
            return;
        }
        jsonFile << jDump;
        jsonFile.close();


    } catch (const std::exception& e) {
        LOG_WARN("Unable to import preferences file: {}", e.what());
        return;
    }
}

std::string userPreferences::getPrefFile() {
    #ifdef __APPLE__
    char* homeDir = getenv("HOME");
    std::string homeStr = homeDir;
    homeStr += "/Library/Preferences/";
    homeStr += "filmvert.json";

    return homeStr;
    #else
    return "";
    #endif
}
