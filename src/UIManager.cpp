#include "UIManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// Saving Functions

// Saves to SaveData.path
bool UIManager::saveProject() {
    std::filesystem::path savePath = saveData.path / saveData.projectName;
    if (!std::filesystem::exists(savePath)) {
        std::cout << "[Error] Failed to save project at: " << savePath << std::endl;
    }
    if (!verifyFolderStructure(savePath)) {
        //TODO: Warn user about fixing folder structure
        std::cout << "Folder structure off, fixing it" << std::endl;
        // For now just fix it
        fixFolderStructure(savePath);
    }

    // Save saveData.json
    {
        std::filesystem::path saveDataPath = savePath / "saveData.json";
        json j = saveData;
        if (!saveJsonToFile(saveDataPath, j)) {
            std::cerr << "[Error] Failed to save saveData.json to file" << std::endl;
            return false;
        }
    }

    // Save scenesData.json
    {
        std::filesystem::path scenePath = savePath / "scenesData.json";
        if (sceneManager.scenes.empty()) {
            // Create default scene
            std::cout << "No scenes found, creating default scene" << std::endl;
            sceneManager.scenes.push_back({saveData.projectName, {}});
        }

        json j = sceneManager;
        if (!saveJsonToFile(scenePath, j)) {
            std::cerr << "[Error] Failed to save scenesData.json to file" << std::endl;
            return false;
        }
    }
    return true;
}

// Saves to specified path
bool UIManager::saveProject(const std::filesystem::path &savePath) {
    if (!verifyFolderStructure(savePath)) {
        //TODO: Warn user about fixing folder structure
        std::cout << "Folder structure off, fixing it" << std::endl;
        // For now just fix it
        fixFolderStructure(savePath);
    }

    // Save saveData.json
    {
        std::filesystem::path saveDataPath = savePath / "saveData.json";
        json j = saveData;
        if (!saveJsonToFile(saveDataPath, j)) {
            std::cerr << "[Error] Failed to save saveData.json to file" << std::endl;
            return false;
        }
    }

    // Save scenesData.json
    {
        std::filesystem::path scenePath = savePath / "scenesData.json";
        if (sceneManager.scenes.empty()) {
            // Create default scene
            std::cout << "No scenes found, creating default scene" << std::endl;
            sceneManager.scenes.push_back({saveData.projectName, {}});
        }

        json j = sceneManager;
        if (!saveJsonToFile(scenePath, j)) {
            std::cerr << "[Error] Failed to save scenesData.json to file" << std::endl;
            return false;
        }
    }
    return true;
}

