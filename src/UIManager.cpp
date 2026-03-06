#include "UIManager.h"

#include "ResourceHandler.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// Saving Functions

// Saves to SaveData.path
bool UIManager::saveProject() {
    std::filesystem::path savePath = save_data.path / save_data.name;
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
        json j = save_data;
        if (!saveJsonToFile(saveDataPath, j)) {
            std::cerr << "[Error] Failed to save saveData.json to file" << std::endl;
            return false;
        }
    }

    // Save scenesData.json
    {
        std::filesystem::path scenePath = savePath / "scenesData.json";
        if (scene_manager.scenes.empty()) {
            // Create default scene
            std::cout << "No scenes found, creating default scene" << std::endl;
            scene_manager.scenes.push_back({save_data.name, {}});
        }

        json j = scene_manager;
        if (!saveJsonToFile(scenePath, j)) {
            std::cerr << "[Error] Failed to save scenesData.json to file" << std::endl;
            return false;
        }
    }
    if (!ResourceHandler::saveResources())
        return false;
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
        json j = save_data;
        if (!saveJsonToFile(saveDataPath, j)) {
            std::cerr << "[Error] Failed to save saveData.json to file" << std::endl;
            return false;
        }
    }

    // Save scenesData.json
    {
        std::filesystem::path scenePath = savePath / "scenesData.json";
        if (scene_manager.scenes.empty()) {
            // Create default scene
            std::cout << "No scenes found, creating default scene" << std::endl;
            scene_manager.scenes.push_back({save_data.name, {}});
        }

        json j = scene_manager;
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

    current_project_path = path;

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
        save_data = j.get<SaveData>();
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
        scene_manager = j.get<SceneManager>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: scenesData.json");
        return false;
    }

    for (auto& scene : scene_manager.scenes) {
        scene.connections.resize(save_data.projector_amount);
    }

    // Create recent path file
    if (create_recent_path)
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

    current_project_path = loadPath;

    // Load saveData.json
    std::filesystem::path saveDataPath = current_project_path / "saveData.json";
    std::ifstream file(saveDataPath);
    if (!file.is_open()) {
        Utilities::showError("Datei: saveData.json konnte nicht geöffnet werden");
        return false;
    }

    try {
        json j;
        file >> j;
        save_data = j.get<SaveData>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: saveData.json");
        return false;
    }

    // Load scenesData.json
    std::filesystem::path scenesPath = current_project_path / "scenesData.json";
    std::ifstream scenesFile(scenesPath);
    if (!scenesFile.is_open()) {
        Utilities::showError("Datei: scenesData.json konnte nicht geöffnet werden");
        return false;
    }

    try {
        json j;
        scenesFile >> j;
        scene_manager = j.get<SceneManager>();
    } catch (const std::exception &e) {
        Utilities::showError("Fehler beim Lesen von: scenesData.json");
        return false;
    }

    for (auto& scene : scene_manager.scenes) {
        scene.connections.resize(save_data.projector_amount);
    }

    ResourceHandler::loadResources();

    refreshProjectors();
    refreshScenes();
    refreshResourcePanel();

    if (auto el = getEl("project-name"))
        el->SetInnerRML(save_data.name);

    // Create recent path file
    if (create_recent_path)
        createRecentPathFile(current_project_path);
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

            save_data.name = value;
        }
    }
    int projc;

    // Validate and get projector count
    if (auto *el = getEl("projector-count-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            int count = std::stoi(input->GetValue());
            save_data.projector_amount = count;
            projc = count;

        }
    }

    // Validate and get project description
    if (auto *el = getEl("project-desc-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            if (!validateInputField(value, "Beschreibung")) return false;
            save_data.description = value;
        }
    }

    // Get project directory
    std::filesystem::path basePath;
    if (auto *el = getEl("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            save_data.path = value;
            basePath = value;
        }
    }

    save_data.version = VERSION;

    // Create full project path
    std::filesystem::path fullProjectPath = basePath / save_data.name;

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
    active_scene_index = 0;
    scene_manager.scenes.push_back({save_data.name, {}});

    for (int i = 0; i < projc; ++i) {
        scene_manager.scenes[active_scene_index].connections.push_back(0);
    }

    current_project_path = fullProjectPath;

    // Save project data
    if (!saveProject(fullProjectPath)) {
        return false;
    }

    // Create recent path file
    if (create_recent_path)
        createRecentPathFile(fullProjectPath);
    else
        std::cout << "Skipping creation of recent path" << std::endl;

    return true;
}

