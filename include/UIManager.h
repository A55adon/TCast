#pragma once

#include "Utilities.h"
#include "EventListener.h"

class UIManager {
public:
// SaveFunction + helpers
    void saveProject();

    void verifyFolderStructure();
    void fixFolderStructure();
    void saveJsonToFile();

// LoadFunction + helpers (createRecentPath())
    void loadProject();

// CreateFunction + helpers
    void createProject();

    void validateProjectorCount();
    void validateInputField();
    void createRecentPathFile();

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
