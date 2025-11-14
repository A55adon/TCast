#pragma once

#include "global.h"
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
#include "Utilities.h"
#include "SceneEventListener.h"

// ============ GLOBAL VARIABLES ============

inline SaveData saveData;
inline SceneManager sceneManager;
inline Window window(1920, 1080);
inline bool createRecentPath = true;
inline std::filesystem::path projectPath;
inline int activeSceneIndex = -1; // -1 means no scene selected


// ============ FORWARD DECLARATIONS ============

// Project Management
void switchToStartup();

bool saveProject(const std::filesystem::path &projectPath);

bool loadProject();

bool createProject();

void setStartupEventListeners();

void setInterfaceEventListeners();

void setupSceneContextMenu();

// UI Helper Methods
void refreshScenes();

void populateFolders(Rml::ElementDocument *doc, const std::string &path);

void setSelectedProject(Rml::ElementDocument *doc, const std::string &name);

// Validation Methods
bool validateProjectorCount(const std::string &value, int &outCount);

bool validateInputField(const std::string &value, const std::string &fieldName);

// File System Methods
bool verifyFolderStructure(const std::filesystem::path &projectPath);

void fixFolderStructure(const std::filesystem::path &projectPath);

bool loadScenesData(const std::filesystem::path &projectPath);

bool saveJsonToFile(const std::filesystem::path &filePath, const json &data, const std::string &errorContext);

bool createRecentPathFile(const std::filesystem::path &path);

// Event Listener Setup Methods

void setupFileDropdownListeners();
void setupDropdownListeners() {
    setupFileDropdownListeners();
}

void setupProjectorGrid();

void setupSceneManagement();

void setupTabListeners();

void setupBrowseButtons();

void setupProjectActions();

void setupProjectSelection();

// ============ UI HELPER METHODS ============



inline void setSelectedProject(Rml::ElementDocument *doc, const std::string &name) {
    if (auto *label = doc->GetElementById("selected-project-label")) {
        label->SetInnerRML(name);
    }
}

