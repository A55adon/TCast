#pragma once
#include <shobjidl.h>
#include <string>
#include <nlohmann/json.hpp>
#include <regex>
#include <shlobj.h>
#include <set>
#include <algorithm>
#include <windows.h>
#include "Window.h"
#include "Shell.h"
#include "ButtonListener.h"
#include <fstream>
#include <iostream>
#include "Utils.h"


inline SaveData saveData;
inline SceneManager sceneManager;
inline auto window = Window(1920, 1080);

inline bool createRecentPath = true;

inline std::filesystem::path projectPath;
//inline std::filesystem::path filePath;

void switchToStartup();

inline void setSelectedProject(Rml::ElementDocument *doc, const std::string &name) {
    if (auto *label = doc->GetElementById("selected-project-label")) {
        label->SetInnerRML(name);
    }
}

inline void refreshScenes() {
    if (auto *sceneList = window.document->GetElementById("sceneList")) {
        std::cout << "[Info] Scenescount: " << sceneManager.scenes.size() << std::endl;

        for (int i = sceneList->GetNumChildren() - 1; i >= 0; i--) {
            // remove all scenes from the list first
            Rml::Element *child = sceneList->GetChild(i);
            sceneList->RemoveChild(child);
        }

        for (int i = 0; i < sceneManager.scenes.size(); ++i) {
            // recreate every scene entry
            Rml::ElementPtr child = window.document->CreateElement("div");
            child->SetClass("scene-item", true);
            child->SetInnerRML("Szene " + std::to_string(i + 1));
            sceneList->AppendChild(std::move(child));
        }
    }
}

// looks for saveData.json, scenesData.json and a resources folder
inline bool verifyFolderStructure(std::filesystem::path projectPath) {
    if (!std::filesystem::exists(projectPath)) return false;
    if (!std::filesystem::is_directory(projectPath)) return false;
    if (!std::filesystem::exists(projectPath / "saveData.json")) return false;
    if (!std::filesystem::exists(projectPath / "scenesData.json")) return false;
    if (!std::filesystem::exists(projectPath / "resources")) return false;
    return true;
}

// creates missing folders in folderstructure
inline void fixFolderStructure(std::filesystem::path projectPath) {
    if (!std::filesystem::exists(projectPath)) std::filesystem::create_directories(projectPath);
    if (!std::filesystem::exists(projectPath / "saveData.json")) {
        std::ofstream file(projectPath / "saveData.json");
        file.close();
    }
    if (!std::filesystem::exists(projectPath / "scenesData.json")) {
        std::ofstream file(projectPath / "scenesData.json");
        file.close();
    }
    if (!std::filesystem::exists(projectPath / "resources")) {
        std::filesystem::create_directories(projectPath / "resources");
    }
}

inline bool saveProject(std::filesystem::path projectPath) {
    if (!verifyFolderStructure(projectPath)) {
        fixFolderStructure(projectPath);
    } {
        std::filesystem::path saveDataPath = projectPath / "saveData.json";
        json j = saveData;
        std::ofstream file(saveDataPath);
        if (!file.is_open()) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML("Konnte nicht Speichern – ungültiger Pfad oder Rechteproblem");
            }
            return false;
        }

        file << j.dump(4);
        file.close();

        if (!std::filesystem::exists(saveDataPath) || !std::filesystem::exists(saveDataPath)) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
            }
            return false;
        }
    } { // create a own scope to clean up the streams properly

        std::filesystem::path scenePath = projectPath / "scenesData.json";
        if (sceneManager.scenes.empty()) {
            // create default scene
            sceneManager.scenes.push_back({saveData.projectName, {}});
        }
        json j = sceneManager;
        std::ofstream file(scenePath);
        if (!file.is_open()) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML("Konnte nicht Speichern – ungültiger Pfad oder Rechteproblem");
            }
            return false;
        }

        file << j.dump(4);
        file.close();

        if (!std::filesystem::exists(scenePath) || !std::filesystem::exists(scenePath)) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
            }
            return false;
        }
    }
    return true;
}

inline bool loadScenesData(std::filesystem::path projectPath) {
    std::ifstream file(projectPath / "scenesData.json");
    if (!file.is_open()) return false;

    sceneManager.scenes.clear();

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

    refreshScenes();
    return true;
}