std::string* UIManager::validateInputField(std::string &value, const std::string &fieldName) {
    if (Utilities::validateString(value)) {
        return &value;
    }
    Utilities::showError("Ungültige Zeichen im Feld: " + fieldName);
    return nullptr;
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

        for (int i = 0; i < scene_manager.scenes.size(); ++i) {
            Rml::ElementPtr child = getWindow().document->CreateElement("div");
            child->SetClass("scene-item", true);
            child->SetId("scene-item-" + std::to_string(i)); // Add ID for targeting

            // Add active class if this is the selected scene
            if (i == active_scene_index) {
                child->SetClass("active", true);
            }

            child->SetAttribute("data-scene-index", std::to_string(i));
            child->SetInnerRML(scene_manager.scenes[i].name);

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

static void clearSelection(Rml::Element* container) {
    for (int i = 0; i < container->GetNumChildren(); ++i) {
        container->GetChild(i)->RemoveAttribute("data-selected");
    }
}

bool deleteProject(const std::string& path) {
    namespace fs = std::filesystem;

    std::error_code ec;
    fs::remove_all(path, ec);

    if (ec) {
        std::cerr << "Failed to delete project: " << ec.message() << '\n';
        return false;
    }

    UIManager::populateFolders("../saves/folderSaves/");
    return true;
}

void UIManager::populateFolders(const std::string& path) {
    Rml::Element* container = getEl("tab-folder-list");
    container->SetInnerRML("");

    for (auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_directory())
            continue;

        std::string folderName = entry.path().filename().string();
        std::string fullPath   = entry.path().string();

        // Folder row
        Rml::ElementPtr folderDiv = getWindow().document->CreateElement("div");
        folderDiv->SetAttribute("class", "sample-project");

        // Folder name
        Rml::ElementPtr nameSpan = getWindow().document->CreateElement("span");
        nameSpan->SetInnerRML(folderName);
        folderDiv->AppendChild(std::move(nameSpan));

        Rml::ElementPtr trashBtn = getWindow().document->CreateElement("div");
        trashBtn->SetAttribute("class", "trash-btn");

        Rml::ElementPtr trashIcon = getWindow().document->CreateElement("img");
        trashIcon->SetAttribute("src", "trash.svg");
        trashIcon->SetAttribute("class", "trash-icon");
        trashBtn->AppendChild(std::move(trashIcon));

        trashBtn->AddEventListener(
            Rml::EventId::Click,
            new ButtonHandler([fullPath]() {
                //TODO: confirm
                deleteProject(fullPath);
            }, true)
        );

        folderDiv->AddEventListener(Rml::EventId::Click, new ButtonHandler(
            [fullPath] {
                if (auto *inputEl = getWindow().document->GetElementById("load-dir-input")) {
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                        input->SetValue(Utilities::toBackwardSlashes(fullPath).string());
                    }
                }
            }
        ));
        folderDiv->AddEventListener(Rml::EventId::Dblclick, new ButtonHandler(
            [fullPath] {
                if (auto *inputEl = getWindow().document->GetElementById("load-dir-input")) {
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                        input->SetValue(Utilities::toBackwardSlashes(fullPath).string());
                    }
                }
                if (loadProject()) {
                    getWindow().document->Hide();
                    std::cout << "Project loaded: " << save_data.name << std::endl;
                    if ((getWindow().document = getWindow().context->LoadDocument("assets/interface.rml"))) {
                        getWindow().document->Show();
                    }
                    setInterfaceEventListeners();
                }
            }
        ));

        folderDiv->AppendChild(std::move(trashBtn));
        container->AppendChild(std::move(folderDiv));
    }
}