inline void showRenameDialog(int sceneIndex) {
    if (sceneIndex < 0 || sceneIndex >= (int) sceneManager.scenes.size()) {
        std::cerr << "[Error] Invalid scene index for rename: " << sceneIndex << std::endl;
        return;
    }

    // Get the scene item element
    std::string sceneItemId = "scene-item-" + std::to_string(sceneIndex);
    if (auto *sceneItem = window.document->GetElementById(sceneItemId)) {
        // Store the current name for potential cancellation
        std::string currentName = sceneManager.scenes[sceneIndex].sceneName;

        // Replace the scene item content with a container
        sceneItem->SetInnerRML("");
        sceneItem->SetClass("renaming", true);

        // Create container for input and button
        Rml::ElementPtr container = window.document->CreateElement("div");
        container->SetClass("rename-container", true);

        // Create input field
        Rml::ElementPtr input = window.document->CreateElement("input");
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
        Rml::ElementPtr okButton = window.document->CreateElement("button");
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
                                            if (auto *okButton = window.document->GetElementById(
                                                "rename-ok-" + std::to_string(sceneIndex))) {
                                                okButton->Click();
                                            }
                                        }
                                    }));

        // Add event listener for OK button functionality
        okButton->AddEventListener(Rml::EventId::Click, new ButtonHandler([sceneIndex, currentName] {
            //std::cout << "OK button clicked for scene: " << sceneIndex << std::endl;
            if (auto *inputEl = window.document->GetElementById("rename-input-" + std::to_string(sceneIndex))) {
                if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                    std::string newName = input->GetValue();

                    // Validate the new name
                    if (!newName.empty() && validateInputField(newName, "Szenenname")) {
                        // Update the scene name
                        sceneManager.scenes[sceneIndex].sceneName = newName;

                        // Save the project
                        std::filesystem::path fullProjectPath =
                                std::filesystem::path(saveData.path) / saveData.projectName;
                        saveProject(fullProjectPath);

                        //std::cout << "Scene renamed to: " << newName << std::endl;
                    } else {
                        // If invalid, restore original name
                        sceneManager.scenes[sceneIndex].sceneName = currentName;
                        std::cout << "[Error] Rename cancelled or invalid name" << std::endl;
                    }

                    // Always refresh to show the updated name
                    refreshScenes();
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
        if (auto *inputEl = window.document->GetElementById("rename-input-" + std::to_string(sceneIndex))) {
            if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                input->Focus();
            }
        }
    }
}

inline void refreshScenes() {
    if (auto *sceneList = window.document->GetElementById("sceneList")) {
        for (int i = sceneList->GetNumChildren() - 1; i >= 0; --i) {
            Rml::Element *child = sceneList->GetChild(i);
            sceneList->RemoveChild(child);
        }

        for (int i = 0; i < (int) sceneManager.scenes.size(); ++i) {
            Rml::ElementPtr child = window.document->CreateElement("div");
            child->SetClass("scene-item", true);
            child->SetId("scene-item-" + std::to_string(i)); // Add ID for targeting

            // Add active class if this is the selected scene
            if (i == activeSceneIndex) {
                child->SetClass("active", true);
            }

            child->SetAttribute("data-scene-index", std::to_string(i));
            child->SetInnerRML(sceneManager.scenes[i].sceneName);

            // Add event listener
            child->AddEventListener(Rml::EventId::Mouseup, new SceneItemHandler(window.document, i));

            sceneList->AppendChild(std::move(child));
        }
    }
}

inline void renameScene(int index) {
    std::cout << "Rename scene: " << index << std::endl;
    // TODO: Implement rename dialog/input
}

inline void deleteScene(int index) {
    std::cout << "Delete scene: " << index << std::endl;
    if (index >= 0 && index < (int) sceneManager.scenes.size()) {
        sceneManager.scenes.erase(sceneManager.scenes.begin() + index);

        // Update active scene index
        if (activeSceneIndex == index) {
            activeSceneIndex = -1; // No scene selected
        } else if (activeSceneIndex > index) {
            activeSceneIndex--; // Adjust index after deletion
        }

        // Save changes and refresh UI
        std::filesystem::path fullProjectPath = std::filesystem::path(saveData.path) / saveData.projectName;
        saveProject(fullProjectPath);
        refreshScenes();
    }
}

inline void duplicateScene(int index) {
    std::cout << "Duplicate scene: " << index << std::endl;
    if (index >= 0 && index < (int) sceneManager.scenes.size()) {
        SceneData newScene = sceneManager.scenes[index];
        newScene.sceneName = newScene.sceneName + " (Copy)";
        sceneManager.scenes.insert(sceneManager.scenes.begin() + index + 1, newScene);

        // Save changes and refresh UI
        std::filesystem::path fullProjectPath = std::filesystem::path(saveData.path) / saveData.projectName;
        saveProject(fullProjectPath);
        refreshScenes();
    }
}

inline void selectScene(int index) {
    std::cout << "Selecting scene: " << index << " (previously: " << activeSceneIndex << ")" << std::endl;
    activeSceneIndex = index;
    std::cout << "Active scene index set to: " << activeSceneIndex << std::endl;
    refreshScenes(); // This will update the visual highlighting
}

inline void populateFolders(Rml::ElementDocument *doc, const std::string &path) {
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
                                                if (auto *inputEl = doc->GetElementById("load-dir-input")) {
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

// ============ VALIDATION METHODS ============

inline bool validateProjectorCount(const std::string &value, int &outCount) {
    static const std::set<std::string> validWords = {
        "eins", "zwei", "drei", "vier", "fuenf", "fünf", "sechs", "sieben", "acht", "neun"
    };

    // Check for single digit numbers
    if (value.size() == 1 && value[0] >= '1' && value[0] <= '9') {
        outCount = value[0] - '0';
        return true;
    }

    // Check for German number words
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto it = validWords.find(lower);
    if (it != validWords.end()) {
        static const std::unordered_map<std::string, int> wordToNumber = {
            {"eins", 1}, {"zwei", 2}, {"drei", 3}, {"vier", 4},
            {"fuenf", 5}, {"fünf", 5}, {"sechs", 6}, {"sieben", 7},
            {"acht", 8}, {"neun", 9}
        };
        outCount = wordToNumber.at(lower);
        return true;
    }

    return false;
}

inline bool validateInputField(const std::string &value, const std::string &fieldName) {
    static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
    if (!std::regex_match(value, pattern)) {
        if (auto *err = window.document->GetElementById("error-text")) {
            err->SetInnerRML("Ungültige Zeichen in " + fieldName + "! Erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
        }
        return false;
    }
    return true;
}

// ============ FILE SYSTEM METHODS ============

inline bool verifyFolderStructure(const std::filesystem::path &projectPath) {
    if (!std::filesystem::exists(projectPath)) return false;
    if (!std::filesystem::is_directory(projectPath)) return false;
    if (!std::filesystem::exists(projectPath / "saveData.json")) return false;
    if (!std::filesystem::exists(projectPath / "scenesData.json")) return false;
    if (!std::filesystem::exists(projectPath / "resources")) return false;
    return true;
}

inline void fixFolderStructure(const std::filesystem::path &projectPath) {
    if (!std::filesystem::exists(projectPath)) {
        std::filesystem::create_directories(projectPath);
    }
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

inline bool saveJsonToFile(const std::filesystem::path &filePath, const json &data, const std::string &errorContext) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte nicht Speichern – " + errorContext);
        }
        return false;
    }

    file << data.dump(4);
    file.close();

    if (!std::filesystem::exists(filePath)) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("JSON-Datei wurde nicht erstellt: " + errorContext);
        }
        return false;
    }

    return true;
}

