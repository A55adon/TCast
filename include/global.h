#pragma once

#include <utility>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <shobjidl.h>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <shlobj.h>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>

#include "Shell.h"
#include "ButtonListener.h"
#include "Window.h"


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

inline Window& getWindow() {
    static Window window(1920,1080);
    return window;
}

// Shared Functions

void renameScene(int index);
void deleteScene(int index);
void duplicateScene(int index);
void selectScene(int index);
void showRenameDialog(int index);

// Global Variables
inline SaveData saveData;
inline SceneManager sceneManager;
inline bool createRecentPath = true;
inline std::filesystem::path projectPath;
inline int activeSceneIndex = -1; // -1 means no scene selected