bool UIManager::loadScenesData() {
    std::cout << "[Info] Loading scenes data" << std::endl;

    std::ifstream file(current_project_path / "scenesData.json");
    if (!file.is_open()) {
        std::cerr << "[Info] Couldn't open scenesData.json" << std::endl;
        return false;
    }

    scene_manager.scenes.clear();

    try {
        json j;
        file >> j;

        if (!j.contains("scenes") || !j["scenes"].is_array()) {
            std::cerr << "[Helper] Invalid scenesData.json format" << std::endl;
            return false;
        }

        for (const auto& sceneJson : j["scenes"]) {
            SceneData scene;

            scene.name =
                sceneJson.value("sceneName", "Unnamed Scene");

            if (sceneJson.contains("sources")) {
                for (const auto& src : sceneJson["sources"]) {
                    scene.sources.push_back(src.get<std::string>());
                }
            }

            if (sceneJson.contains("splitSources")) {
                for (const auto& src : sceneJson["splitSources"]) {
                    scene.split_sources.push_back(src.get<std::string>());
                }
            }

            if (sceneJson.contains("connections")) {
                for (const auto& c : sceneJson["connections"]) {
                    scene.connections.push_back(c.get<int>());
                }
            }

            scene_manager.scenes.push_back(std::move(scene));
        }

    } catch (const std::exception& e) {
        std::cerr << " Error parsing JSON: " << e.what() << std::endl;
        return false;
    }

    std::cout << "Loaded "
              << scene_manager.scenes.size()
              << " scenes" << std::endl;

    //for (int i = 0; i < saveData.projectorCount - 1; i++) {
    //    std::cout << sceneManager.scenes[0].connection[i] << " connection" << std::endl;
    //}

    refreshScenes();
    return true;
}
void UIManager::switchToStartup() {
    if ((getWindow().document = getWindow().context->LoadDocument("assets/startup.rml"))) {
        scene_manager.scenes.clear();
        setStartupEventListeners();
        getWindow().document->Show();
    }
}

void UIManager::refreshResourcePanel()
{
    auto* resourceList = getEl("resource-list");

    while (resourceList->GetNumChildren() > 0)
        resourceList->RemoveChild(resourceList->GetChild(0));

    const auto& resources = ResourceHandler::getResources();

    int i = 0;
    for (const Resource& r : resources)
    {
        if (!r.is_video && r.name.size() >= 10 &&
        r.name.compare(r.name.size() - 10, 10, "_thumbnail") == 0)
            continue;

        std::filesystem::path previewImage;

        if (r.is_video)
        {
            // find thumbnail resource
            const Resource* thumb = nullptr;
            for (const Resource& candidate : ResourceHandler::getResources())
            {
                if (candidate.id == r.thumbnail_id)
                {
                    thumb = &candidate;
                    break;
                }
            }

            // ignore missing thumbnail
            if (!thumb || !std::filesystem::exists(thumb->path))
                continue;

            previewImage = thumb->path;
        }
        else
        {
            if (!std::filesystem::exists(r.path))
                continue;

            previewImage = r.path;
        }

        Rml::ElementPtr item = getWindow().document->CreateElement("div");
        item->SetAttribute("class", "resource-item");
        item->SetAttribute("id", "resource-item-" + std::to_string(r.id));

        Rml::ElementPtr img = getWindow().document->CreateElement("img");
        img->SetAttribute("src", previewImage.string());
        img->SetAttribute("alt", "Resource");

        Rml::ElementPtr p = getWindow().document->CreateElement("p");
        p->AppendChild(getWindow().document->CreateTextNode(r.name));

        item->AppendChild(std::move(img));
        item->AppendChild(std::move(p));

        if (i == active_resource_index)
            item->SetClass("active", true);

        const int resourceId = r.id;
        item->AddEventListener(
            Rml::EventId::Mouseup,
            new ResourceItemHandler(getWindow().document, resourceId)
        );

        resourceList->AppendChild(std::move(item));
        i++;
    }
}