bool UIManager::verifyFolderStructure(const std::filesystem::path &savePath) {
    if (!std::filesystem::exists(savePath)) return false;
    if (!std::filesystem::is_directory(savePath)) return false;
    if (!std::filesystem::exists(savePath / "saveData.json")) return false;
    if (!std::filesystem::exists(savePath / "scenesData.json")) return false;
    if (!std::filesystem::exists(savePath / "resources")) return false;
    return true;
}
void UIManager::fixFolderStructure(const std::filesystem::path &savePath) {
    if (!std::filesystem::exists(savePath)) {
        std::filesystem::create_directories(savePath);
    }
    if (!std::filesystem::exists(savePath / "saveData.json")) {
        std::ofstream file(savePath / "saveData.json");
        file.close();
    }
    if (!std::filesystem::exists(savePath / "scenesData.json")) {
        std::ofstream file(savePath / "scenesData.json");
        file.close();
    }
    if (!std::filesystem::exists(savePath / "resources")) {
        std::filesystem::create_directories(savePath / "resources");
    }

}
bool UIManager::saveJsonToFile(const std::filesystem::path &savePath, const json &data) {
    std::ofstream file(savePath);
    if (!file.is_open()) {
        if (auto *el = getWindow().document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte json nicht Speichern!");
        }
        return false;
    }

    file << data.dump(4);
    file.close();

    if (!std::filesystem::exists(savePath)) {
        if (auto *el = getWindow().document->GetElementById("error-text")) {
            el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
        }
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//Loading Functions

// Loads from SaveData.path
bool UIManager::loadProject() {
    std::string path;
    if (auto *el = getEl("load-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            path = input->GetValue();
        }
    }

    if (path.empty()) {
        Utilities::showError("Bitte einen validen Pfad eingeben.");
        return false;
    }

    projectPath = path;

    // Load saveData.json
    std::filesystem::path saveDataPath = path + "/saveData.json";
    std::ifstream file(saveDataPath);
    if (!file.is_open()) {
        Utilities::showError("Datei: saveData.json konnte nicht geöffnet werden");
        return false;
    }

    try {
        json j;
        file >> j;
        saveData = j.get<SaveData>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: saveData.json");
        return false;
    }

    // Load scenesData.json
    std::filesystem::path scenesPath = path + "/scenesData.json";
    std::ifstream scenesFile(scenesPath);
    if (!scenesFile.is_open()) {
        Utilities::showError("Datei: scenesData.json konnte nicht geöffnet werden");
        return false;
    }

    try {
        json j;
        scenesFile >> j;
        sceneManager = j.get<SceneManager>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: scenesData.json");
        return false;
    }

    // Create recent path file
    if (createRecentPath)
        createRecentPathFile(path);
    else
        std::cout << "Skipping creation of recent path" << std::endl;

    return true;
}

// Loads from specified path
bool UIManager::loadProject(const std::filesystem::path& loadPath) {

    if (!std::filesystem::exists(loadPath)) {
        Utilities::showError("Pfad: " + loadPath.string() + " existiert nicht.");
        return false;
    }

    projectPath = loadPath;

    // Load saveData.json
    std::filesystem::path saveDataPath = projectPath / "saveData.json";
    std::ifstream file(saveDataPath);
    if (!file.is_open()) {
        Utilities::showError("Datei: saveData.json konnte nicht geöffnet werden");
        return false;
    }

    try {
        json j;
        file >> j;
        saveData = j.get<SaveData>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: saveData.json");
        return false;
    }

    // Load scenesData.json
    std::filesystem::path scenesPath = projectPath / "scenesData.json";
    std::ifstream scenesFile(scenesPath);
    if (!scenesFile.is_open()) {
        Utilities::showError("Datei: scenesData.json konnte nicht geöffnet werden");
        return false;
    }

    try {
        json j;
        scenesFile >> j;
        sceneManager = j.get<SceneManager>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: scenesData.json");
        return false;
    }

    // Create recent path file
    if (createRecentPath)
        createRecentPathFile(projectPath);
    else
        std::cout << "Skipping creation of recent path" << std::endl;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//Loading Functions

bool UIManager::createProject() {
    if (auto *el = getEl("project-name-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            if (!validateInputField(value, "Projektname"))
                return false;

            saveData.projectName = value;
        }
    }

    // Validate and get projector count
    if (auto *el = getEl("projector-count-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            if (auto count = validateProjectorCount(value).value()) {
                saveData.projectorCount = count;
            } else {
                Utilities::showError("Ungültige Eingabe für Projektoranzahl (1-9)");
                return false;
            }
        }
    }

    // Validate and get project description
    if (auto *el = getEl("project-desc-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            if (!validateInputField(value, "Beschreibung")) return false;
            saveData.description = value;
        }
    }

    // Get project directory
    std::filesystem::path basePath;
    if (auto *el = getEl("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            saveData.path = value;
            basePath = value;
        }
    }

    // Create full project path
    std::filesystem::path fullProjectPath = basePath / saveData.projectName;

    // Prevent overwriting existing project
    if (std::filesystem::exists(fullProjectPath)) {
        Utilities::showError("Projekt existiert bereits auf diesem Pfad");
        return false;
    }

    // Create project directory
    std::error_code ec;
    if (!std::filesystem::create_directories(fullProjectPath, ec) || ec) {
        Utilities::showError("Konnte Projektverzeichnis nicht erstellen");
        return false;
    }

    projectPath = fullProjectPath;

    // Save project data
    if (!saveProject(fullProjectPath)) {
        return false;
    }

    // Create recent path file
    if (createRecentPath)
        createRecentPathFile(fullProjectPath);
    else
        std::cout << "Skipping creation of recent path" << std::endl;

    return true;
}

std::optional<int> UIManager::validateProjectorCount(const std::string &value) {
    if (value.size() == 1 && value[0] >= '1' && value[0] <= '9') {
        return value[0] - '0';
    }
    return std::nullopt;
}
std::string* UIManager::validateInputField(std::string &value, const std::string &fieldName) {
    if (Utilities::validateString(value)) {
        return &value;
    } else {
        Utilities::showError("Ungültige Zeichen im Feld: " + fieldName);
        return nullptr;
    }

}
bool UIManager::createRecentPathFile(const std::filesystem::path &savePath) {

    std::filesystem::path saveDir = "../saves";
    std::filesystem::create_directories(saveDir);

    std::filesystem::path recentFile = saveDir / "recent.path";
    std::ofstream recent(recentFile, std::ios::out | std::ios::binary);
    if (!recent) {
        std::cerr << "Failed to create " << recentFile << "\n";
        return false;
    }

    recent << savePath.string();
    std::cout << "Created " << recentFile << " with path: " << savePath << "\n";
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//Other helpers

void UIManager::refreshScenes() {
    if (auto *sceneList = getEl("sceneList")) {
        for (int i = sceneList->GetNumChildren() - 1; i >= 0; --i) {
            Rml::Element *child = sceneList->GetChild(i);
            sceneList->RemoveChild(child);
        }

        for (int i = 0; i < sceneManager.scenes.size(); ++i) {
            Rml::ElementPtr child = getWindow().document->CreateElement("div");
            child->SetClass("scene-item", true);
            child->SetId("scene-item-" + std::to_string(i)); // Add ID for targeting

            // Add active class if this is the selected scene
            if (i == activeSceneIndex) {
                child->SetClass("active", true);
            }

            child->SetAttribute("data-scene-index", std::to_string(i));
            child->SetInnerRML(sceneManager.scenes[i].sceneName);

            // Add event listener
            child->AddEventListener(Rml::EventId::Mouseup, new SceneItemHandler(getWindow().document, i));

            sceneList->AppendChild(std::move(child));
        }
    }
}
void UIManager::setSelectedProject(const std::string &name) {
    if (auto *label = getEl("selected-project-label")) {
        label->SetInnerRML(name);
    }
}
void UIManager::populateFolders(const std::string &path) {
    Rml::Element *container = getEl("tab-folder-list");

    container->SetInnerRML("");

    for (auto &entry: std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            std::string fullPath = entry.path().string();

            Rml::ElementPtr folderDiv = getWindow().document->CreateElement("div");
            folderDiv->SetClassNames("sample-project");
            folderDiv->SetId("folder-" + folderName);
            folderDiv->SetInnerRML(folderName);

            folderDiv->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                            [fullPath] {
                                                if (auto *inputEl = getWindow().document->GetElementById("load-dir-input")) {
                                                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(
                                                        inputEl)) {
                                                        input->SetValue(Utilities::toBackwardSlashes(fullPath));
                                                    }
                                                }
                                            }
                                        ));

            container->AppendChild(std::move(folderDiv));
        }
    }
}
bool UIManager::loadScenesData() {
    std::cout << "[Info] Loading scenes data" << std::endl;

    std::ifstream file(projectPath / "scenesData.json");
    if (!file.is_open()) {
        std::cerr << "[Info] Couldn't open " << projectPath / "scensesData.json" << std::endl;
        return false;
    }

    sceneManager.scenes.clear();

    try {
        json j;
        file >> j;

        if (j.contains("scenes")) {
            for (const auto &sceneJson: j["scenes"]) {
                SceneData scene;
                scene.sceneName = sceneJson.value("sceneName", "Unnamed Scene");
                if (sceneJson.contains("sources")) {
                    for (const auto &src: sceneJson["sources"])
                        scene.sources.push_back(src.get<std::string>());
                }
                sceneManager.scenes.push_back(scene);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[Helper] Error parsing JSON: " << e.what() << std::endl;
        return false;
    }

    std::cout << "[Helper] Loaded " << sceneManager.scenes.size() << " scenes" << std::endl;
    refreshScenes();
    return true;
}
void UIManager::switchToStartup() {
    if ((getWindow().document = getWindow().context->LoadDocument("assets/startup.rml"))) {
        setStartupEventListeners();
        getWindow().document->Show();
    }
}
void UIManager::refreshResourcePanel() {
    auto* resourceList = getEl("resource-list");
    while (resourceList->GetNumChildren() > 0) {
        resourceList->RemoveChild(resourceList->GetChild(0));
    }
    auto directory = saveData.path / saveData.projectName / "resources";
    int i = 0;
    for (auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".png") continue;

        std::string filename = entry.path().filename().string();
        std::string name_no_ext = entry.path().stem().string();

        Rml::ElementPtr item = getWindow().document->CreateElement("div");
        item->SetAttribute("class", ("resource-item"));
        item->SetAttribute("id", ("resource-item-" + std::to_string(i)));

        Rml::ElementPtr img = getWindow().document->CreateElement("img");
        auto imagePath = directory / filename;
        img->SetAttribute("src", imagePath.string());
        img->SetAttribute("alt", "Resource");

        Rml::ElementPtr p = getWindow().document->CreateElement("p");
        p->AppendChild(getWindow().document->CreateTextNode(name_no_ext));

        item->AppendChild(std::move(img));
        item->AppendChild(std::move(p));

        if (i == activeResourceIndex) {
            item->SetClass("active", true);
        }

        item->AddEventListener(Rml::EventId::Mouseup, new ResourceItemHandler(getWindow().document, i));

        resourceList->AppendChild(std::move(item));
        i++;
    }
}

void UIManager::refreshProjectors() {
    if (auto *projectorGrid = getEl("projectorGrid")) {
        for (int i = projectorGrid->GetNumChildren() - 1; i >= 0; --i) {
            Rml::Element *child = projectorGrid->GetChild(i);
            projectorGrid->RemoveChild(child);
        }
        std::cout << "[Info] Projector count: " << saveData.projectorCount << std::endl;
        for (int i = 0; i < saveData.projectorCount; ++i) {
            Rml::ElementPtr child = getWindow().document->CreateElement("div");
            child->SetClass("projector", true);

            Rml::ElementPtr span = getWindow().document->CreateElement("span");
            span->SetInnerRML("Beamer " + std::to_string(i + 1));
            child->AppendChild(std::move(span));
            child->SetId("projector-" + std::to_string(i));

            child->SetAttribute("data-scene-index", std::to_string(i));

            // Add event listener
            child->AddEventListener(Rml::EventId::Mouseup, new ProjectorHandler(getWindow().document, i));

            projectorGrid->AppendChild(std::move(child));

        }
    }
    for (int i = 0; i < saveData.projectorCount; i++) {
        auto projector = getEl("projector-" + std::to_string(i));

        if (i < sceneManager.scenes[activeSceneIndex].sources.size() &&
            !sceneManager.scenes[activeSceneIndex].sources[i].empty()) {

            projector->SetInnerRML("");
            //auto img = getWindow().document->CreateElement("img");
            //img->SetAttribute("src", sceneManager.scenes[activeSceneIndex].sources[i]);
            projector->SetAttribute("style", "decorator: image(" + sceneManager.scenes[activeSceneIndex].sources[i] + ");");
            //projector->AppendChild(std::move(img));
            }
        else {
            Utilities::showError("Couldn't load resource for projector-" + std::to_string(i));
        }
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//Context menus

void selectScene(const int index) {
    std::cout << "Selecting scene: " << index << " (previously: " << activeSceneIndex << ")" << std::endl;
    activeSceneIndex = index;
    UIManager::refreshScenes();
    UIManager::refreshProjectors();
}
void showSceneRenameDialog(int sceneIndex) {
    if (sceneIndex < 0 || sceneIndex >= (int) sceneManager.scenes.size()) {
        std::cerr << "[Error] Invalid scene index for rename: " << sceneIndex << std::endl;
        return;
    }

    // Get the scene item element
    std::string sceneItemId = "scene-item-" + std::to_string(sceneIndex);
    if (auto *sceneItem = getEl(sceneItemId)) {
        // Store the current name for potential cancellation
        std::string currentName = sceneManager.scenes[sceneIndex].sceneName;

        // Replace the scene item content with a container
        sceneItem->SetInnerRML("");
        sceneItem->SetClass("renaming", true);

        // Create container for input and button
        Rml::ElementPtr container = getWindow().document->CreateElement("div");
        container->SetClass("rename-container", true);

        // Create input field
        Rml::ElementPtr input = getWindow().document->CreateElement("input");
        input->SetAttribute("type", "text");
        input->SetAttribute("value", currentName);
        input->SetId("rename-input-" + std::to_string(sceneIndex));
        input->SetClass("rename-input", true);

        // Stop propagation for input field clicks
        input->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            //std::cout << "Input field click - propagation stopped" << std::endl;
        }, true)); // The true parameter will stop propagation

        input->AddEventListener(Rml::EventId::Mouseup, new ButtonHandler([] {
            //std::cout << "Input field mouseup - propagation stopped" << std::endl;
        }, true));

        // Create OK button
        Rml::ElementPtr okButton = getWindow().document->CreateElement("button");
        okButton->SetInnerRML("OK");
        okButton->SetId("rename-ok-" + std::to_string(sceneIndex));
        okButton->SetClass("rename-ok-button", true);

        // Stop propagation for OK button clicks
        okButton->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            //std::cout << "OK button click - propagation stopped" << std::endl;
        }, true));

        okButton->AddEventListener(Rml::EventId::Mouseup, new ButtonHandler([] {
            //std::cout << "OK button mouseup - propagation stopped" << std::endl;
        }, true));

        // Add event listener for Enter key in input field
        input->AddEventListener(Rml::EventId::Keydown, new KeyEventHandler(
                                    [sceneIndex, currentName](Rml::Event &event) {
                                        if (event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_RETURN) {
                                            //std::cout << "Enter key pressed in rename input" << std::endl;
                                            event.StopPropagation(); // Stop the Enter key from propagating

                                            // Simulate OK button click when Enter is pressed
                                            if (auto *okButton = getWindow().document->GetElementById(
                                                "rename-ok-" + std::to_string(sceneIndex))) {
                                                okButton->Click();
                                            }
                                        }
                                    }));

        // Add event listener for OK button functionality
        okButton->AddEventListener(Rml::EventId::Click, new ButtonHandler([sceneIndex, currentName] {
            //std::cout << "OK button clicked for scene: " << sceneIndex << std::endl;
            if (auto *inputEl = getWindow().document->GetElementById("rename-input-" + std::to_string(sceneIndex))) {
                if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                    std::string newName = input->GetValue();

                    // Validate the new name
                    if (!newName.empty() && UIManager::validateInputField(newName, "Szenenname")) {
                        // Update the scene name
                        sceneManager.scenes[sceneIndex].sceneName = newName;

                        // Save the project
                        std::filesystem::path fullProjectPath =
                                std::filesystem::path(saveData.path) / saveData.projectName;
                        UIManager::saveProject(fullProjectPath);

                        //std::cout << "Scene renamed to: " << newName << std::endl;
                    } else {
                        // If invalid, restore original name
                        sceneManager.scenes[sceneIndex].sceneName = currentName;
                        std::cout << "[Error] Rename cancelled or invalid name" << std::endl;
                    }

                    // Always refresh to show the updated name
                    UIManager::refreshScenes();
                }
            }
        }));

        // Also stop propagation on the container itself
        container->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            std::cout << "Container click - propagation stopped" << std::endl;
        }, true));

        container->AddEventListener(Rml::EventId::Mouseup, new ButtonHandler([] {
            std::cout << "Container mouseup - propagation stopped" << std::endl;
        }, true));

        // Append input and button to container, then container to scene item
        container->AppendChild(std::move(input));
        container->AppendChild(std::move(okButton));
        sceneItem->AppendChild(std::move(container));

        // Focus the input field
        if (auto *inputEl = getWindow().document->GetElementById("rename-input-" + std::to_string(sceneIndex))) {
            if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                input->Focus();
            }
        }
    }
}
void deleteScene(int sceneIndex) {
    std::cout << "Delete scene: " << sceneIndex << std::endl;
    if (sceneIndex >= 0 && sceneIndex < (int) sceneManager.scenes.size()) {
        sceneManager.scenes.erase(sceneManager.scenes.begin() + sceneIndex);

        // Update active scene index
        if (activeSceneIndex == sceneIndex) {
            activeSceneIndex = -1; // No scene selected
        } else if (activeSceneIndex > sceneIndex) {
            activeSceneIndex--; // Adjust index after deletion
        }

        // Save changes and refresh UI
        std::filesystem::path fullProjectPath = std::filesystem::path(saveData.path) / saveData.projectName;
        UIManager::saveProject(fullProjectPath);
        UIManager::refreshScenes();
    }
}

