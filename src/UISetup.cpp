#include "UISetup.h"

#include "ResourceHandler.h"
#include "RmlUi_Backend.h"

void setStartupEventListeners() {
    UISetup::setupTabListeners();
    UISetup::setupBrowseButtons();
    UISetup::setupProjectActions();
    UISetup::setupProjectSelection();

    // Set default directory
    if (auto *el = getWindow().document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            input->SetValue(Utilities::toBackwardSlashes(Utilities::getSaveFolderPath()).string());
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

    //if (auto* tabFolder = getEl("tab-folder")) {
    //    tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabFolder] {
    //        getEl("tab-folder-div")->SetProperty("display", "flex");
    //        getEl("tab-tct-div")->SetProperty("display", "none");
    //        getEl("browse-folder-btn")->SetProperty("display", "block");
    //        getEl("browse-tct-btn")->SetProperty("display", "none");
    //        tabFolder->SetClassNames("tab-button active");
    //        getEl("tab-tct")->SetClassNames("tab-button");
    //    }));
    //}

    //if (auto* tabTct = getEl("tab-tct")) {
    //    tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabTct] {
    //        getEl("tab-folder-div")->SetProperty("display", "none");
    //        getEl("tab-tct-div")->SetProperty("display", "flex");
    //        getEl("browse-folder-btn")->SetProperty("display", "none");
    //        getEl("browse-tct-btn")->SetProperty("display", "block");
    //        tabTct->SetClassNames("tab-button active");
    //        getEl("tab-folder")->SetClassNames("tab-button");
    //    }));
    //}
}
void setInput(const std::string& id, const std::string& value) {
    auto* el = getEl(id);
    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
        input->SetValue(Utilities::toBackwardSlashes(value).string());
    }
}
void UISetup::setupBrowseButtons() {
    //if (auto* btn = getEl("browse-folder-btn")) {
    //    btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
    //        auto folder = Utilities::browseFolder().string();
    //        if (!folder.empty()) setInput("load-dir-input", folder);
    //    }));
    //}

    if (auto* btn = getEl("browse-load-btn")) {
        btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            auto folder = Utilities::browseFolder().string();
            if (!folder.empty()) setInput("load-dir-input", folder);
        }));
    }

    //if (auto* btn = getEl("browse-tct-btn")) {
    //    btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
    //        auto file = Utilities::browseTCTFile().string();
    //        if (!file.empty()) setInput("load-dir-input", file);
    //    }));
    //}

    if (auto* btn = getEl("browse-btn")) {
        btn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            auto folder = Utilities::browseFolder().string();
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
                LOG_INFO("UISetup","Project created: " + (save_data.path / save_data.name).string());
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
                LOG_INFO("UISetup","Project loaded: " + save_data.name);
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

    getWindow().document->AddEventListener(Rml::EventId::Mousemove,
    new MouseEventHandler([](Rml::Event& event) {
        if (!drag.active) return;

        double mx, my;
        glfwGetCursorPos(Backend::GetWindow(), &mx, &my);

        if (!drag.ghostVisible) {
            // Check if moved enough to be a drag
            double dx = mx - drag.startX;
            double dy = my - drag.startY;
            if (dx*dx + dy*dy < 36.0) return; // 6px threshold

            // Now create the ghost
            drag.ghostVisible = true;
            auto* doc = getWindow().document;
            if (!doc) return;

            Rml::ElementPtr g = doc->CreateElement("div");
            g->SetId("drag-ghost");
            g->SetAttribute("style",
                "position:fixed;pointer-events:none;z-index:9999;"
                "width:80px;height:80px;opacity:0.75;border-radius:6px;"
                "border:2px solid #3498db;overflow:hidden;"
                "left:" + std::to_string(int(mx - 40)) + "px;"
                "top:"  + std::to_string(int(my - 40)) + "px;");

            Rml::ElementPtr gImg = doc->CreateElement("img");
            gImg->SetAttribute("src", drag.imagePath);
            gImg->SetAttribute("style", "width:100%;height:100%;object-fit:cover;");
            g->AppendChild(std::move(gImg));

            drag.ghost = g.get();
            if (auto* body = getEl("body"))
                body->AppendChild(std::move(g));
            return;
        }

        // Ghost already visible — just reposition it
        drag.ghost->SetProperty("left", std::to_string(int(mx - 40)) + "px");
        drag.ghost->SetProperty("top",  std::to_string(int(my - 40)) + "px");
    }));

    // Cancel drag on mouseup anywhere (projectors will stop propagation on success)
    getWindow().document->AddEventListener(Rml::EventId::Mouseup,
        new ButtonHandler([] {
            if (drag.active) endDrag();
        }));

    ResourceHandler::initResources();

    if (!UIManager::loadScenesData()) {
        std::cerr << "Failed to load scenesData" << std::endl;
    }
    if (auto *projectname = getWindow().document->GetElementById("project-name")) {
        LOG_INFO("UISetup","Setting project name: " + save_data.name);
        projectname->SetInnerRML(save_data.name);
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
    getEl("settings")->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
        getEl("settings-overlay")->SetAttribute("style", "display: block");
    }));
    getEl("settings-close")->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
        getEl("settings-overlay")->SetAttribute("style", "display: none");
    }));

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

    if (auto* el = getEl("file-dropdown-save-as")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            std::filesystem::path path1= Utilities::browseFolder();
            std::filesystem::path path2 = save_data.path / save_data.name;
            try {
                fs::copy(path2, path1 / save_data.name, fs::copy_options::recursive);
                Utilities::showPopup("[" + path2.string() + "] gespeichert unter [" + (path2 / save_data.path/ save_data.name).string() + "]");
            } catch (const std::filesystem::filesystem_error& e) {
                Utilities::showError(e.what());
                Utilities::showError("[" + path2.string() + "] konnte nicht unter [" + (path2 / save_data.path/ save_data.name).string() + "] gespeichert werden!");
            }
        }));
    }

    if (auto* el = getEl("file-dropdown-exportproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            try {
                std::filesystem::path destination = Utilities::browseFolder();

                std::string fullPath = save_data.path.string() + "\\" + save_data.name;

                if (std::filesystem::exists(fullPath + ".tct"))
                    std::filesystem::remove(fullPath + ".tct");

                std::string command =
                    "powershell Compress-Archive -Path \"" + fullPath +
                    "\\*\" -DestinationPath \"" + fullPath + ".zip\" -Force";

                int result = std::system(command.c_str());
                if (result != 0) {
                    std::cerr << "Failed to zip folder. Exit code: " << result << '\n';
                    return;
                }

                std::filesystem::rename(fullPath + ".zip", fullPath + ".tct");

                std::filesystem::copy(fullPath + ".tct", destination);

                LOG_INFO("UISetup","File " + save_data.name
                          + ".tct exported successfully to: "
                          + save_data.path.string());

            } catch (const std::filesystem::filesystem_error& e) {
                 LOG_ERR("main",(std::string)"Filesystem error: " + e.what());
            }
        }));
    }

    if (auto* el = getEl("file-dropdown-importproject")) {
        el->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {

            try {
                std::filesystem::path sourceFile = Utilities::browseTCTFile();

                if (!std::filesystem::exists(sourceFile))
                    return;

                char buffer[MAX_PATH];
                GetModuleFileNameA(NULL, buffer, MAX_PATH);
                std::filesystem::path exeDir = std::filesystem::path(buffer).parent_path();

                std::filesystem::path savesDir = exeDir.parent_path() / "saves" / "folderSaves";

                if (!std::filesystem::exists(savesDir))
                    std::filesystem::create_directories(savesDir);

                std::filesystem::path copiedTct = savesDir / sourceFile.filename();

                std::filesystem::copy_file(
                    sourceFile,
                    copiedTct,
                    std::filesystem::copy_options::overwrite_existing
                );

                std::filesystem::path zipPath = copiedTct;
                zipPath.replace_extension(".zip");

                std::filesystem::rename(copiedTct, zipPath);

                std::filesystem::path extractPath = savesDir / zipPath.stem();

                std::string command =
                    "powershell -NoProfile -Command \"Expand-Archive -Path '" +
                    zipPath.string() +
                    "' -DestinationPath '" +
                    extractPath.string() +
                    "' -Force\"";

                int result = std::system(command.c_str());
                if (result != 0) {
                     LOG_INFO("UISetup","Failed to unzip folder. Exit code: " + result);
                    return;
                }

                std::filesystem::remove(zipPath);

                 LOG_INFO("UISetup","Project imported successfully");

                UIManager::loadProject(extractPath);

            } catch (const std::filesystem::filesystem_error& e) {
                 LOG_ERR("UISetup",(std::string)"Filesystem error: " + e.what());
            }
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
                                             scene_manager.scenes.emplace_back(SceneData{
                                                 "Szene " + std::to_string(scene_manager.scenes.size() + 1),
                                                 {}
                                             });
                                             UIManager::saveProject(current_project_path);

                                             UIManager::refreshScenes();
                                         })
        );
    }
    auto sceneButtonsArrowUp = getEl("scene-buttons-arrow-up");
        sceneButtonsArrowUp->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
                if (active_scene_index != -1 && active_scene_index != 0) {
                    SceneData temp = scene_manager.scenes[active_scene_index - 1];
                    scene_manager.scenes[active_scene_index - 1] = scene_manager.scenes[active_scene_index];
                    scene_manager.scenes[active_scene_index] = temp;
                    active_scene_index--;
                    UIManager::saveProject(current_project_path);

                    UIManager::refreshScenes();
                }
            })
        );
    auto sceneButtonsArrowDown = getEl("scene-buttons-arrow-down");
        sceneButtonsArrowDown->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
                if (active_scene_index != -1 && active_scene_index != scene_manager.scenes.size() - 1) {
                    SceneData temp = scene_manager.scenes[active_scene_index + 1];
                    scene_manager.scenes[active_scene_index + 1] = scene_manager.scenes[active_scene_index];
                    scene_manager.scenes[active_scene_index] = temp;
                    active_scene_index++;

                    UIManager::saveProject(current_project_path);
                    UIManager::refreshScenes();
                }
            })
        );
    auto sceneButtonsDeleteAll = getEl("scene-buttons-delete-all");
        sceneButtonsDeleteAll->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
            //TODO: confirm
            scene_manager.scenes.clear();

            active_scene_index = 0;
            UIManager::saveProject(current_project_path);
            UIManager::refreshScenes();
        }));
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
            if (proj) {
                proj->requestDie();
                if (proj->th.joinable())
                    proj->th.join();
                delete proj;
            }
        }
        projectors.clear();

        // --- Regenerate split sources (this now populates splitInfo) ---
        UIManager::regenerateSplitSources();

        auto& currentScene = scene_manager.scenes[active_scene_index];

        // --- Create new projectors with split info ---
        for (int i = 0; i < save_data.projector_amount; i++) {
            // Check if we have a valid splitInfo for this projector
            if (i < currentScene.split_info.size()) {
                const auto& splitInfo = currentScene.split_info[i];

                // Get the actual resource path using ResourceHandler
                std::string resourcePath;
                try {
                    if (splitInfo.resourceId != -1) {
                        Resource& resource = ResourceHandler::getResource(splitInfo.resourceId);
                        resourcePath = resource.path.string();

                        LOG_INFO("UISetup",
                            "Launching projector " + std::to_string(i) +
                            " with resource: " + resourcePath +
                            " (ID: " + std::to_string(splitInfo.resourceId) + ")" +
                            " Split: " + std::string(splitInfo.isSplit ? "Yes" : "No")
                        );

                        if (splitInfo.isSplit) {
                            //std::cout << " Range: " << splitInfo.start << " - " << splitInfo.end;
                        }

                        // Create projector with split info
                        auto p = new Projector(i + 1, resourcePath, splitInfo);
                        projectors.push_back(p);
                    } else {
                        //std::cout << "No resource ID for projector " << i << std::endl;
                        // Create empty projector or skip
                        projectors.push_back(nullptr);
                    }
                } catch (const std::exception& e) {
                    LOG_ERR("UISetup", "Error launching projector " + std::to_string(i) + ": " + e.what());
                    projectors.push_back(nullptr);
                }
            } else {
                LOG_ERR("UISetup", "No split info for projector " + std::to_string(i));
                projectors.push_back(nullptr);
            }
        }

        LOG_INFO("UISetup", "[Info] Projectors started, count: " + projectors.size());
    }));

    auto *stop = getEl("stop-projection");
    stop->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {

        // --- Stop all projectors safely ---
        for (auto& proj : projectors) {
            if (proj) {
                proj->requestDie();
            }
        }

        // Wait for all threads to finish
        for (auto& proj : projectors) {
            if (proj) {
                if (proj->th.joinable())
                    proj->th.join();
                delete proj;
            }
        }

        projectors.clear();
        LOG_INFO("UISetup","Projectors stopped");
    }));
}