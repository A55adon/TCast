// ProjectManager.cpp
#include "ProjectManager.h"

SaveData g_saveData;
SceneManager g_sceneManager;
std::filesystem::path g_projectFolderPath;
std::filesystem::path g_projectDataPath;
std::filesystem::path g_scenesDataPath;
bool g_createRecentPath = true;
Window g_window = Window(1920, 1080);

bool ValidateBeamerCount(const std::string& value, int& outCount) {
    static const std::set<std::string> validWords = {
        "eins", "zwei", "drei", "vier", "fuenf", "fünf", "sechs", "sieben", "acht", "neun"
    };

    if (value.size() == 1 && value[0] >= '1' && value[0] <= '9') {
        outCount = value[0] - '0';
        return true;
    }

    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto it = validWords.find(lower);
    if (it != validWords.end()) {
        if (lower == "eins") outCount = 1;
        else if (lower == "zwei") outCount = 2;
        else if (lower == "drei") outCount = 3;
        else if (lower == "vier") outCount = 4;
        else if (lower == "fünf" || lower == "fuenf") outCount = 5;
        else if (lower == "sechs") outCount = 6;
        else if (lower == "sieben") outCount = 7;
        else if (lower == "acht") outCount = 8;
        else if (lower == "neun") outCount = 9;
        return true;
    }

    return false;
}

bool SaveScenesData() {
    if (g_scenesDataPath.empty()) return false;

    if (g_sceneManager.scenes.empty()) {
        SceneData defaultScene;
        defaultScene.sceneName = g_saveData.projectName;
        defaultScene.sources.emplace_back("test");
        g_sceneManager.scenes.push_back(defaultScene);
    }

    json j = g_sceneManager;
    std::ofstream file(g_scenesDataPath);
    if (!file.is_open()) {
        if (auto* el = g_window.document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte Datei nicht erstellen – ungültiger Pfad oder Rechteproblem");
        }
        return false;
    }
    file << j.dump(4);
    file.close();

    if (!std::filesystem::exists(g_scenesDataPath)) {
        if (auto* el = g_window.document->GetElementById("error-text")) {
            el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
        }
        return false;
    }
    return true;
}

void LoadScenesData(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    nlohmann::json data;
    file >> data;

    g_sceneManager.scenes.clear();

    if (data.contains("scenes")) {
        for (const auto& sceneJson : data["scenes"]) {
            SceneData scene;
            scene.sceneName = sceneJson.value("sceneName", "Unnamed Scene");
            if (sceneJson.contains("sources")) {
                for (const auto& src : sceneJson["sources"])
                    scene.sources.push_back(src.get<std::string>());
            }
            g_sceneManager.scenes.push_back(scene);
        }
    }
}

void UpdateGlobalPaths() {
    g_projectFolderPath = ToBackwardSlashes(g_saveData.path + "/" + g_saveData.projectName);
    g_projectDataPath = g_projectFolderPath / "projectData.json";
    g_scenesDataPath = g_projectFolderPath / "scenesData.json";
}

bool SaveNewProject() {
    std::string errorId = "error-text";

    if (auto* el = g_window.document->GetElementById("project-name-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto* element_error = g_window.document->GetElementById(errorId)) {
                    element_error->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                }
                return false;
            }
            g_saveData.projectName = value;
        }
    }
    if (auto* el = g_window.document->GetElementById("projector-count-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            int count = 0;
            if (!ValidateBeamerCount(value, count)) {
                if (auto* err = g_window.document->GetElementById(errorId)) {
                    err->SetInnerRML("Nur Zahlen 1-9 erlaubt!");
                }
                return false;
            }
            g_saveData.projectorCount = count;
        }
    }
    if (auto* el = g_window.document->GetElementById("project-desc-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto* element_error = g_window.document->GetElementById(errorId)) {
                    element_error->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                }
                return false;
            }
            g_saveData.description = value;
        }
    }

    if (auto* el = g_window.document->GetElementById("project-dir-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            g_saveData.path = input->GetValue();
        }
    }

    UpdateGlobalPaths();

    std::error_code ec;
    if (std::filesystem::exists(g_projectFolderPath)) {
        if (!std::filesystem::is_directory(g_projectFolderPath)) {
            if (auto* el = g_window.document->GetElementById(errorId)) {
                el->SetInnerRML("Pfad existiert, ist aber keine Ordnerstruktur!");
            }
            return false;
        }
    } else {
        if (!std::filesystem::create_directories(g_projectFolderPath, ec) || ec) {
            if (auto* el = g_window.document->GetElementById(errorId)) {
                el->SetInnerRML(("Konnte Ordner nicht erstellen: " + ec.message()).c_str());
            }
            return false;
        }
    }

    if (std::filesystem::exists(g_projectDataPath)) {
        if (auto* el = g_window.document->GetElementById(errorId)) {
            el->SetInnerRML("Projekt existiert bereits – bitte anderen Namen wählen!");
        }
        return false;
    }

    json j = g_saveData;
    std::ofstream file(g_projectDataPath);
    if (!file.is_open()) {
        if (auto* el = g_window.document->GetElementById(errorId)) {
            el->SetInnerRML("Konnte Datei nicht erstellen – ungültiger Pfad oder Rechteproblem");
        }
        return false;
    }
    file << j.dump(4);
    file.close();

    if (!SaveScenesData()) {
        return false;
    }

    return true;
}