inline bool saveProject(const std::filesystem::path &projectPath) {
    if (!verifyFolderStructure(projectPath)) {
        fixFolderStructure(projectPath);
    }

    // Save saveData.json
    {
        std::filesystem::path saveDataPath = projectPath / "saveData.json";
        json j = saveData;
        if (!saveJsonToFile(saveDataPath, j, "saveData.json")) {
            return false;
        }
    }

    // Save scenesData.json
    {
        std::filesystem::path scenePath = projectPath / "scenesData.json";
        if (sceneManager.scenes.empty()) {
            // Create default scene
            sceneManager.scenes.push_back({saveData.projectName, {}});
        }

        json j = sceneManager;
        if (!saveJsonToFile(scenePath, j, "scenesData.json")) {
            return false;
        }
    }

    return true;
}

inline bool loadScenesData(const std::filesystem::path &projectPath) {
    std::cout << "[Helper][loadScenesData] Loading scenes data" << std::endl;

    std::ifstream file(projectPath / "scenesData.json");
    if (!file.is_open()) {
        std::cerr << "[loadScenes] Couldn't open " << projectPath / "scensesData.json" << std::endl;
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
        std::cerr << "[Helper][loadScenesData] Error parsing JSON: " << e.what() << std::endl;
        return false;
    }

    std::cout << "[Helper][loadScenesData] Loaded " << sceneManager.scenes.size() << " scenes" << std::endl;
    refreshScenes();
    return true;
}

inline bool createRecentPathFile(const std::filesystem::path &path) {
    if (!createRecentPath) return true;

    if (!std::filesystem::exists("../saves")) {
        std::filesystem::create_directories("../saves");
    }

    std::ofstream recent("../saves/recent.path", std::ios::out | std::ios::binary);
    if (recent) {
        recent << path.string();
        std::cout << "Created recent.path with path: " << path << std::endl;
        return true;
    } else {
        std::cerr << "Failed to create recent.path" << std::endl;
        return false;
    }
}

// ============ PROJECT MANAGEMENT METHODS ============

