#pragma once

#include "UIManager.h"

class UISetup {
public:
// Startupfunction + helpers

    static void setupTabListeners();
    static void setupBrowseButtons();
    static void setupProjectActions();
    static void setupProjectSelection();

// Interfacefunction + helpers

    static void setupDropdownListeners();
    static void setupFileDropdownListeners();
    static void setupProjectorGrid();
    static void setupSceneManagement();
    static void setupSceneContextMenu();
    static void setupResourcePanel();
    static void setupResourceContextMenu();
    static void setupProjectors();
    static void setupProjectorContextMenu();

};
void setStartupEventListeners();
void setInterfaceEventListeners();