inline bool validateBeamerCount(const std::string &value, int &outCount) {
    static const std::set<std::string> validWords = {
        "eins", "zwei", "drei", "vier", "fuenf", "fünf", "sechs", "sieben", "acht", "neun"
    };

    //numbers as letters
    if (value.size() == 1 && value[0] >= '1' && value[0] <= '9') {
        outCount = value[0] - '0';
        return true;
    }

    // big/small letters
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

inline void PopulateFolders(Rml::ElementDocument *doc, const std::string &path) {
    namespace fs = std::filesystem;
    Rml::Element *container = doc->GetElementById("tab-folder-list");
    if (!container) return;
    container->SetInnerRML("");

    for (auto &entry: fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            std::string fullPath = entry.path().string();

            Rml::ElementPtr folderDiv = doc->CreateElement("div");
            folderDiv->SetClassNames("sample-project");
            folderDiv->SetId("folder-" + folderName);
            folderDiv->SetInnerRML(folderName);

            folderDiv->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                            [doc, fullPath] {
                                                if (auto *inputEl = doc->GetElementById("load-dir-input"))
                                                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl))
                                                        input->SetValue(ToBackwardSlashes(fullPath));
                                            }
                                        ));

            container->AppendChild(std::move(folderDiv));
        }
    }
}
inline bool saveNewProject() {
    std::filesystem::path path;

    if (auto* el = window.document->GetElementById("project-name-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto* err = window.document->GetElementById("error-text"))
                    err->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                return false;
            }
            saveData.projectName = value;
        }
    }

    if (auto* el = window.document->GetElementById("projector-count-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            int count = 0;
            if (!validateBeamerCount(value, count)) {
                if (auto* err = window.document->GetElementById("error-text"))
                    err->SetInnerRML("Nur Zahlen 1-9 erlaubt!");
                return false;
            }
            saveData.projectorCount = count;
        }
    }

    if (auto* el = window.document->GetElementById("project-desc-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto* err = window.document->GetElementById("error-text"))
                    err->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                return false;
            }
            saveData.description = value;
        }
    }

    if (auto* el = window.document->GetElementById("project-dir-input")) {
        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            std::string value = input->GetValue();
            saveData.path = value;
            path = value;
        }
    }

    std::error_code ec;

    // full project folder path = base dir + project name
    path /= saveData.projectName;

    // prevent overwriting
    if (std::filesystem::exists(path)) {
        if (auto* el = window.document->GetElementById("error-text"))
            el->SetInnerRML("Projekt existiert bereits – bitte anderen Namen wählen!");
        return false;
    }

    // create project directory
    if (!std::filesystem::create_directories(path, ec) || ec) {
        if (auto* el = window.document->GetElementById("error-text"))
            el->SetInnerRML(("Konnte Ordner nicht erstellen: " + ec.message()).c_str());
        return false;
    }

    // create JSONs
    json j = saveData;
    std::ofstream file(path / "saveData.json");
    if (!file.is_open()) {
        if (auto* el = window.document->GetElementById("error-text"))
            el->SetInnerRML("Konnte Datei nicht erstellen – ungültiger Pfad oder Rechteproblem");
        return false;
    }
    file << j.dump(4);
    file.close();

    // now call saveProject() to generate scenesData.json
    if (!saveProject(path))
        return false;

    if (!std::filesystem::exists("../saves"))
        std::filesystem::create_directories("../saves");

    if (createRecentPath) {
        std::ofstream recent("../saves/recent.path", std::ios::out | std::ios::binary);
        if (recent) {
            recent << path.string();
            std::cout << "created recent.path with path " << path << std::endl;
        } else {
            std::cerr << "Failed to create recent.path" << std::endl;
        }
    }

    return true;
}


inline bool loadProject() {
    std::string path;
    std::string scenesPath;


    auto *errEl = window.document->GetElementById("load-error-text");
    if (errEl)
        errEl->SetInnerRML(""); // clear old errors

    if (auto *el = window.document->GetElementById("load-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            path = input->GetValue();
        }
    }
    scenesPath = ToBackwardSlashes(path + "/" + saveData.projectName + "/" + "scenesData.json");
    path = path + "/" + "projectData" + ".json";

    std::cout << path << std::endl;

    if (path.empty()) {
        if (errEl) errEl->SetInnerRML("Bitte einen Pfad eingeben.");
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        if (errEl) errEl->SetInnerRML("Datei konnte nicht geöffnet werden: " + path);
        return false;
    }

    try {
        json j;
        file >> j;
        saveData = j.get<SaveData>();
    } catch (const std::exception &e) {
        if (errEl) errEl->SetInnerRML(std::string("[1] Fehler beim Lesen der JSON-Datei: ") + e.what());
        return false;
    }

    std::ifstream fileScenes(scenesPath);
    if (!fileScenes.is_open()) {
        if (errEl) errEl->SetInnerRML("Datei konnte nicht geöffnet werden: " + path);
        return false;
    }

    try {
        json j;
        fileScenes >> j;
        sceneManager = j.get<SceneManager>();
    } catch (const std::exception &e) {
        if (errEl) errEl->SetInnerRML(std::string("[2] Fehler beim Lesen der JSON-Datei: ") + e.what());
        return false;
    }

    if (!std::filesystem::exists("../saves")) {
        std::filesystem::create_directories("../saves"); // create directory if missing
    }
    if (createRecentPath) {
        if (!std::filesystem::exists("../saves/recent.path")) {
            std::ofstream filestream("../saves/recent.path", std::ios::out | std::ios::binary);
            if (filestream) {
                filestream << path;
                filestream.close();
                std::cout << "created recent.path with path " << path << std::endl;
            } else {
                std::cerr << "Failed to create recent.path" << std::endl;
            }
        }
    }

    return true;
}

