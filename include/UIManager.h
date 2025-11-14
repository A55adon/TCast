#pragma once

#include "Utilities.h"
#include "EventListener.h"

class UIManager {
public:
// SaveFunction + helpers
    static bool saveProject();
    static bool saveProject(const std::filesystem::path &savePath);

    static bool verifyFolderStructure(const std::filesystem::path &projectPath);
    static void fixFolderStructure(const std::filesystem::path &savePath);
    static bool saveJsonToFile(const std::filesystem::path &savePath, const json &data);

// LoadFunction + helpers (createRecentPath())
    static bool loadProject();
    static bool loadProject(std::filesystem::path loadPath);

// CreateFunction + helpers
    static bool createProject();

    static std::optional<int> validateProjectorCount(const std::string &value);
    static std::string* validateInputField(std::string &value, const std::string &fieldName);
    static bool createRecentPathFile(const std::filesystem::path &savePath);

// Other
    void refreshScenes();
    void setSelectedProject();
    void populateFolders();
    void loadScenesData();
    void switchToStartup();
    void selectScene(int index);

// Scene Contextmenu
    void showRenameDialog();
    void renameScene();
    void deleteScene();
    void duplicateScene();

};
