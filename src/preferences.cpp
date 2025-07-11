
#include "preferences.h"
#include "nlohmann/json.hpp"
#include "logger.h"
#include "structs.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>

// Global Application Preferences
userPreferences appPrefs;

//--- Load From File ---//
/*
    Attempt to load the preferences in
    from disk
*/
void userPreferences::loadFromFile() {
    try {
        std::ifstream f(getPrefFile().c_str());
        if (!f)
            return;
        nlohmann::json prefObj;
        prefObj = nlohmann::json::parse(f);
        if (prefObj.contains("Preferences")) {
            prefs = prefObj["Preferences"].get<preferenceSet>();
            tmpAutoSave = prefs.autoSave;
            if (prefs.maxSimExports < 1) {
                prefs.maxSimExports = std::thread::hardware_concurrency();
                prefs.maxSimExports = prefs.maxSimExports < 1 ? 4 : prefs.maxSimExports;
            }

        }
    }  catch (const std::exception& e) {
        LOG_WARN("Unable to import preferences file: {}", e.what());
        return;
    }

}

//--- Save To File ---//
/*
    Write out the current preference set
    struct to disk
*/
void userPreferences::saveToFile() {
    try {
        nlohmann::json j;
       j["Preferences"] = prefs;

        std::string jDump = j.dump(4);
        std::ofstream jsonFile(getPrefFile(), std::ios::out | std::ios::trunc);
        if (!jsonFile) {
            LOG_ERROR("Unable to write preferences file!");
            return;
        }
        jsonFile << jDump;
        jsonFile.close();


    } catch (const std::exception& e) {
        LOG_WARN("Unable to export preferences file: {}", e.what());
        return;
    }
}

//--- Get Preference File ---//
/*
    Get the location of the preferences file
*/
std::string userPreferences::getPrefFile() {

    #if defined(WIN32)
    std::string appData;
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath)))
    {
        appData = szPath;
    }
    else
    {
        LOG_CRITICAL("Cannot query the AppData folder!");
    }
    appData += "\\Filmvert\\";
    std::string prefPath = appData;
    if (!std::filesystem::exists(prefPath))
        if (!std::filesystem::create_directory(prefPath))
            LOG_ERROR("Unable to create preference directory!");
    prefPath += std::string("/fv_pref.json");
    return prefPath;


    #elif defined __APPLE__
    char* homeDir = getenv("HOME");
    std::string homeStr = homeDir;
    homeStr += "/Library/Preferences/Filmvert/";
    if (!std::filesystem::exists(homeStr))
        if (!std::filesystem::create_directory(homeStr))
            LOG_ERROR("Unable to create preference directory!");
    homeStr += "fv_pref.json";
    return homeStr;


    #else
    char* homeDir = getenv("HOME");
    std::string homeStr = homeDir;
    homeStr += "/.local/Filmvert/";
    if (!std::filesystem::exists(homeStr))
        if (!std::filesystem::create_directory(homeStr))
            LOG_ERROR("Unable to create preference directory!");
    homeStr += "fv_pref.json";
    return homeStr;

    #endif
}

//--- Display Release Notes ---//
/*
    Checks the user preferences string against the
    app's current version. If the app is newer than
    what the preferences string is, then save the
    preferences with the updated version, and return
    true to display the release notes
*/
bool userPreferences::displayReleaseNotes() {

    std::string currentVersion = fmt::format("{}.{}.{}", VERMAJOR, VERMINOR, VERPATCH);
    if (prefs.verString != currentVersion) {
        // We have a new version, update the preferences
        // and display the popup
        prefs.verString = currentVersion;
        saveToFile();
        return true;
    }

    return false;
}
