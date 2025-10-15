// ProjectManager.h
#pragma once
#include "Data.h"
#include "Window.h"
#include "PathUtils.h"
#include <string>
#include <filesystem>
#include <regex>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>

extern SaveData g_saveData;
extern SceneManager g_sceneManager;
extern std::filesystem::path g_projectFolderPath;
extern std::filesystem::path g_projectDataPath;
extern std::filesystem::path g_scenesDataPath;
extern bool g_createRecentPath;
extern Window g_window;

bool ValidateBeamerCount(const std::string& value, int& outCount);
bool SaveScenesData();
void LoadScenesData(const std::string& filename);
bool SaveNewProject();
bool SaveProject();
bool SaveProjectAs();
bool LoadProject();
bool ExportProject();
bool ImportProject();
void UpdateGlobalPaths();