bool SaveProject() {
    if (g_projectDataPath.empty()) {
        return SaveProjectAs();
    }

    json j = g_saveData;
    std::ofstream file(g_projectDataPath);
    if (!file.is_open()) {
        return false;
    }
    file << j.dump(4);
    file.close();

    return SaveScenesData();
}

bool SaveProjectAs() {
    std::string savePath = BrowseSaveFile(L"json", L"JSON Files (*.json)");
    if (savePath.empty()) return false;

    g_projectDataPath = savePath;
    g_projectFolderPath = g_projectDataPath.parent_path();
    g_scenesDataPath = g_projectFolderPath / "scenesData.json";
    g_saveData.path = g_projectFolderPath.parent_path().string();

    return SaveProject();
}

bool LoadProject() {
    std::string path;
    std::string errorId = "load-error-text";

    auto* errEl = g_window.document->GetElementById(errorId);
    if (errEl) errEl->SetInnerRML("");

    if (auto* el = g_window.document->GetElementById("load-dir-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            path = input->GetValue();
        }
    }

    if (std::filesystem::path(path).extension() == ".tct") {
        // Handle import if tct
        return ImportProject();
    }

    g_projectFolderPath = path;
    g_projectDataPath = g_projectFolderPath / "projectData.json";
    g_scenesDataPath = g_projectFolderPath / "scenesData.json";
    g_saveData.path = g_projectFolderPath.parent_path().string();

    if (g_projectDataPath.empty()) {
        if (errEl) errEl->SetInnerRML("Bitte einen Pfad eingeben.");
        return false;
    }

    std::ifstream file(g_projectDataPath);
    if (!file.is_open()) {
        if (errEl) errEl->SetInnerRML("Datei konnte nicht geöffnet werden: " + g_projectDataPath.string());
        return false;
    }

    try {
        json j;
        file >> j;
        g_saveData = j.get<SaveData>();
    } catch (const std::exception& e) {
        if (errEl) errEl->SetInnerRML(std::string("[1] Fehler beim Lesen der JSON-Datei: ") + e.what());
        return false;
    }

    std::ifstream fileScenes(g_scenesDataPath);
    if (!fileScenes.is_open()) {
        if (errEl) errEl->SetInnerRML("Datei konnte nicht geöffnet werden: " + g_scenesDataPath.string());
        return false;
    }

    try {
        json j;
        fileScenes >> j;
        g_sceneManager = j.get<SceneManager>();
    } catch (const std::exception& e) {
        if (errEl) errEl->SetInnerRML(std::string("[2] Fehler beim Lesen der JSON-Datei: ") + e.what());
        return false;
    }

    return true;
}

bool ExportProject() {
    std::string exportPath = BrowseSaveFile(L"tct", L"TCT Files (*.tct)");
    if (exportPath.empty()) return false;

    try {
        std::string zipPath = exportPath + ".zip";
        std::string command = "powershell Compress-Archive -Path \"" + g_projectFolderPath.string() +
                              "\\*\" -DestinationPath \"" + zipPath + "\" -Force";
        int result = std::system(command.c_str());

        if (result != 0) {
            std::cerr << "Failed to zip folder. Exit code: " << result << '\n';
            return false;
        }

        std::filesystem::rename(zipPath, exportPath);

        std::cout << "Exported to: " << exportPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
    }
}

bool ImportProject() {
    std::string tctPath = BrowseTCTFile();
    if (tctPath.empty()) return false;

    std::string importFolder = BrowseFolder();
    if (importFolder.empty()) return false;

    try {
        std::string zipPath = tctPath + ".zip";
        std::filesystem::rename(tctPath, zipPath);

        std::string command = "powershell Expand-Archive -Path \"" + zipPath + "\" -DestinationPath \"" + importFolder + "\" -Force";
        int result = std::system(command.c_str());

        std::filesystem::remove(zipPath);

        if (result != 0) {
            std::cerr << "Failed to unzip. Exit code: " << result << '\n';
            return false;
        }

        // Assume the unzipped folder is the project folder, load it
        // For simplicity, assume it's directly unzipped to importFolder/projectName
        // You may need to adjust if unzip creates a subfolder
        g_projectFolderPath = importFolder; // or detect the folder
        UpdateGlobalPaths();
        return LoadProject();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
    }
}