#pragma once

#include "UIManager.h"

class UISetup {
public:
// Startupfunction + helpers
    void setStartupEventListeners();

    void setupTabListeners();
    void setupBrowseButtons();
    void setupProjectActions();
    void setupProjectSelection();

// Interfacefunction + helpers
    void setInterfaceEventListeners();

    void setupDropdownListeners();
    void setupFileDropdownListeners();
    void setupProjectorGrid();
    void setupSceneManagement();
    void setupSceneContextMenu();


};