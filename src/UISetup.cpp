#include "UISetup.h"

#include "ResourceHandler.h"

void setStartupEventListeners() {
    UISetup::setupTabListeners();
    UISetup::setupBrowseButtons();
    UISetup::setupProjectActions();
    UISetup::setupProjectSelection();

    // Set default directory
    if (auto *el = getWindow().document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            input->SetValue(Utilities::toBackwardSlashes(Utilities::getSaveFolderPath()));
        }
    }

    UIManager::populateFolders("../saves/folderSaves/");
}

void UISetup::setupTabListeners() {
    if (auto* tabLoad = getEl("tab-load")) {
        tabLoad->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            getEl("tab-load-div")->SetAttribute("style", "display:flex");
            getEl("tab-new-div")->SetAttribute("style", "display:none");
            getEl("tab-load")->SetClass("active", true);
            getEl("tab-new")->SetClass("active", false);
        }));
    }

    if (auto* tabNew = getEl("tab-new")) {
        tabNew->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            getEl("tab-load-div")->SetAttribute("style", "display:none");
            getEl("tab-new-div")->SetAttribute("style", "display:flex");
            getEl("tab-new")->SetClass("active", true);
            getEl("tab-load")->SetClass("active", false);
        }));
    }

    if (auto* tabFolder = getEl("tab-folder")) {
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabFolder] {
            getEl("tab-folder-div")->SetProperty("display", "flex");
            getEl("tab-tct-div")->SetProperty("display", "none");
            getEl("browse-folder-btn")->SetProperty("display", "block");
            getEl("browse-tct-btn")->SetProperty("display", "none");
            tabFolder->SetClassNames("tab-button active");
            getEl("tab-tct")->SetClassNames("tab-button");
        }));
    }

    if (auto* tabTct = getEl("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabTct] {
            getEl("tab-folder-div")->SetProperty("display", "none");
            getEl("tab-tct-div")->SetProperty("display", "flex");
            getEl("browse-folder-btn")->SetProperty("display", "none");
            getEl("browse-tct-btn")->SetProperty("display", "block");
            tabTct->SetClassNames("tab-button active");
            getEl("tab-folder")->SetClassNames("tab-button");
        }));
    }
}
void setInput(const std::string& id, const std::string& value) {
    auto* el = getEl(id);
    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
        input->SetValue(Utilities::toBackwardSlashes(value));
    }
}
void UISetup::setupBrowseButtons() {
    if (auto* btn = getEl("browse-folder-btn")) {
        btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            auto folder = Utilities::browseFolder();
            if (!folder.empty()) setInput("load-dir-input", folder);
        }));
    }

    if (auto* btn = getEl("browse-load-btn")) {
        btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            auto folder = Utilities::browseFolder();
            if (!folder.empty()) setInput("load-dir-input", folder);
        }));
    }

    if (auto* btn = getEl("browse-tct-btn")) {
        btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            auto file = Utilities::browseTCTFile();
            if (!file.empty()) setInput("load-dir-input", file);
        }));
    }

    if (auto* btn = getEl("browse-btn")) {
        btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            auto folder = Utilities::browseFolder();
            if (!folder.empty()) setInput("project-dir-input", folder);
        }));
    }
}
void UISetup::setupProjectActions() {
    // Save new project
    if (auto *saveNewProjectBtn = getEl("save-btn")) {
        saveNewProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (UIManager::createProject()) {
                getWindow().document->Hide();
                std::cout << "Project created: " << saveData.projectName << std::endl;
                if ((getWindow().document = getWindow().context->LoadDocument("assets/interface.rml"))) {
                    getWindow().document->Show();
                }
                setInterfaceEventListeners();
            }
            // TODO: Add user feedback
        }));
    }

    // Load project
    if (auto *loadProjectBtn = getEl("load-btn")) {
        loadProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (UIManager::loadProject()) {
                getWindow().document->Hide();
                std::cout << "Project loaded: " << saveData.projectName << std::endl;
                if ((getWindow().document = getWindow().context->LoadDocument("assets/interface.rml"))) {
                    getWindow().document->Show();
                }
                setInterfaceEventListeners();
            }
            // TODO: Add user feedback
        }));
    }

    if (auto* slider = getEl("projector-count-input")) {
        slider->AddEventListener(Rml::EventId::Change, new ButtonHandler([] {
            auto* sliderEl = dynamic_cast<Rml::ElementFormControl*>(
                getWindow().document->GetElementById("projector-count-input"));

            auto* valueEl = getWindow().document->GetElementById("projector-count-value");

            if (sliderEl && valueEl) {
                int value = static_cast<int>(std::stof(sliderEl->GetValue()));
                valueEl->SetInnerRML(std::to_string(value));

            }
        }));
    }

}
void UISetup::setupProjectSelection() {
    for (int i = 1; i <= 5; i++) {
        std::string id = "folder-proj-" + std::to_string(i);
        if (auto *proj = getWindow().document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [name = proj->GetInnerRML()] {
                                           UIManager::setSelectedProject(name);
                                       }));
        }

        id = "tct-proj-" + std::to_string(i);
        if (auto *proj = getWindow().document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [name = proj->GetInnerRML()] {
                                           UIManager::setSelectedProject(name);
                                       }));
        }
    }
}

