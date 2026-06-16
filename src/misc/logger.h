#ifndef _LOGGER_H
#define _LOGGER_H

#if defined (WIN32)
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#endif

#include <string>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

class glLog
{
public:
  static std::shared_ptr<spdlog::logger>& GetLogger();
private:
  static std::shared_ptr<spdlog::logger> s_logger;

};

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(glLog::GetLogger(), __VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_LOGGER_INFO(glLog::GetLogger(), __VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_LOGGER_WARN(glLog::GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(glLog::GetLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(glLog::GetLogger(), __VA_ARGS__)



#endif
