#pragma once

#include "Utilities.h"
#include "EventListener.h"

#include <iostream>
#include <iomanip>

static struct DragState {
    bool active       = false;
    bool ghostVisible = false;  // <-- add
    double startX     = 0;      // <-- add
    double startY     = 0;      // <-- add
    int  resourceId   = -1;
    std::string imagePath;
    Rml::Element* ghost = nullptr;
} drag;

static void endDrag() {
    if (drag.ghost) {
        if (auto* body = getEl("body"))
            body->RemoveChild(drag.ghost);
        drag.ghost = nullptr;
    }
    drag.active      = false;
    drag.ghostVisible = false;
    drag.resourceId  = -1;
    drag.imagePath.clear();
}

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