void UIManager::refreshProjectors() {
    auto* projectorGrid = getEl("projectorGrid");
    if (!projectorGrid) return;

    // remove all children
    while (projectorGrid->GetNumChildren() > 0)
        projectorGrid->RemoveChild(projectorGrid->GetChild(0));
    int n = std::max(1, save_data.projector_amount);

    // container metrics
    float gridWidth = static_cast<float>(projectorGrid->GetClientWidth());
    gridWidth -= 60.f;
    const int totalConnectors = std::max(0, n - 1);

    // MUST match RCSS exactly
    const float gap = 10.0f;            // .projector-grid gap
    const float connectorWidth = 16.0f; // .connect-btn width
    const float gridPadding = 0.0f;     // set to 16.0f ONLY if grid has 8px left + right padding

    // total non-projector width
    const float reservedWidth =
        gridPadding +
        (totalConnectors * connectorWidth) +
        ((n - 1) * gap);

    // usable space for projectors
    float usableWidth = gridWidth - reservedWidth;
    if (usableWidth <= 0.0f)
        usableWidth = 1.0f;

    // ideal width per projector
    float projectorWidth = usableWidth / static_cast<float>(n);

    // hard upper bound: cannot exceed container slice
    const float maxProjectorWidth = gridWidth / static_cast<float>(n);
    if (projectorWidth > maxProjectorWidth)
        projectorWidth = maxProjectorWidth;

    // optional minimum size (only if you WANT overflow/scrolling)
    const float minProjectorWidth = 0.0f;
    if (projectorWidth < minProjectorWidth)
        projectorWidth = minProjectorWidth;

    // derive height AFTER width is final
    float projectorHeight = projectorWidth * 9.0f / 16.0f;

   // std::cout << "\n=== refreshProjectors DEBUG ===\n";
   // std::cout << std::fixed << std::setprecision(2);
//
   // std::cout << "gridWidth        = " << gridWidth << " px\n";
   // std::cout << "n                = " << n << "\n";
   // std::cout << "totalConnectors  = " << totalConnectors << "\n";
   // std::cout << "gap              = " << gap << " px\n";
   // std::cout << "connectorWidth   = " << connectorWidth << " px\n";
   // std::cout << "gridPadding      = " << gridPadding << " px\n";
//
   // std::cout << "reservedWidth    = " << reservedWidth << " px\n";
   // std::cout << "  connectors     = " << (totalConnectors * connectorWidth) << " px\n";
   // std::cout << "  gaps           = " << ((n - 1) * gap) << " px\n";
//
   // std::cout << "usableWidth      = " << usableWidth << " px\n";
   // std::cout << "projectorWidth   = " << projectorWidth << " px\n";
   // std::cout << "projectorHeight  = " << projectorHeight << " px\n";



    // container styling (flex row, horizontal scroll when needed)
    projectorGrid->SetProperty("display", "flex");
    projectorGrid->SetProperty("flex-direction", "row");
    projectorGrid->SetProperty("flex-wrap", "nowrap");
    projectorGrid->SetProperty("gap", std::to_string((int)gap) + "px");
    projectorGrid->SetProperty("align-items", "center");
    projectorGrid->SetProperty("overflow-x", "auto");
    projectorGrid->SetProperty("overflow-x", "visible");

    for (auto& scene : scene_manager.scenes) {
        scene.connections.resize(n);
    }
    scene_manager.scenes[active_scene_index].split_sources.resize(n);

    for (int i = 0; i < n; ++i) {
        // projector wrapper
        Rml::ElementPtr projector = getWindow().document->CreateElement("div");
        projector->SetClass("projector", true);
        projector->SetId("projector-" + std::to_string(i));

        // prevent flex from stretching the box
        projector->SetProperty("flex", "0 0 auto");

        // set explicit pixel size
        int pw = static_cast<int>(std::round(projectorWidth));
        int ph = static_cast<int>(std::round(projectorHeight));
        projector->SetProperty("width", std::to_string(pw) + "px");
        projector->SetProperty("height", std::to_string(ph) + "px");
        projector->SetProperty("overflow", "hidden");

        // create an inner element that will hold the background image and preserve aspect ratio
        Rml::ElementPtr inner = getWindow().document->CreateElement("div");
        inner->SetClass("projector-inner", true);
        inner->SetProperty("width", "100%");
        inner->SetProperty("height", "100%");

        if (i < scene_manager.scenes[active_scene_index].sources.size())
        {
            auto& scene = scene_manager.scenes[active_scene_index];

            std::string& baseSrc = scene.sources[i];
            std::string& splitSrc =
                (i < scene.split_sources.size())
                ? scene.split_sources[i]
                : baseSrc;

            if (!baseSrc.empty() && !std::filesystem::exists(baseSrc))
            {
                baseSrc = "";
                if (i < scene.split_sources.size())
                    splitSrc = "";

                if (i < scene.connections.size())
                    scene.connections[i] = 0;

                if (i > 0 && i - 1 < scene.connections.size())
                    scene.connections[i - 1] = 0;
            }
            else if (!baseSrc.empty())
            {
                std::string src = !splitSrc.empty() ? splitSrc : baseSrc;

                std::filesystem::path p(src);
                if (p.extension() == ".mp4")
                    src = (p.parent_path() / "thumbs" /
                          (p.stem().string() + ".png")).string();

                if (std::filesystem::exists(src))
                {
                    inner->SetProperty("decorator", "image(\"" + src + "\")");
                }
                else
                {
                    splitSrc = "";
                }
            }
        }

        // attach the inner to the wrapper
        projector->AppendChild(std::move(inner));

        // click handler for selecting projector
        projector->AddEventListener(Rml::EventId::Mouseup, new ProjectorHandler(getWindow().document, i));

        // add wrapper to the grid
        projectorGrid->AppendChild(std::move(projector));

        // ---- CONNECT BUTTON (except after last projector) ----
        if (i < n - 1) {
            Rml::ElementPtr connectBtn = getWindow().document->CreateElement("div");
            connectBtn->SetClass("connect-btn", true);

            if (scene_manager.scenes[active_scene_index].connections[i] == 1)
                connectBtn->SetClass("connected", true);
            else
                connectBtn->SetClass("disconnected", true);

            // size and vertical centering
            connectBtn->SetProperty("width", std::to_string(static_cast<int>(connectorWidth)) + "px");
            // make connect button height proportional to projector (60% of projector height)
            int ch = std::max(10, static_cast<int>(std::round(projectorHeight * 0.6f)));
            connectBtn->SetProperty("height", std::to_string(ch) + "px");
            connectBtn->SetProperty("align-self", "center");

            connectBtn->AddEventListener(Rml::EventId::Click, new ConnectHandler(i));

            projectorGrid->AppendChild(std::move(connectBtn));
        }
    }
}