inline bool createProject() {
    // Validate and get project name
    if (auto *el = window.document->GetElementById("project-name-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            if (!validateInputField(value, "Projektname")) return false;
            saveData.projectName = value;
        }
    }

    // Validate and get projector count
    if (auto *el = window.document->GetElementById("projector-count-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            int count = 0;
            if (!validateProjectorCount(value, count)) {
                if (auto *err = window.document->GetElementById("error-text")) {
                    err->SetInnerRML("Nur Zahlen 1-9 erlaubt!");
                }
                return false;
            }
            saveData.projectorCount = count;
        }
    }

    // Validate and get project description
    if (auto *el = window.document->GetElementById("project-desc-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();
            if (!validateInputField(value, "Beschreibung")) return false;
            saveData.description = value;
        }
    }

    // Get project directory
    std::filesystem::path basePath;
    if (auto *el = window.document->GetElementById("project-dir-input")) {
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
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Projekt existiert bereits – bitte anderen Namen wählen!");
        }
        return false;
    }

    // Create project directory
    std::error_code ec;
    if (!std::filesystem::create_directories(fullProjectPath, ec) || ec) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte Ordner nicht erstellen: " + ec.message());
        }
        return false;
    }

    projectPath = fullProjectPath;

    // Save project data
    if (!saveProject(fullProjectPath)) {
        return false;
    }

    // Create recent path file
    createRecentPathFile(fullProjectPath);

    return true;
}

inline bool loadProject() {
    auto *errEl = window.document->GetElementById("load-error-text");
    if (errEl) {
        errEl->SetInnerRML(""); // Clear old errors
    }

    // Get project path
    std::string path;
    if (auto *el = window.document->GetElementById("load-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            path = input->GetValue();
        }
    }

    if (path.empty()) {
        if (errEl) errEl->SetInnerRML("Bitte einen Pfad eingeben.");
        return false;
    }

    projectPath = path;

    // Load saveData.json
    std::filesystem::path saveDataPath = path + "/saveData.json";
    std::ifstream file(saveDataPath);
    if (!file.is_open()) {
        if (errEl) errEl->SetInnerRML("Datei konnte nicht geöffnet werden: " + saveDataPath.string());
        return false;
    }

    try {
        json j;
        file >> j;
        saveData = j.get<SaveData>();
    } catch (const std::exception &e) {
        if (errEl) errEl->SetInnerRML(std::string("Fehler beim Lesen der saveData.json: ") + e.what());
        return false;
    }

    // Load scenesData.json
    std::filesystem::path scenesPath = path + "/scenesData.json";
    std::ifstream scenesFile(scenesPath);
    if (!scenesFile.is_open()) {
        if (errEl) errEl->SetInnerRML("Datei konnte nicht geöffnet werden: " + scenesPath.string());
        return false;
    }

    try {
        json j;
        scenesFile >> j;
        sceneManager = j.get<SceneManager>();
    } catch (const std::exception &e) {
        if (errEl) errEl->SetInnerRML(std::string("Fehler beim Lesen der scenesData.json: ") + e.what());
        return false;
    }

    // Create recent path file
    createRecentPathFile(path);

    return true;
}

// ============ EVENT LISTENER SETUP METHODS ============

