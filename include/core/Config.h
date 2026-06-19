//
// Created by ruut on 5/25/26.
//

#ifndef SPHLASH_CONFIG_H
#define SPHLASH_CONFIG_H

#include <string>
#include "nlohmann/json.hpp"

class Config {
public:
    bool load(const std::string& path);
    bool save(const std::string& path = "") const;

    template<typename T>
    T get(const std::string& key, const T& defaultValue, const std::string& section = "") {
        if (config.empty()) {
            return defaultValue;
        }

        if (section.empty()) {
            return config.value(key, defaultValue);
        }

        if (config.contains(section))
            return config[section].value(key, defaultValue);

        return defaultValue;
    }

    template<typename T>
    void set(const std::string& key, const T& value, const std::string& section = "") {
        if (section.empty()) {
            config[key] = value;
        } else {
            config[section][key] = value;
        }
    }

private:
    std::string path;
    nlohmann::json config;
};

#endif //SPHLASH_CONFIG_H