void duplicateScene(int sceneIndex) {
    std::cout << "Duplicate scene: " << sceneIndex << std::endl;
    if (sceneIndex >= 0 && sceneIndex < (int)sceneManager.scenes.size()) {
        SceneData newScene = sceneManager.scenes[sceneIndex];

        std::string baseName = newScene.sceneName;
        std::regex copyRegex(R"( \(Copy(?: \d+)?\)$)");
        baseName = std::regex_replace(baseName, copyRegex, "");

        std::string newName = baseName + " (Copy)";
        int copyIndex = 2;

        bool nameExists;
        do {
            nameExists = false;
            for (const auto& scene : sceneManager.scenes) {
                if (scene.sceneName == newName) {
                    newName = baseName + " (Copy " + std::to_string(copyIndex) + ")";
                    copyIndex++;
                    nameExists = true;
                    break;
                }
            }
        } while (nameExists);

        newScene.sceneName = newName;
        sceneManager.scenes.insert(sceneManager.scenes.begin() + sceneIndex + 1, newScene);

        UIManager::saveProject(projectPath);
        UIManager::refreshScenes();
    }
}
void showResourceRenameDialog(int index) {
    auto directory = saveData.path / saveData.projectName / "resources";
    std::vector<std::filesystem::path> files;

    for (auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".png") continue;
        files.push_back(entry.path());
    }

    if (index < 0 || index >= (int)files.size()) return;

    std::filesystem::path filePath = files[index];
    std::string currentName = filePath.stem().string();

    std::string itemId = "resource-item-" + std::to_string(index);
    if (auto* itemEl = getEl(itemId)) {
        itemEl->SetClass("renaming", true);

        // Detach existing image so we can keep it
        Rml::ElementPtr existingImg;
        for (int i = 0; i < itemEl->GetNumChildren(); ++i) {
            if (itemEl->GetChild(i)->GetTagName() == "img") {
                existingImg = itemEl->RemoveChild(itemEl->GetChild(i));
                break;
            }
        }

        itemEl->SetInnerRML(""); // clear old text / name
        if (existingImg) {
            itemEl->AppendChild(std::move(existingImg));
        }

        Rml::ElementPtr container = getWindow().document->CreateElement("div");
        container->SetClass("rename-container", true);

        Rml::ElementPtr input = getWindow().document->CreateElement("input");
        input->SetAttribute("type", "text");
        input->SetAttribute("value", currentName);
        input->SetId("rename-input-" + std::to_string(index));
        input->SetClass("rename-input", true);

        Rml::ElementPtr okButton = getWindow().document->CreateElement("button");
        okButton->SetInnerRML("OK");
        okButton->SetClass("rename-ok-button", true);

        container->AppendChild(std::move(input));
        container->AppendChild(std::move(okButton));
        itemEl->AppendChild(std::move(container));

        // Focus input
        if (auto* inputEl = getWindow().document->GetElementById("rename-input-" + std::to_string(index))) {
            if (auto* inputField = dynamic_cast<Rml::ElementFormControl*>(inputEl)) {
                inputField->Focus();
            }
        }

        // Add listener safely via container
        if (auto* containerEl = itemEl->GetChild(itemEl->GetNumChildren() - 1)) {
            if (auto* okEl = containerEl->GetChild(1)) {
                okEl->AddEventListener(Rml::EventId::Click, new ButtonHandler([index, filePath, currentName] {
                    if (auto* inputEl = getWindow().document->GetElementById("rename-input-" + std::to_string(index))) {
                        if (auto* inputField = dynamic_cast<Rml::ElementFormControl*>(inputEl)) {
                            std::string newName = inputField->GetValue();
                            if (!newName.empty()) {
                                std::filesystem::path newThumbnail = filePath.parent_path() / (newName + filePath.extension().string());
                                std::filesystem::rename(filePath, newThumbnail);

                                auto mainResource = saveData.path / saveData.projectName / "resources" / filePath.filename();
                                if (std::filesystem::exists(mainResource)) {
                                    std::filesystem::path newMain = mainResource.parent_path() / (newName + mainResource.extension().string());
                                    std::filesystem::rename(mainResource, newMain);
                                }

                                UIManager::refreshResourcePanel();
                            }
                        }
                    }
                }));
            }
        }
    }
}