int generationid = 0;

void UIManager::regenerateSplitSources()
{
    auto& scene = scene_manager.scenes[active_scene_index];

    scene.sources.resize(save_data.projector_amount);
    scene.split_sources.clear();
    scene.split_sources.resize(save_data.projector_amount);
    scene.split_info.clear();
    scene.split_info.resize(save_data.projector_amount);

    std::filesystem::path dir =
        save_data.path / save_data.name / "resources" / "splits";

    if (!std::filesystem::exists(dir))
        std::filesystem::create_directories(dir);

    // Clear old split images for this scene
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().filename().string().find("scene_" + std::to_string(active_scene_index)) != std::string::npos) {
            std::filesystem::remove_all(entry.path());
        }
    }

    int i = 0;
    while (i < save_data.projector_amount)
    {
        int groupStart = i;
        int groupLength = 1;

        while (groupStart + groupLength - 1 < save_data.projector_amount - 1 &&
               scene.connections[groupStart + groupLength - 1])
        {
            groupLength++;
        }

        const std::string leftSource = scene.sources[groupStart];

        for (int k = 0; k < groupLength; ++k)
            scene.sources[groupStart + k] = leftSource;

        if (groupLength > 1 && !leftSource.empty())
        {
            for (int k = 0; k < groupLength; ++k)
            {
                float start = float(k) / float(groupLength);
                float end   = float(k + 1) / float(groupLength);

                // Generate split image for UI preview
                std::filesystem::path out =
                    dir /
                    ("scene_" + std::to_string(active_scene_index) +
                     "_proj_" + std::to_string(generationid) + "_" +
                     std::to_string(groupStart + k) + ".png");

                Utilities::cropImagePart(start, end, leftSource, out.string());

                SplitInfo splitInfo;
                try {
                    Resource resource;
                    for (auto& res : ResourceHandler::getResources()) {
                        if (res.id == ResourceHandler::getResourceIdByPath(leftSource)) {
                            resource = res;
                        }
                    }
                    for (auto& res : ResourceHandler::getResources()) {
                        if (res.thumbnail_id == ResourceHandler::getResourceIdByPath(leftSource)) {
                            resource = res;
                        }
                    }
                    splitInfo.resourceId = resource.id;
                    splitInfo.isSplit = true;
                    splitInfo.start = start;
                    splitInfo.end = end;

                    scene.split_info[groupStart + k] = splitInfo;
                    scene.split_sources[groupStart + k] = out.string();

                    std::cout << "Created split " << groupStart + k
                              << ": " << start << " - " << end
                              << " (Resource ID: " << splitInfo.resourceId << ")"
                              << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error creating split for projector " << groupStart + k
                              << ": " << e.what() << std::endl;
                }
            }
        }
        else if (!leftSource.empty())
        {
            SplitInfo splitInfo;
            try {
                Resource resource;
                for (auto& res : ResourceHandler::getResources()) {
                    if (res.id == ResourceHandler::getResourceIdByPath(leftSource)) {
                        resource = res;
                    }
                }
                for (auto& res : ResourceHandler::getResources()) {
                    if (res.thumbnail_id == ResourceHandler::getResourceIdByPath(leftSource)) {
                        resource = res;
                    }
                }

                splitInfo.resourceId = resource.id;
                splitInfo.isSplit = false;
                splitInfo.start = 0.0f;
                splitInfo.end = 1.0f;

                scene.split_info[groupStart] = splitInfo;
                scene.split_sources[groupStart] = leftSource;

                std::cout << "Full resource for projector " << groupStart
                          << " (Resource ID: " << splitInfo.resourceId << ")"
                          << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error getting resource ID for projector " << groupStart
                          << ": " << e.what() << std::endl;
            }
        }
        else
        {
            // No source assigned
            SplitInfo splitInfo;
            splitInfo.resourceId = -1;
            splitInfo.isSplit = false;
            splitInfo.start = 0.0f;
            splitInfo.end = 1.0f;

            scene.split_info[groupStart] = splitInfo;
            std::cout << "No resource for projector " << groupStart << std::endl;
        }

        i += groupLength;
    }

    ++generationid;
    refreshProjectors();
    saveProject();
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//Context menus

