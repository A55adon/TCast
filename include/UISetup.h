#pragma once

#include "UIManager.h"

class UISetup {
public:
// Startup function + helpers
    static void setupTabListeners();
    static void setupBrowseButtons();
    static void setupProjectActions();
    static void setupProjectSelection();
// Interface function + helpers
    static void setupDropdownListeners();
    static void setupFileDropdownListeners();
    static void setupSceneManagement();
    static void setupSceneContextMenu();
    static void setupResourcePanel();
    static void setupResourceContextMenu();
    static void setupProjectors();
    static void setupProjectorContextMenu();
    static void setupProjection();
};
void setStartupEventListeners();
void setInterfaceEventListeners();