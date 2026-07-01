//
// Created by ruut on 3/16/26.
//

#include "util/Logger.h"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <iostream>

#ifdef HAS_STD_FORMAT
#include <format>
#endif

// Allocate memory for mutex
std::mutex Logger::mutex;
int Logger::activeLevels = Logger::LogLevel::Warning | Logger::LogLevel::Error | Logger::LogLevel::Fatal;

void Logger::log(LogLevel level, const char* file, const char* function, const std::string &message) {
    if (!(level & activeLevels))
        return;

    std::lock_guard<std::mutex> lock(mutex);
    std::ostringstream oss;

#ifdef HAS_STD_FORMAT
    // Timestamp
    auto timestamp = std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now());
#endif

    // Logging color
    std::string colorCode;
    switch (level) {
    case Info:    colorCode = "\033[32m"; break; // green
    case Warning: colorCode = "\033[33m"; break; // yellow
    case Error:   colorCode = "\033[31m"; break; // red
    case Fatal:   colorCode = "\033[41m"; break; // red background
    default:      colorCode = "\033[0m";  break; // reset
    }

    auto filename = std::filesystem::path(file).filename().string();

    oss << colorCode
#ifdef HAS_STD_FORMAT
        << "[" << timestamp << "] "
#endif
        << "[" << getLevelName(level) << "] "
        << filename << " (" << function << ") "
        << message
        << std::endl;

    std::cout << oss.str();

    // Stop program if fatal error occurs
    if (level == Fatal)
        std::abort();
}

void Logger::activateLevel(LogLevel level) {
    activeLevels |= level;
}

void Logger::deactivateLevel(LogLevel level) {
    activeLevels &= ~level;
}

std::string Logger::getLevelName(LogLevel level) {
    switch (level) {
    case Info: return "INFO";
    case Warning: return "WARNING";
    case Error: return "ERROR";
    case Fatal: return "FATAL";
    default: return "UNKNOWN";
    }
}