void setInterfaceEventListeners() {
    ResourceHandler::initResources();

    if (!UIManager::loadScenesData()) {
        std::cerr << "Failed to load scenesData" << std::endl;
    }
    if (auto *projectname = getWindow().document->GetElementById("project-name")) {
        std::cout << "[Info] Setting project name: " << saveData.projectName << std::endl;
        projectname->SetInnerRML(saveData.projectName);
    }

    UISetup::setupDropdownListeners();
    UISetup::setupSceneManagement();
    UISetup::setupSceneContextMenu();

    UISetup::setupResourcePanel();
    UISetup::setupResourceContextMenu();

    UISetup::setupProjectors();
    UISetup::setupProjectorContextMenu();

    UISetup::setupProjection();

}

void UISetup::setupDropdownListeners() {
    setupFileDropdownListeners();
}
void UISetup::setupFileDropdownListeners() {
    if (auto* el = getEl("file-dropdown-newproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            UIManager::switchToStartup();

            getEl("tab-load-div")->SetAttribute("style", "display:none");
            getEl("tab-new-div")->SetAttribute("style", "display:flex");

            getEl("tab-new")->SetClass("active", true);
            getEl("tab-load")->SetClass("active", false);

            getEl("tab-load")->SetAttribute("style", "display:none");
        }));
    }

    if (auto* el = getEl("file-dropdown-loadproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            UIManager::switchToStartup();

            getEl("tab-load-div")->SetAttribute("style", "display:flex");
            getEl("tab-new-div")->SetAttribute("style", "display:none");

            getEl("tab-load")->SetClass("active", true);
            getEl("tab-new")->SetClass("active", false);

            getEl("tab-new")->SetAttribute("style", "display:none");
        }));
    }

    if (auto* el = getEl("file-dropdown-exportproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            try {
                std::string fullPath = saveData.path.string() + "\\" + saveData.projectName;

                if (std::filesystem::exists(fullPath + ".tct"))
                    std::filesystem::remove_all(fullPath + ".tct");

                std::string command =
                    "powershell Compress-Archive -Path \"" + fullPath +
                    "\\*\" -DestinationPath \"" + fullPath + ".zip\" -Force";

                int result = std::system(command.c_str());
                if (result != 0) {
                    std::cerr << "Failed to zip folder. Exit code: " << result << '\n';
                    return;
                }

                std::filesystem::rename(fullPath + ".zip", fullPath + ".tct");
                std::cout << "File " << saveData.projectName
                          << ".tct exported successfully to: "
                          << saveData.path.string() << '\n';

            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Filesystem error: " << e.what() << '\n';
            }
        }));
    }

    if (auto* el = getEl("file-dropdown-importproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            // TODO
        }));
    }

    if (auto* el = getEl("file-dropdown-closeproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            UIManager::switchToStartup();
        }));
    }

    if (auto* el = getEl("file-dropdown-closeprogramm")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            exit(EXIT_SUCCESS);
        }));
    }
}
void UISetup::setupSceneManagement() {
     UIManager::refreshScenes();

    if (auto *addSceneButton = getEl("add-scene-btn")) {
        addSceneButton->AddEventListener(Rml::EventId::Click,
                                         new ButtonHandler([]() {
                                             sceneManager.scenes.emplace_back(SceneData{
                                                 "Szene " + std::to_string(sceneManager.scenes.size() + 1),
                                                 {}
                                             });
                                             UIManager::saveProject(projectPath);

                                             UIManager::refreshScenes();
                                         })
        );
    }
    auto sceneButtonsArrowUp = getEl("scene-buttons-arrow-up");
        sceneButtonsArrowUp->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
                if (activeSceneIndex != -1 && activeSceneIndex != 0) {
                    SceneData temp = sceneManager.scenes[activeSceneIndex - 1];
                    sceneManager.scenes[activeSceneIndex - 1] = sceneManager.scenes[activeSceneIndex];
                    sceneManager.scenes[activeSceneIndex] = temp;
                    activeSceneIndex--;
                    UIManager::saveProject(projectPath);

                    UIManager::refreshScenes();
                }
            })
        );
    auto sceneButtonsArrowDown = getEl("scene-buttons-arrow-down");
        sceneButtonsArrowDown->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
                if (activeSceneIndex != -1 && activeSceneIndex != sceneManager.scenes.size() - 1) {
                    SceneData temp = sceneManager.scenes[activeSceneIndex + 1];
                    sceneManager.scenes[activeSceneIndex + 1] = sceneManager.scenes[activeSceneIndex];
                    sceneManager.scenes[activeSceneIndex] = temp;
                    activeSceneIndex++;

                    UIManager::saveProject(projectPath);
                    UIManager::refreshScenes();
                }
            })
        );
    auto sceneButtonsDeleteAll = getEl("scene-buttons-delete-all");
        sceneButtonsDeleteAll->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
            //TODO: confirm
            sceneManager.scenes.clear();

            activeSceneIndex = 0;
            UIManager::saveProject(projectPath);
            UIManager::refreshScenes();
        }));


    std::cout << "Scenes count: " << sceneManager.scenes.size() << std::endl;
}
void UISetup::setupSceneContextMenu() {
    // Rename button in context menu
    if (auto *contextRename = getEl("scene-context-rename")) {
        contextRename->AddEventListener(Rml::EventId::Click, new SceneContextMenuHandler(getWindow().document, "rename"));
    }

    if (auto *contextDelete = getEl("scene-context-delete")) {
        contextDelete->AddEventListener(Rml::EventId::Click, new SceneContextMenuHandler(getWindow().document, "delete"));
    }

    if (auto *contextDuplicate = getEl("scene-context-duplicate")) {
        contextDuplicate->AddEventListener(Rml::EventId::Click,
                                           new SceneContextMenuHandler(getWindow().document, "duplicate"));
    }

    if (auto *body = getEl("body")) {
        body->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (auto *contextMenu = getEl("sceneContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }));
    }

    getWindow().document->AddEventListener(Rml::EventId::Keydown, new KeyEventHandler([](Rml::Event &event) {
        if (event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_ESCAPE) {
            if (auto *contextMenu = getEl("sceneContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }
    }));
}

void UISetup::setupResourcePanel() {
    auto* dialog = getEl("add-resource-dialog");
    auto* addBtn = getEl("add-resource-btn");
    auto* cancelBtn = getEl("resource-cancel-btn");
    auto* confirmBtn = getEl("resource-confirm-btn");

    auto* browseBtn = getEl("browse-png-btn");
    auto* fileInput = getEl("resource-file");
    auto* nameInput = getEl("resource-name");

    addBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([dialog, fileInput, nameInput]{
        dialog->SetAttribute("style", "display:flex;");
        fileInput->SetAttribute("value", "");
        nameInput->SetAttribute("value", "");
    }));


    browseBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([fileInput, nameInput] {
        std::string paths = Utilities::browseImageOrMp4();
        if (paths.empty()) return;

        fileInput->SetAttribute("value", paths);

        Rml::String val;
        nameInput->GetAttribute("value", val);

        if (val.empty()) {
            std::stringstream ss(paths);
            std::string item;
            std::vector<std::string> names;

            while (std::getline(ss, item, ',')) {
                names.push_back(std::filesystem::path(item).stem().string());
            }

            std::string joined;
            for (size_t i = 0; i < names.size(); i++) {
                if (i > 0) joined += ",";
                joined += names[i];
            }

            nameInput->SetAttribute("value", joined);
        }
    }));


    cancelBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([dialog]{
        dialog->SetAttribute("style", "display:none;");
    }));

    auto* fileInputEl = dynamic_cast<Rml::ElementFormControlInput*>(fileInput);
    auto* nameInputEl = dynamic_cast<Rml::ElementFormControlInput*>(nameInput);

    confirmBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([dialog, fileInputEl, nameInputEl] {

        if (!fileInputEl || !nameInputEl) return;

        std::string filePaths = fileInputEl->GetValue();
        std::string names = nameInputEl->GetValue();

        if (filePaths.empty()) {
            Utilities::showError("Bitte eine Datei angeben!");
            return;
        }
        if (names.empty()) {
            Utilities::showError("Bitte einen Namen angeben!");
            return;
        }

        // Split comma-separated lists
        std::vector<std::string> pathList;
        std::vector<std::string> nameList;

        {
            std::stringstream ss(filePaths);
            std::string item;
            while (std::getline(ss, item, ',')) pathList.push_back(item);
        }
        {
            std::stringstream ss(names);
            std::string item;
            while (std::getline(ss, item, ',')) nameList.push_back(item);
        }

        if (pathList.size() != nameList.size()) {
            Utilities::showError("Anzahl der Dateien und Namen stimmt nicht überein!");
            return;
        }

        for (size_t i = 0; i < pathList.size(); ++i)
            ResourceHandler::createResource(pathList[i], nameList[i]);

        dialog->SetAttribute("style", "display:none;");
        UIManager::refreshResourcePanel();
    }));

    UIManager::refreshResourcePanel();
}

void UISetup::setupResourceContextMenu() {
    // Rename button in context menu
    if (auto *contextRename = getEl("resource-context-rename")) {
        contextRename->AddEventListener(Rml::EventId::Click, new ResourceContextMenuHandler(getWindow().document, "rename"));
    }

    if (auto *contextDelete = getEl("resource-context-delete")) {
        contextDelete->AddEventListener(Rml::EventId::Click, new ResourceContextMenuHandler(getWindow().document, "delete"));
    }

    if (auto *body = getEl("body")) {
        body->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (auto *contextMenu = getEl("resourceContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }));
    }

    getWindow().document->AddEventListener(Rml::EventId::Keydown, new KeyEventHandler([](Rml::Event &event) {
        if (event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_ESCAPE) {
            if (auto *contextMenu = getEl("resourceContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }
    }));
}

void UISetup::setupProjectors() {
    UIManager::regenerateSplitSources();
    UIManager::refreshProjectors();
}

void UISetup::setupProjectorContextMenu() {
    // Select Resource button in context menu
    if (auto *contextRename = getEl("projector-context-selectResource")) {
        contextRename->AddEventListener(Rml::EventId::Click, new ProjectorContextMenuHandler(getWindow().document, "selectResource"));
    }


    if (auto *body = getEl("body")) {
        body->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (auto *contextMenu = getEl("projectorContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }));
    }

    getWindow().document->AddEventListener(Rml::EventId::Keydown, new KeyEventHandler([](Rml::Event &event) {
        if (event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_ESCAPE) {
            if (auto *contextMenu = getEl("projectorContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }
    }));
}

void UISetup::setupProjection() {
    auto *start = getEl("project-sources");
    start->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {

        // --- Stop and clear existing projectors ---
        for (auto& proj : projectors) {
            proj->requestDie();
            if (proj->th.joinable())
                proj->th.join();
            delete proj;
        }
        projectors.clear();

        // --- Regenerate split sources according to current connections ---
        UIManager::regenerateSplitSources();

        // --- Create new projectors ---
        for (int i = 0; i < saveData.projectorCount; i++) {
            std::string imgPath = sceneManager.scenes[activeSceneIndex].splitSources[i];

            // If no split image, fallback to original source
            if (imgPath.empty())
                imgPath = sceneManager.scenes[activeSceneIndex].sources[i];

            auto p = new Projector(i + 1, imgPath);
            projectors.push_back(p);
        }

    }));

    auto *stop = getEl("stop-projection");
    stop->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {

        // --- Stop all projectors safely ---
        for (auto& proj : projectors) {
            proj->requestDie();
        }

        for (auto& proj : projectors) {
            if (proj->th.joinable())
                proj->th.join();
            delete proj;
        }

        projectors.clear();
        std::cout << "[Info] Projectors stopped, count: " << projectors.size() << std::endl;
    }));
}
