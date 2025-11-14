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
        std::cout << "Skipping creation of recent.path" << std::endl;

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
        std::cout << "Skipping creation of recent.path" << std::endl;

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
        std::cout << "Skipping creation of recent.path" << std::endl;

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

void UIManager::selectScene(const int index) {
    std::cout << "Selecting scene: " << index << " (previously: " << activeSceneIndex << ")" << std::endl;
    activeSceneIndex = index;
    refreshScenes();
}