// for start load/save page
inline void setInterfaceEventListeners() {
    loadScenesData("");

#pragma region filedropdown
    if (auto *dropdownNewproject = window.document->GetElementById("file-dropdown-newproject")) {
        dropdownNewproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdownNewproject] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new")->SetClass("active", true);
            window.document->GetElementById("tab-load")->SetClass("active", false);
            window.document->GetElementById("tab-load")->SetAttribute("style", "display:none");
        }));
    }

    if (auto *dropdownLoadproject = window.document->GetElementById("file-dropdown-loadproject")) {
        dropdownLoadproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdownLoadproject] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-load")->SetClass("active", true);
            window.document->GetElementById("tab-new")->SetClass("active", false);
            window.document->GetElementById("tab-new")->SetAttribute("style", "display:none");
        }));
    }

    if (auto *filedropdownexportproject = window.document->GetElementById("file-dropdown-exportproject")) {
        filedropdownexportproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([filedropdownexportproject] {
            try {
                std::string fullPath = saveData.path + "\\" + saveData.projectName;

                if (std::filesystem::exists(fullPath + ".tct")) {
                    std::filesystem::remove_all(fullPath + ".tct");
                }

                std::string command = "powershell Compress-Archive -Path \"" + fullPath +
                                      "\\*\" -DestinationPath \"" + fullPath + ".zip\" -Force";
                int result = std::system(command.c_str());

                if (result == 0)
                    std::cout << "Folder zipped successfully.\n";
                else
                    std::cerr << "Failed to zip folder. Exit code: " << result << '\n';

                std::filesystem::rename(fullPath + ".zip", fullPath + ".tct");

                std::cout << "File " + saveData.projectName + ".tct exported successfully to: " + saveData.path <<
                        std::endl;
            } catch (const std::filesystem::filesystem_error &e) {
                std::cerr << "Filesystem error: " << e.what() << '\n';
            }
            //TODO: feedback
        }));
    }

    if (auto *filedropdownimportproject = window.document->GetElementById("file-dropdown-importproject")) {
        filedropdownimportproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([filedropdownimportproject] {
            //TODO: import projects

            //TODO: feedback
        }));
    }

    if (auto *dropdowncloseproject = window.document->GetElementById("file-dropdown-closeproject")) {
        dropdowncloseproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdowncloseproject] {
            switchToStartup();
        }));
    }

    if (auto *dropdowncloseprogramm = window.document->GetElementById("file-dropdown-closeprogramm")) {
        dropdowncloseprogramm->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdowncloseprogramm] {
            exit(EXIT_SUCCESS);
        }));
    }

#pragma endregion
#pragma region settingsdropdown
#pragma endregion
#pragma region viewdropdown
#pragma endregion
#pragma region helpdropdown
#pragma endregion


    if (auto *projectorgrid = window.document->GetElementById("projectorGrid")) {
        std::cout << "[Info] Projectorcount: " << saveData.projectorCount << std::endl;
        for (int i = 0; i < saveData.projectorCount; ++i) {
            Rml::ElementPtr child = window.document->CreateElement("div");
            child->SetClass("projector", true);

            Rml::ElementPtr span = window.document->CreateElement("span");
            span->SetInnerRML("Beamer " + std::to_string(i + 1));
            child->AppendChild(std::move(span));


            projectorgrid->AppendChild(std::move(child));
        }
    }

    if (auto *addSceneButton = window.document->GetElementById("add-scene-btn")) {
        addSceneButton->AddEventListener(Rml::EventId::Click,
                                         new ButtonHandler([&]() {
                                             sceneManager.scenes.emplace_back(SceneData{
                                                 "Szene " + std::to_string(sceneManager.scenes.size() + 1),
                                                 {}
                                             });
                                             refreshScenes();
                                             saveProject(saveData.path);
                                         })
        );
    }

    refreshScenes();


    if (auto *projectname = window.document->GetElementById("project-name")) {
        projectname->SetInnerRML(saveData.projectName);
    }
}