void deleteResource(int index) {
    auto directory = saveData.path / saveData.projectName / "resources";
    int i = 0;
    std::filesystem::path fileToDelete;

    for (auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".png") continue;

        if (i == index) {
            fileToDelete = entry.path();
            break;
        }
        i++;
    }

    if (!fileToDelete.empty()) {
        try {
            std::filesystem::remove(fileToDelete);

            // Optionally also remove the main resource image if it exists somewhere else
            auto mainImage = saveData.path / saveData.projectName / "resources" / fileToDelete.filename();
            if (std::filesystem::exists(mainImage)) {
                std::filesystem::remove(mainImage);
            }

            UIManager::refreshResourcePanel(); // Update the UI
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error deleting resource: " << e.what() << std::endl;
        }
    }
}
void selectResource(int resourceIndex){
    std::cout << "Selecting Resource: " << resourceIndex << " (previously: " << activeResourceIndex << ")" << std::endl;
    activeResourceIndex = resourceIndex;
    UIManager::refreshResourcePanel();
}

void showProjectorResourceSelection(int index)
{
    auto* doc = getWindow().document;
    if (!doc) return;

    if (auto* existing = getEl("projector-resource-overlay")) {
        if (auto* body = getEl("body")) body->RemoveChild(existing);
    }

    Rml::ElementPtr overlay = doc->CreateElement("div");
    overlay->SetId("projector-resource-overlay");
    overlay->SetAttribute("style",
        "position:fixed;left:0;top:0;width:100%;height:100%;display:flex;align-items:center;"
        "justify-content:center;background:rgba(0,0,0,0.5);z-index:2000;");


    Rml::ElementPtr dialog = doc->CreateElement("div");
    dialog->SetId("projector-resource-dialog");
    dialog->SetAttribute("style",
    "width:80%;max-width:600px;max-height:80%;"
    "overflow-y:auto;overflow-x:hidden;"
    "background:#34495e;border-radius:8px;"
    "padding:12px;box-sizing:border-box;"
    "display:flex;flex-direction:column;gap:8px;");

    overlay->AppendChild(std::move(dialog));

    Rml::Element* overlay_raw = overlay.get();

    struct CloseOverlayHandler : Rml::EventListener {
        Rml::Element* overlay;
        CloseOverlayHandler(Rml::Element* o) : overlay(o) {}

        void ProcessEvent(Rml::Event& e) override {
            if (auto* body = getEl("body"))
                body->RemoveChild(overlay);
        }
    };

    //overlay->AddEventListener(Rml::EventId::Click, new CloseOverlayHandler(overlay_raw));

    Rml::Element* dialog_raw = overlay_raw->GetChild(0);
    struct StopPropagationHandler : public Rml::EventListener {
        void ProcessEvent(Rml::Event& e) override {
            e.StopPropagation();
        }
    };
    // Close overlay on ANY mouse interaction (left or right)
    overlay_raw->AddEventListener(Rml::EventId::Mousedown, new CloseOverlayHandler(overlay_raw));
    overlay_raw->AddEventListener(Rml::EventId::Mouseup,   new CloseOverlayHandler(overlay_raw));

    // Prevent dialog from receiving close events
    dialog_raw->AddEventListener(Rml::EventId::Mousedown, new StopPropagationHandler());
    dialog_raw->AddEventListener(Rml::EventId::Mouseup,   new StopPropagationHandler());



    auto directory = saveData.path / saveData.projectName / "resources";
    for (auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".png") continue;

        std::string filename = entry.path().filename().string();
        std::string pathstr = (directory / filename).string();

        Rml::ElementPtr row = doc->CreateElement("div");
        row->SetAttribute("style",
            "display:flex;align-items:center;gap:14px;padding:8px;background:#3f5870;border-radius:6px;"
            "cursor:pointer;");

        Rml::ElementPtr img = doc->CreateElement("img");
        img->SetAttribute("src", pathstr);
        img->SetAttribute("style", "width:64px;height:64px;object-fit:cover;border-radius:4px;");

        Rml::ElementPtr label = doc->CreateElement("div");
        label->SetAttribute("style", "color:white;font-size:16px;");
        label->SetInnerRML(filename);

        row->AppendChild(std::move(img));
        row->AppendChild(std::move(label));

        row->SetAttribute("style","display:flex;align-items:center;gap:14px;padding:8px;"
            "background:#3f5870;border-radius:6px;cursor:pointer;"
            "width:100%;box-sizing:border-box;");


        Rml::Element* row_raw = row.get();

        struct ProjectorResourceSelectHandler : Rml::EventListener {
            int projectorIndex;
            std::string imagePath;
            Rml::Element* overlay;
            ProjectorResourceSelectHandler(int idx, const std::string& p, Rml::Element* o)
                : projectorIndex(idx), imagePath(p), overlay(o) {}
            void ProcessEvent(Rml::Event&) override {
                auto& scene = sceneManager.scenes[activeSceneIndex];
                if (projectorIndex >= scene.sources.size()) {
                    scene.sources.resize(projectorIndex + 1);
                }
                scene.sources[projectorIndex] = imagePath;
                std::cout << "Set resource[" << projectorIndex << "] to " << imagePath << std::endl;
                if (auto* body = getEl("body")) body->RemoveChild(overlay);
                UIManager::refreshProjectors();
                UIManager::saveProject();
            }
        };

        row_raw->AddEventListener(Rml::EventId::Mouseup,
            new ProjectorResourceSelectHandler(index, pathstr, overlay_raw));

        dialog_raw->AppendChild(std::move(row));
    }

    if (auto* body = getEl("body")) body->AppendChild(std::move(overlay));
}


