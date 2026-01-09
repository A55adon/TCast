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
#include <chrono>

#include "Shell.h"
#include "Window.h"

using json = nlohmann::json;

struct SaveData {
    std::string projectName;
    int projectorCount{};
    std::string description;
    std::filesystem::path path;
};

struct SceneData {
    std::string sceneName;
    std::vector<std::string> sources; // [0] would be the name of the resource of the first projector
    std::vector<std::string> splitSources; // for connections

    std::vector<int> connection;

    SceneData()= default;

    SceneData(std::string  name, std::vector<std::string> src)
        : sceneName(std::move(name)), sources(std::move(src)) {}
};

inline SaveData saveData;

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
void showSceneRenameDialog(int index);

void showResourceRenameDialog(int index);
void deleteResource(int index);
void selectResource(int resourceIndex);

void connect(int index);
void disconnect(int index);

void showProjectorResourceSelection(int index);

// For switching in UI helper functions
void setStartupEventListeners();
void setInterfaceEventListeners();

// Global Variables
inline SceneManager sceneManager;
inline bool createRecentPath = true;
inline std::filesystem::path projectPath;
inline std::vector<Projector*> projectors;
inline int activeSceneIndex = 0; // -1 means no scene selected
inline int activeResourceIndex = -1; // -1 means no scene selected



