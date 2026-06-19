//
// Created by ruut on 5/25/26.
//

#include "core/Config.h"
#include <format>
#include <fstream>
#include <iostream>
#include "util/Logger.h"

bool Config::load(const std::string& path) {
    this->path = path;

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN(std::format("Could not load config file on '{}'.", path));
        return false;
    }

    try {
        file >> config;
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("Could not parse config file");
        return false;
    }
}

bool Config::save(const std::string& path) const {
    if (config.empty()) {
        return false;
    }

    std::string configPath;

    if (!path.empty()) {
        configPath = path;
    } else {
        configPath = this->path;
    }

    if (configPath.empty()) {
        return false;
    }

    // Create directory if needed
    std::filesystem::path filePath(configPath);

    std::error_code ec;
    std::filesystem::create_directories(filePath.parent_path(), ec);

    if (ec) {
        LOG_ERROR("Failed to create config directory.");
        return false;
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR("Could not open destination for saving config file.");
        return false;
    }

    file << config.dump(4);
    return true;
}