void setupFileDropdownListeners() {
    // New Project
    if (auto *dropdownNewproject = window.document->GetElementById("file-dropdown-newproject")) {
        dropdownNewproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new")->SetClass("active", true);
            window.document->GetElementById("tab-load")->SetClass("active", false);
            window.document->GetElementById("tab-load")->SetAttribute("style", "display:none");
        }));
    }

    // Load Project
    if (auto *dropdownLoadproject = window.document->GetElementById("file-dropdown-loadproject")) {
        dropdownLoadproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-load")->SetClass("active", true);
            window.document->GetElementById("tab-new")->SetClass("active", false);
            window.document->GetElementById("tab-new")->SetAttribute("style", "display:none");
        }));
    }

    // Export Project
    if (auto *filedropdownexportproject = window.document->GetElementById("file-dropdown-exportproject")) {
        filedropdownexportproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            try {
                std::string fullPath = saveData.path + "\\" + saveData.projectName;

                if (std::filesystem::exists(fullPath + ".tct")) {
                    std::filesystem::remove_all(fullPath + ".tct");
                }

                std::string command = "powershell Compress-Archive -Path \"" + fullPath +
                                      "\\*\" -DestinationPath \"" + fullPath + ".zip\" -Force";
                int result = std::system(command.c_str());

                if (result == 0) {
                    std::cout << "Folder zipped successfully.\n";
                } else {
                    std::cerr << "Failed to zip folder. Exit code: " << result << '\n';
                    return;
                }

                std::filesystem::rename(fullPath + ".zip", fullPath + ".tct");
                std::cout << "File " + saveData.projectName + ".tct exported successfully to: " + saveData.path <<
                        std::endl;
            } catch (const std::filesystem::filesystem_error &e) {
                std::cerr << "Filesystem error: " << e.what() << '\n';
            }
            // TODO: Add user feedback
        }));
    }

    // Import Project
    if (auto *filedropdownimportproject = window.document->GetElementById("file-dropdown-importproject")) {
        filedropdownimportproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            // TODO: Implement project import functionality
            // TODO: Add user feedback
        }));
    }

    // Close Project
    if (auto *dropdowncloseproject = window.document->GetElementById("file-dropdown-closeproject")) {
        dropdowncloseproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            switchToStartup();
        }));
    }

    // Close Program
    if (auto *dropdowncloseprogramm = window.document->GetElementById("file-dropdown-closeprogramm")) {
        dropdowncloseprogramm->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            exit(EXIT_SUCCESS);
        }));
    }
}

void setupProjectorGrid() {
    if (auto *projectorgrid = window.document->GetElementById("projectorGrid")) {
        std::cout << "[Info] Projector count: " << saveData.projectorCount << std::endl;
        for (int i = 0; i < saveData.projectorCount; ++i) {
            Rml::ElementPtr child = window.document->CreateElement("div");
            child->SetClass("projector", true);

            Rml::ElementPtr span = window.document->CreateElement("span");
            span->SetInnerRML("Beamer " + std::to_string(i + 1));
            child->AppendChild(std::move(span));

            projectorgrid->AppendChild(std::move(child));
        }
    }
}