void selectScene(const int index) {
    std::cout << "Selecting scene: " << index << " (previously: " << active_scene_index << ")" << std::endl;
    active_scene_index = index;
    UIManager::refreshScenes();
    UIManager::refreshProjectors();
}
void showSceneRenameDialog(int sceneIndex) {
    if (sceneIndex < 0 || sceneIndex >= (int) scene_manager.scenes.size()) {
        std::cerr << "[Error] Invalid scene index for rename: " << sceneIndex << std::endl;
        return;
    }

    // Get the scene item element
    std::string sceneItemId = "scene-item-" + std::to_string(sceneIndex);
    if (auto *sceneItem = getEl(sceneItemId)) {
        // Store the current name for potential cancellation
        std::string currentName = scene_manager.scenes[sceneIndex].name;

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
                        scene_manager.scenes[sceneIndex].name = newName;

                        // Save the project
                        std::filesystem::path fullProjectPath =
                                std::filesystem::path(save_data.path) / save_data.name;
                        UIManager::saveProject(fullProjectPath);

                        //std::cout << "Scene renamed to: " << newName << std::endl;
                    } else {
                        // If invalid, restore original name
                        scene_manager.scenes[sceneIndex].name = currentName;
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
    if (sceneIndex >= 0 && sceneIndex < (int) scene_manager.scenes.size()) {
        scene_manager.scenes.erase(scene_manager.scenes.begin() + sceneIndex);

        // Update active scene index
        if (active_scene_index == sceneIndex) {
            active_scene_index = -1; // No scene selected
        } else if (active_scene_index > sceneIndex) {
            active_scene_index--; // Adjust index after deletion
        }

        // Save changes and refresh UI
        std::filesystem::path fullProjectPath = std::filesystem::path(save_data.path) / save_data.name;
        UIManager::saveProject(fullProjectPath);
        UIManager::refreshScenes();
    }
}

void duplicateScene(int sceneIndex) {
    std::cout << "Duplicate scene: " << sceneIndex << std::endl;
    if (sceneIndex >= 0 && sceneIndex < (int)scene_manager.scenes.size()) {
        SceneData newScene = scene_manager.scenes[sceneIndex];

        std::string baseName = newScene.name;
        std::regex copyRegex(R"( \(Copy(?: \d+)?\)$)");
        baseName = std::regex_replace(baseName, copyRegex, "");

        std::string newName = baseName + " (Copy)";
        int copyIndex = 2;

        bool nameExists;
        do {
            nameExists = false;
            for (const auto& scene : scene_manager.scenes) {
                if (scene.name == newName) {
                    newName = baseName + " (Copy " + std::to_string(copyIndex) + ")";
                    copyIndex++;
                    nameExists = true;
                    break;
                }
            }
        } while (nameExists);

        newScene.name = newName;
        scene_manager.scenes.insert(scene_manager.scenes.begin() + sceneIndex + 1, newScene);

        UIManager::saveProject(current_project_path);
        UIManager::refreshScenes();
    }
}
void showResourceRenameDialog(int id) {
    // Use the new ResourceHandler method
    std::optional<Resource> resourceOpt = ResourceHandler::getResource(id);

    if (!resourceOpt.has_value()) {
        std::cerr << "Error: Resource with ID " << id << " not found!" << std::endl;
        return;
    }

    Resource resource = resourceOpt.value();
    std::filesystem::path filePath = resource.path;  // From Resource struct
    std::string currentName = resource.name;         // From Resource struct

    std::string itemId = "resource-item-" + std::to_string(id);
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
        input->SetId("rename-input-" + std::to_string(id));
        input->SetClass("rename-input", true);

        Rml::ElementPtr okButton = getWindow().document->CreateElement("button");
        okButton->SetInnerRML("OK");
        okButton->SetClass("rename-ok-button", true);

        container->AppendChild(std::move(input));
        container->AppendChild(std::move(okButton));
        itemEl->AppendChild(std::move(container));

        // Focus input
        if (auto* inputEl = getWindow().document->GetElementById("rename-input-" + std::to_string(id))) {
            if (auto* inputField = dynamic_cast<Rml::ElementFormControl*>(inputEl)) {
                inputField->Focus();
            }
        }

        // Add listener safely via container
        if (auto* containerEl = itemEl->GetChild(itemEl->GetNumChildren() - 1)) {
            if (auto* okEl = containerEl->GetChild(1)) {
                okEl->AddEventListener(Rml::EventId::Click, new ButtonHandler([id, filePath, currentName] {
                    if (auto* inputEl = getWindow().document->GetElementById("rename-input-" + std::to_string(id))) {
                        if (auto* inputField = dynamic_cast<Rml::ElementFormControl*>(inputEl)) {
                            std::string newName = inputField->GetValue();
                            if (!newName.empty() && newName != currentName) {
                                // Rename using ResourceHandler method
                                ResourceHandler::getResource(id).name = newName;

                                if (true) {
                                    std::cout << "Successfully renamed resource " << id << " to: " << newName << std::endl;

                                    // Update the UI
                                    UIManager::refreshResourcePanel();

                                    // Optionally: Close the rename dialog
                                    std::string itemId = "resource-item-" + std::to_string(id);
                                    if (auto* itemEl = getEl(itemId)) {
                                        // Restore original display
                                        itemEl->SetClass("renaming", false);

                                        // Remove rename container
                                        if (itemEl->GetNumChildren() > 0) {
                                            auto* containerEl = itemEl->GetChild(itemEl->GetNumChildren() - 1);
                                            if (containerEl && containerEl->HasAttribute("rename-container")) {
                                                itemEl->RemoveChild(containerEl);
                                            }
                                        }

                                        // Update displayed name
                                        itemEl->SetInnerRML(newName);

                                        // Re-add the image if needed
                                        // (You might need to restore the image element here)
                                    }
                                } else {
                                    std::cerr << "Failed to rename resource " << id << std::endl;
                                    // Show error to user
                                }
                            }
                        }
                    }

                    UIManager::saveProject();
                    UIManager::refreshResourcePanel();
                }));
            }
        }
    }
}

