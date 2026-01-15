#pragma once

#include "Utilities.h"
#include "EventListener.h"


class UIManager {
public:
// SaveFunction + helpers
    static bool saveProject();
    static bool saveProject(const std::filesystem::path &savePath);

    static bool verifyFolderStructure(const std::filesystem::path &savePath);
    static void fixFolderStructure(const std::filesystem::path &savePath);
    static bool saveJsonToFile(const std::filesystem::path &savePath, const json &data);

// LoadFunction + helpers (createRecentPath())
    static bool loadProject();
    static bool loadProject(const std::filesystem::path& loadPath);

// CreateFunction + helpers
    static bool createProject();

    static std::string* validateInputField(std::string &value, const std::string &fieldName);
    static bool createRecentPathFile(const std::filesystem::path &savePath);

// Other
    static void refreshScenes();
    static void setSelectedProject(const std::string &name);
    static void populateFolders(const std::string &path);
    static bool loadScenesData();
    static void switchToStartup();
    static void refreshResourcePanel();
    static void refreshProjectors();

    static void regenerateSplitSources();
};