void setupSceneManagement() {
    refreshScenes();

    if (auto *addSceneButton = window.document->GetElementById("add-scene-btn")) {
        addSceneButton->AddEventListener(Rml::EventId::Click,
                                         new ButtonHandler([]() {
                                             sceneManager.scenes.emplace_back(SceneData{
                                                 "Szene " + std::to_string(sceneManager.scenes.size() + 1),
                                                 {}
                                             });
                                             saveProject(projectPath);

                                             refreshScenes();
                                         })
        );
    }
    //auto sceneButtonsArrowUp = Utilities::getEl("scene-buttons-arrow-up");
    //    sceneButtonsArrowUp->AddEventListener(Rml::EventId::Click,
    //        new ButtonHandler([]() {
    //            if (activeSceneIndex != -1 && activeSceneIndex != 0) {
    //                SceneData temp = sceneManager.scenes[activeSceneIndex - 1];
    //                sceneManager.scenes[activeSceneIndex - 1] = sceneManager.scenes[activeSceneIndex];
    //                sceneManager.scenes[activeSceneIndex] = temp;
    //                activeSceneIndex--;
    //                saveProject(projectPath);
//
    //                refreshScenes();
    //            }
    //        })
    //    );
    if (auto *sceneButtonArrowDown = window.document->GetElementById("scene-buttons-arrow-down")) {
        std::cout << activeSceneIndex << std::endl;

        sceneButtonArrowDown->AddEventListener(Rml::EventId::Click,
            new ButtonHandler([]() {
                if (activeSceneIndex != -1) {
                    SceneData temp = sceneManager.scenes[activeSceneIndex + 1];
                    std::cout << "Swapping scene " << activeSceneIndex << " with " << (
                        activeSceneIndex + 1) << std::endl;
                    sceneManager.scenes[activeSceneIndex + 1] = sceneManager.scenes[
                        activeSceneIndex];
                    sceneManager.scenes[activeSceneIndex] = temp;
                    activeSceneIndex--;
                    std::filesystem::path fullProjectPath =
                            std::filesystem::path(saveData.path) / saveData.
                            projectName;
                    saveProject(fullProjectPath);

                    refreshScenes();
                }
            })
        );
    }
    std::cout << "Scenes count: " << sceneManager.scenes.size() << std::endl;


}
//
void setupTabListeners() {
    // Main tabs (Load/New)
    if (auto *tabLoad = window.document->GetElementById("tab-load")) {
        tabLoad->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-load")->SetClass("active", true);
            window.document->GetElementById("tab-new")->SetClass("active", false);
        }));
    }

    if (auto *tabNew = window.document->GetElementById("tab-new")) {
        tabNew->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new")->SetClass("active", true);
            window.document->GetElementById("tab-load")->SetClass("active", false);
        }));
    }

    // Sub-tabs (Folder/TCT)
    if (auto *tabFolder = window.document->GetElementById("tab-folder")) {
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabFolder] {
            window.document->GetElementById("tab-folder-div")->SetProperty("display", "flex");
            window.document->GetElementById("tab-tct-div")->SetProperty("display", "none");
            window.document->GetElementById("browse-folder-btn")->SetProperty("display", "block");
            window.document->GetElementById("browse-tct-btn")->SetProperty("display", "none");
            tabFolder->SetClassNames("tab-button active");
            window.document->GetElementById("tab-tct")->SetClassNames("tab-button");
        }));
    }

    if (auto *tabTct = window.document->GetElementById("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabTct] {
            window.document->GetElementById("tab-folder-div")->SetProperty("display", "none");
            window.document->GetElementById("tab-tct-div")->SetProperty("display", "flex");
            window.document->GetElementById("browse-folder-btn")->SetProperty("display", "none");
            window.document->GetElementById("browse-tct-btn")->SetProperty("display", "block");
            tabTct->SetClassNames("tab-button active");
            window.document->GetElementById("tab-folder")->SetClassNames("tab-button");
        }));
    }
}
//
void setupBrowseButtons() {
    // Folder browse buttons
    if (auto *browseFolderBtn = window.document->GetElementById("browse-folder-btn")) {
        browseFolderBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            std::string folder = Utilities::browseFolder();
            if (!folder.empty()) {
                if (auto *inputEl = window.document->GetElementById("load-dir-input")) {
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                        input->SetValue(Utilities::toBackwardSlashes(folder));
                    }
                }
            }
        }));
    }

    if (auto *browseLoadBtn = window.document->GetElementById("browse-load-btn")) {
        browseLoadBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            std::string folder = Utilities::browseFolder();
            if (!folder.empty()) {
                if (auto *inputEl = window.document->GetElementById("load-dir-input")) {
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                        input->SetValue(Utilities::toBackwardSlashes(folder));
                    }
                }
            }
        }));
    }

    // TCT file browse button
    if (auto *browseTctBtn = window.document->GetElementById("browse-tct-btn")) {
        browseTctBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            std::string file = Utilities::browseTCTFile();
            if (!file.empty()) {
                if (auto *inputEl = window.document->GetElementById("load-dir-input")) {
                    if (auto *input = dynamic_cast<Rml::ElementFormControl *>(inputEl)) {
                        input->SetValue(Utilities::toBackwardSlashes(file));
                    }
                }
            }
        }));
    }

    // Project directory browse button
    if (auto *browseDirBtn = window.document->GetElementById("browse-btn")) {
        browseDirBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (auto *el = window.document->GetElementById("project-dir-input")) {
                if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
                    input->SetValue(Utilities::toBackwardSlashes(Utilities::browseFolder()));
                }
            }
        }));
    }
}
//
void setupProjectActions() {
    // Save new project
    if (auto *saveNewProjectBtn = window.document->GetElementById("save-btn")) {
        saveNewProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (createProject()) {
                window.document->Hide();
                std::cout << "Project created: " << saveData.projectName << std::endl;
                if ((window.document = window.context->LoadDocument("assets/interface.rml"))) {
                    window.document->Show();
                }
                setInterfaceEventListeners();
            }
            // TODO: Add user feedback
        }));
    }

    // Load project
    if (auto *loadProjectBtn = window.document->GetElementById("load-btn")) {
        loadProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (loadProject()) {
                window.document->Hide();
                std::cout << "Project loaded: " << saveData.projectName << std::endl;
                if ((window.document = window.context->LoadDocument("assets/interface.rml"))) {
                    window.document->Show();
                }
                setInterfaceEventListeners();
            }
            // TODO: Add user feedback
        }));
    }
}
//
void setupProjectSelection() {
    for (int i = 1; i <= 5; i++) {
        std::string id = "folder-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [name = proj->GetInnerRML()] {
                                           setSelectedProject(window.document, name);
                                       }));
        }

        id = "tct-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [name = proj->GetInnerRML()] {
                                           setSelectedProject(window.document, name);
                                       }));
        }
    }
}