void deleteResource(int id) {
    ResourceHandler::deleteResource(id);
    UIManager::refreshResourcePanel();
    UIManager::refreshProjectors();
}
void selectResource(int index){
    std::cout << "Selecting Resource: " << index << " (previously: " << active_resource_index << ")" << std::endl;
    active_resource_index = index;
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
        void ProcessEvent(Rml::Event&) override {
            if (auto* body = getEl("body"))
                body->RemoveChild(overlay);
        }
    };

    overlay_raw->AddEventListener(Rml::EventId::Mousedown, new CloseOverlayHandler(overlay_raw));
    overlay_raw->AddEventListener(Rml::EventId::Mouseup,   new CloseOverlayHandler(overlay_raw));

    Rml::Element* dialog_raw = overlay_raw->GetChild(0);

    struct StopPropagationHandler : Rml::EventListener {
        void ProcessEvent(Rml::Event& e) override { e.StopPropagation(); }
    };

    dialog_raw->AddEventListener(Rml::EventId::Mousedown, new StopPropagationHandler());
    dialog_raw->AddEventListener(Rml::EventId::Mouseup,   new StopPropagationHandler());

    // "None" row
    {
        Rml::ElementPtr row = doc->CreateElement("div");
        row->SetAttribute("style",
            "display:flex;align-items:center;gap:14px;padding:8px;background:#3f5870;"
            "border-radius:6px;cursor:pointer;width:100%;box-sizing:border-box;");

        Rml::ElementPtr img = doc->CreateElement("div");
        img->SetAttribute("style",
            "width:64px;height:64px;border-radius:4px;"
            "background:#2b3d4f;display:flex;align-items:center;justify-content:center;"
            "color:#aaa;font-size:12px;");
        img->SetInnerRML("None");

        Rml::ElementPtr label = doc->CreateElement("div");
        label->SetAttribute("style", "color:white;font-size:16px;");
        label->SetInnerRML("Kein Bild");

        row->AppendChild(std::move(img));
        row->AppendChild(std::move(label));

        struct ProjectorResourceSelectHandler_Clear : Rml::EventListener {
            int projectorIndex;
            Rml::Element* overlay;
            ProjectorResourceSelectHandler_Clear(int idx, Rml::Element* o)
                : projectorIndex(idx), overlay(o) {}
            void ProcessEvent(Rml::Event&) override {
                auto& scene = scene_manager.scenes[active_scene_index];
                if (projectorIndex >= scene.sources.size()) {
                    scene.sources.resize(projectorIndex + 1);
                }
                scene.sources[projectorIndex] = "";
                if (auto* body = getEl("body")) body->RemoveChild(overlay);
                UIManager::refreshProjectors();
                UIManager::saveProject();
            }
        };

        row.get()->AddEventListener(Rml::EventId::Mouseup,
            new ProjectorResourceSelectHandler_Clear(index, overlay_raw));

        dialog_raw->AppendChild(std::move(row));
    }

    // --- Populate from ResourceHandler ---
    std::vector<Resource> resources = ResourceHandler::getResources();

    for (const Resource& r : resources) {

        // skip thumbnail resources
        if (!r.is_video && r.name.size() >= 10 &&
            r.name.compare(r.name.size() - 10, 10, "_thumbnail") == 0)
            continue;

        std::filesystem::path previewImage;

        if (r.is_video) {
            // find thumbnail resource
            const Resource* thumb = nullptr;
            for (const Resource& candidate : resources) {
                if (candidate.id == r.thumbnail_id) {
                    thumb = &candidate;
                    break;
                }
            }

            if (!thumb || !std::filesystem::exists(thumb->path))
                continue;

            previewImage = thumb->path;
        } else {
            if (!std::filesystem::exists(r.path))
                continue;

            previewImage = r.path;
        }

        Rml::ElementPtr row = doc->CreateElement("div");
        row->SetAttribute("style",
            "display:flex;align-items:center;gap:14px;padding:8px;background:#3f5870;border-radius:6px;"
            "cursor:pointer;width:100%;box-sizing:border-box;");

        Rml::ElementPtr img = doc->CreateElement("img");
        img->SetAttribute("src", previewImage.string());
        img->SetAttribute("style", "width:64px;height:64px;object-fit:cover;border-radius:4px;");

        Rml::ElementPtr label = doc->CreateElement("div");
        label->SetAttribute("style", "color:white;font-size:16px;");
        label->SetInnerRML(r.name);

        row->AppendChild(std::move(img));
        row->AppendChild(std::move(label));

        struct ProjectorResourceSelectHandler : Rml::EventListener {
            int projectorIndex;
            std::string imagePath;
            Rml::Element* overlay;

            ProjectorResourceSelectHandler(int idx, std::string p, Rml::Element* o)
                : projectorIndex(idx), imagePath(std::move(p)), overlay(o) {}

            void ProcessEvent(Rml::Event&) override {
                auto& scene = scene_manager.scenes[active_scene_index];
                if (projectorIndex >= scene.sources.size())
                    scene.sources.resize(projectorIndex + 1);
                scene.sources[projectorIndex] = imagePath;
                if (auto* body = getEl("body")) body->RemoveChild(overlay);
                UIManager::refreshProjectors();
                UIManager::saveProject();
                UIManager::regenerateSplitSources();
            }
        };


        // click handler to assign resource to projector
        row.get()->AddEventListener(
            Rml::EventId::Mouseup,
            new ProjectorResourceSelectHandler(index, previewImage.string(), overlay_raw)
        );


        dialog_raw->AppendChild(std::move(row));
    }

    if (auto* body = getEl("body")) body->AppendChild(std::move(overlay));
}

void connectProjectors(int index) {
    if (index < 0 || index >= save_data.projector_amount - 1)
        return;

    scene_manager.scenes[active_scene_index].connections[index] = 1;

    // make sure splitSources has space
    scene_manager.scenes[active_scene_index].split_sources.resize(save_data.projector_amount);

    // regenerate splits for the scene
    UIManager::regenerateSplitSources();

    UIManager::refreshProjectors();
    UIManager::saveProject();
}

void disconnectProjectors(int index) {
    scene_manager.scenes[active_scene_index].connections[index] = 0;

    // regenerate splits for the scene
    UIManager::regenerateSplitSources();

    UIManager::refreshProjectors();
    UIManager::saveProject();
}