// for main page
inline void setStartupInterfaceEventListeners() {
    auto *tabLoad = window.document->GetElementById("tab-load");
    auto *tabNew = window.document->GetElementById("tab-new");

    if (tabLoad) {
        tabLoad->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-load")->SetClass("active", true);
            doc->GetElementById("tab-new")->SetClass("active", false);
        }));
    }

    if (tabNew) {
        tabNew->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-new")->SetClass("active", true);
            doc->GetElementById("tab-load")->SetClass("active", false);
        }));
    }

    if (auto *tabFolder = window.document->GetElementById("tab-folder")) {
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabFolder] {
            if (auto *fDiv = window.document->GetElementById("tab-folder-div"))
                fDiv->SetProperty("display", "flex");
            if (auto *tDiv = window.document->GetElementById("tab-tct-div"))
                tDiv->SetProperty("display", "none");

            if (auto *bFolder = window.document->GetElementById("browse-folder-btn"))
                bFolder->SetProperty("display", "block");
            if (auto *bTct = window.document->GetElementById("browse-tct-btn"))
                bTct->SetProperty("display", "none");

            tabFolder->SetClassNames("tab-button active");
            if (auto *tabTct = window.document->GetElementById("tab-tct"))
                tabTct->SetClassNames("tab-button");
        }));
    }

    if (auto *tabTct = window.document->GetElementById("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabTct] {
            if (auto *fDiv = window.document->GetElementById("tab-folder-div"))
                fDiv->SetProperty("display", "none");
            if (auto *tDiv = window.document->GetElementById("tab-tct-div"))
                tDiv->SetProperty("display", "flex");

            if (auto *bFolder = window.document->GetElementById("browse-folder-btn"))
                bFolder->SetProperty("display", "none");
            if (auto *bTct = window.document->GetElementById("browse-tct-btn"))
                bTct->SetProperty("display", "block");

            tabTct->SetClassNames("tab-button active");
            if (auto *tabFolder = window.document->GetElementById("tab-folder"))
                tabFolder->SetClassNames("tab-button");
        }));
    }

    if (auto *browseFolderBtn = window.document->GetElementById("browse-folder-btn")) {
        browseFolderBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                if (auto *inputEl = doc->GetElementById("load-dir-input"))
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl))
                        input->SetValue(ToBackwardSlashes(folder));
                std::cout << "clicked" << std::endl;
            }
        }));
    }
    if (auto *browseLoadBtn = window.document->GetElementById("browse-load-btn")) {
        browseLoadBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                if (auto *inputEl = doc->GetElementById("load-dir-input"))
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl))
                        input->SetValue(ToBackwardSlashes(folder));
            }
        }));
    }

    if (auto *browseTctBtn = window.document->GetElementById("browse-tct-btn")) {
        browseTctBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string file = BrowseTCTFile();
            if (!file.empty()) {
                if (auto *inputEl = doc->GetElementById("load-dir-input"))
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl))
                        input->SetValue(ToBackwardSlashes(file));
            }
        }));
    }

    if (auto *browseDirBtn = window.document->GetElementById("browse-btn")) {
        browseDirBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (auto *el = doc->GetElementById("project-dir-input")) {
                if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
                    input->SetValue(ToBackwardSlashes(BrowseFolder()));
                }
            }
        }));
    }
    if (auto *saveNewProjectBtn = window.document->GetElementById("save-btn")) {
        saveNewProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (saveNewProject()) {
                doc->Hide();
                std::cout << saveData.projectName << std::endl;
                if ((window.document = window.context->LoadDocument("assets/interface.rml")))
                    window.document->Show();

                setInterfaceEventListeners();
            }
            //TODO: feedback
        }));
    }
    if (auto *loadProjectBtn = window.document->GetElementById("load-btn")) {
        loadProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (loadProject()) {
                doc->Hide();
                std::cout << saveData.projectName << std::endl;
                if ((window.document = window.context->LoadDocument("assets/interface.rml")))
                    window.document->Show();

                setInterfaceEventListeners();
            }
            //TODO: feedback
        }));
    }

    for (int i = 1; i <= 5; i++) {
        std::string id = "folder-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [doc = window.document, name = proj->GetInnerRML()] {
                                           setSelectedProject(doc, name);
                                       }));
        }
        id = "tct-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [doc = window.document, name = proj->GetInnerRML()] {
                                           setSelectedProject(doc, name);
                                       }));
        }
    }
    if (auto *el = window.document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            input->SetValue(ToBackwardSlashes(GetSaveFolderPath()));
        }
    }

    PopulateFolders(window.document, "../saves/folderSaves/");
}

inline void switchToStartup() {
    if ((window.document = window.context->LoadDocument("assets/startup.rml"))) {
        setStartupInterfaceEventListeners();
        window.document->Show();
    }
}