// ============ MAIN EVENT LISTENER SETUP ============
//
void setStartupEventListeners() {
    setupTabListeners();
    setupBrowseButtons();
    setupProjectActions();
    setupProjectSelection();

    // Set default directory
    if (auto *el = window.document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            input->SetValue(Utilities::toBackwardSlashes(Utilities::getSaveFolderPath()));
        }
    }

    populateFolders(window.document, "../saves/folderSaves/");
}

void setInterfaceEventListeners() {
    if (!loadScenesData(projectPath)) {
        std::cerr << "Failed to load scenesData" << std::endl;
    }
    if (auto *projectname = window.document->GetElementById("project-name")) {
        std::cout << "[Info] Setting project name: " << saveData.projectName << std::endl;
        projectname->SetInnerRML(saveData.projectName);
    }
    setupDropdownListeners();
    setupProjectorGrid();
    setupSceneManagement();

    setupSceneContextMenu();
}

void setupSceneContextMenu() {
    // Rename button in context menu
    if (auto *contextRename = window.document->GetElementById("context-rename")) {
        contextRename->AddEventListener(Rml::EventId::Click, new SceneContextMenuHandler(window.document, "rename"));
    }

    if (auto *contextDelete = window.document->GetElementById("context-delete")) {
        contextDelete->AddEventListener(Rml::EventId::Click, new SceneContextMenuHandler(window.document, "delete"));
    }

    if (auto *contextDuplicate = window.document->GetElementById("context-duplicate")) {
        contextDuplicate->AddEventListener(Rml::EventId::Click,
                                           new SceneContextMenuHandler(window.document, "duplicate"));
    }

    if (auto *body = window.document->GetElementById("body")) {
        body->AddEventListener(Rml::EventId::Click, new ButtonHandler([] {
            if (auto *contextMenu = window.document->GetElementById("sceneContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }));
    }

    window.document->AddEventListener(Rml::EventId::Keydown, new KeyEventHandler([](Rml::Event &event) {
        if (event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_ESCAPE) {
            if (auto *contextMenu = window.document->GetElementById("sceneContextMenu")) {
                contextMenu->SetProperty("display", "none");
            }
        }
    }));
}

// ============ SCENE MANAGEMENT ============

void switchToStartup() {
    if ((window.document = window.context->LoadDocument("assets/startup.rml"))) {
        setStartupEventListeners();
        window.document->Show();
    }
}
