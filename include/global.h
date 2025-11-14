#pragma once

#include <utility>

#include "Window.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct SaveData {
    std::string projectName;
    int projectorCount{};
    std::string description;
    std::string path;
};

struct SceneData {
    std::string sceneName;
    std::vector<std::string> sources;

    SceneData()= default;

    SceneData(std::string  name, std::vector<std::string> src)
        : sceneName(std::move(name)), sources(std::move(src)) {}
};

struct SceneManager {
    std::vector<SceneData> scenes;
};


// GLOBAL

