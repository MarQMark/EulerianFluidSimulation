//
// Created by ruut on 3/16/26.
//

#ifndef SPHLASH_LOGGER_H
#define SPHLASH_LOGGER_H

#define LOG_INFO(msg) Logger::log(Logger::LogLevel::Info, __FILE__, __func__, msg);
#define LOG_WARN(msg) Logger::log(Logger::LogLevel::Warning, __FILE__, __func__, msg);
#define LOG_ERROR(msg) Logger::log(Logger::LogLevel::Error, __FILE__, __func__, msg);
#define LOG_FATAL(msg) Logger::log(Logger::LogLevel::Fatal, __FILE__, __func__, msg);

#include <mutex>
#include <string>

class Logger {
public:
    enum LogLevel {
        Info = 1 << 0,
        Warning = 1 << 1,
        Error = 1 << 2,
        Fatal = 1 << 3,
    };

    static void log(LogLevel level, const char* file, const char* function, const std::string& message);
    static void activateLevel(LogLevel level);
    static void deactivateLevel(LogLevel level);

private:
    static std::mutex mutex;
    static int activeLevels;

    static std::string getLevelName(LogLevel level);
};

#endif //FLUIDSIMSPH_LOGGER_H