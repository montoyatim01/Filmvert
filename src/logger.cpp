#include "logger.h"



std::shared_ptr<spdlog::logger> glLog::s_logger = nullptr;

std::shared_ptr<spdlog::logger>& glLog::GetLogger() {
    if(s_logger == nullptr) {
      #if defined(WIN32)  //Log to file
        std::string appData;
        char szPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath)))
        {
            appData = szPath;
        }
        else
        {
            // Something has gone horribly wrong!
        }
        appData += "\\Filmvert\\";
        std::string logPath = appData;
        logPath += std::string("/Filmvert.log");
        auto maxSize = 1024 * 1024 * 5;
        auto maxFiles = 4;
        std::wstring wLogPath = std::wstring(logPath.begin(), logPath.end());
        s_logger = spdlog::rotating_logger_mt("Filmvert", wLogPath, maxSize, maxFiles);
        s_logger->flush_on(spdlog::level::info);
        s_logger->set_pattern("[%Y-%m-%d %T.%e] [%n] [%l] %v");


        #elif defined __APPLE__
        std::string logPath = "/Users/Shared/Filmvert";
        logPath += std::string("/Filmvert.log");
        auto maxSize = 1024 * 1024 * 5;
        auto maxFiles = 4;
        s_logger = spdlog::rotating_logger_mt("Filmvert", logPath, maxSize, maxFiles);
        s_logger->flush_on(spdlog::level::info);
        s_logger->set_pattern("[%Y-%m-%d %T.%e] [%n] [%l] %v");


        #else
        char* homeDir = getenv("HOME");
        std::string homeStr = homeDir;
        std::string logPath = homeStr + "/.local/Filmvert";
        logPath += std::string("/Filmvert.log");
        auto maxSize = 1024 * 1024 * 5;
        auto maxFiles = 4;
        s_logger = spdlog::rotating_logger_mt("Filmvert", logPath, maxSize, maxFiles);
        s_logger->flush_on(spdlog::level::info);
        s_logger->set_pattern("[%Y-%m-%d %T.%e] [%n] [%l] %v");
        #endif
    }
    return s_logger;
}
