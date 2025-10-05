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

using json = nlohmann::json;

struct SaveData {
    std::string projectName;
    Rml::String beamerCount{};
    std::string description;
    std::string path;
};

inline SaveData saveData;
inline Window window = Window(1920, 1080);

inline void to_json(json &j, const SaveData &d) {
    j = json{
                {"projectName", d.projectName},
                {"beamerCount", d.beamerCount},
                {"description", d.description},
                {"path", d.path}
    };
}
inline void from_json(const json &j, SaveData &d) {
    j.at("projectName").get_to(d.projectName);
    j.at("beamerCount").get_to(d.beamerCount);
    j.at("description").get_to(d.description);
    j.at("path").get_to(d.path);
}


inline void SetSelectedProject(Rml::ElementDocument *doc, const std::string &name) {
    if (auto *label = doc->GetElementById("selected-project-label")) {
        label->SetInnerRML(name.c_str());
    }
}

inline std::string BrowseFolder() {
    std::string result;
    IFileDialog *pfd = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD options;
        if (SUCCEEDED(pfd->GetOptions(&options))) {
            pfd->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        if (SUCCEEDED(pfd->Show(nullptr))) {
            IShellItem *psi = nullptr;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, buffer, MAX_PATH, nullptr, nullptr);
                    result = buffer;
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

inline std::string BrowseTCTFile() {
    std::string result;
    IFileDialog *pfd = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD options;
        if (SUCCEEDED(pfd->GetOptions(&options))) {
            pfd->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
        }
        COMDLG_FILTERSPEC rgSpec[] = {{L"TCT Files (*.tct)", L"*.tct"}};
        pfd->SetFileTypes(1, rgSpec);
        pfd->SetFileTypeIndex(1);
        pfd->SetDefaultExtension(L"tct");
        if (SUCCEEDED(pfd->Show(nullptr))) {
            IShellItem *psi = nullptr;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, buffer, MAX_PATH, nullptr, nullptr);
                    result = buffer;
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

inline std::string GetExecutablePath() {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return "";
    }
    return std::string(buffer, len);
}

inline std::string GetSaveFolderPath() {
    std::filesystem::path exePath = GetExecutablePath();
    std::filesystem::path exeDir  = exePath.parent_path();      // ...\cmake-build-debug (build directory)
    std::filesystem::path projectDir = exeDir.parent_path();    // ...\TCast
    std::filesystem::path savePath = projectDir / "saves" / "folderSaves";

    // make sure directory exists
    std::filesystem::create_directories(savePath);

    return savePath.string();
}

inline std::string ToBackwardSlashes(const std::string& path) {
    std::string fixed = path;
    std::replace(fixed.begin(), fixed.end(), '/', '\\');
    return fixed;
}

inline bool validateBeamerCount(const Rml::String& value, int& outCount) {
    static const std::set<std::string> validWords = {
        "eins","zwei","drei","vier","fuenf","fünf","sechs","sieben","acht","neun"
    };

    //numbers as letters
    if (value.size() == 1 && value[0] >= '1' && value[0] <= '9') {
        outCount = value[0] - '0';
        return true;
    }

    // big/small letters
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    auto it = validWords.find(lower);
    if (it != validWords.end()) {
        if (lower == "eins")   outCount = 1;
        else if (lower == "zwei")  outCount = 2;
        else if (lower == "drei")  outCount = 3;
        else if (lower == "vier")  outCount = 4;
        else if (lower == "fünf" || lower == "fuenf") outCount = 5;
        else if (lower == "sechs") outCount = 6;
        else if (lower == "sieben") outCount = 7;
        else if (lower == "acht")   outCount = 8;
        else if (lower == "neun")   outCount = 9;
        return true;
    }

    return false;
}

inline bool saveNewProject() {
    std::string path;

    if (auto *el = window.document->GetElementById("project-name-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();

            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto *element_error = window.document->GetElementById("error-text")) {
                    element_error->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                }
                return false;
            }

            saveData.projectName = value;
        }
    }
    if (auto *el = window.document->GetElementById("projector-count-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            Rml::String value = input->GetValue();
            int count = 0;
            if (!validateBeamerCount(value, count)) {
                if (auto *err = window.document->GetElementById("error-text")) {
                    err->SetInnerRML("Nur Zahlen 1-9 erlaubt!");
                }
                return false;
            }
            saveData.beamerCount = count;
        }
    }
    if (auto *el = window.document->GetElementById("project-desc-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            Rml::String value = input->GetValue();

            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto *element_error = window.document->GetElementById("error-text")) {
                    element_error->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                }
                return false;
            }

            saveData.description = value;
        }
    }

    if (auto *el = window.document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            Rml::String value = input->GetValue();

            saveData.path = value;
            std::string forwardSlashpath = value + "/" + saveData.projectName + "/" + "projectData" + ".json";
            path = ToBackwardSlashes(forwardSlashpath);
            std::cout << path << std::endl;
        }
    }

    std::filesystem::path filePath = path;
    auto folderPath = filePath.parent_path();
    std::error_code ec;

    // Check if folder exists
    if (std::filesystem::exists(folderPath)) {
        if (!std::filesystem::is_directory(folderPath)) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML("Pfad existiert, ist aber keine Ordnerstruktur!");
            }
            return false;
        }
    } else {
        // Try to create missing directories
        if (!std::filesystem::create_directories(folderPath, ec) || ec) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML(("Konnte Ordner nicht erstellen: " + ec.message()).c_str());
            }
            return false;
        }
    }

    // Prevent overwriting existing project file
    if (std::filesystem::exists(filePath)) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Projekt existiert bereits – bitte anderen Namen wählen!");
        }
        return false;
    }

    // Open file for writing
    json j = saveData;
    std::ofstream file(filePath);
    if (!file.is_open()) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte Datei nicht erstellen – ungültiger Pfad oder Rechteproblem");
        }
        return false;
    }

    // Write JSON
    file << j.dump(4);
    file.close();

    // Verify file exists
    if (!std::filesystem::exists(filePath)) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
        }
        return false;
    }

    return true;

}

bool loadProject() {
    std::string path;


    auto *errEl = window.document->GetElementById("load-error-text");
    if (errEl)
        errEl->SetInnerRML(""); // clear old errors

    if (auto *el = window.document->GetElementById("load-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            path = input->GetValue();
        }
    }
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
        if (errEl) errEl->SetInnerRML(std::string("Fehler beim Lesen der JSON-Datei: ") + e.what());
        return false;
    }


    return true;
}


void setStartupInterfaceEventListeners()
{
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

    if (auto* tabFolder = window.document->GetElementById("tab-folder")) {
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabFolder] {
            if (auto* fDiv = window.document->GetElementById("tab-folder-div"))
                fDiv->SetProperty("display", "flex");
            if (auto* tDiv = window.document->GetElementById("tab-tct-div"))
                tDiv->SetProperty("display", "none");

            if (auto* bFolder = window.document->GetElementById("browse-folder-btn"))
                bFolder->SetProperty("display", "block");
            if (auto* bTct = window.document->GetElementById("browse-tct-btn"))
                bTct->SetProperty("display", "none");

            tabFolder->SetClassNames("tab-button active");
            if (auto* tabTct = window.document->GetElementById("tab-tct"))
                tabTct->SetClassNames("tab-button");
        }));
    }

    if (auto* tabTct = window.document->GetElementById("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabTct] {
            if (auto* fDiv = window.document->GetElementById("tab-folder-div"))
                fDiv->SetProperty("display", "none");
            if (auto* tDiv = window.document->GetElementById("tab-tct-div"))
                tDiv->SetProperty("display", "flex");

            if (auto* bFolder = window.document->GetElementById("browse-folder-btn"))
                bFolder->SetProperty("display", "none");
            if (auto* bTct = window.document->GetElementById("browse-tct-btn"))
                bTct->SetProperty("display", "block");

            tabTct->SetClassNames("tab-button active");
            if (auto* tabFolder = window.document->GetElementById("tab-folder"))
                tabFolder->SetClassNames("tab-button");
        }));
    }

    if (auto *browseFolderBtn = window.document->GetElementById("browse-folder-btn")) {
        browseFolderBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                if (auto* inputEl = doc->GetElementById("load-dir-input"))
                    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                        input->SetValue(ToBackwardSlashes(folder));
                        std::cout << "clicked" << std::endl;
            }
        }));
    }
    if (auto *browseLoadBtn = window.document->GetElementById("browse-load-btn")) {
        browseLoadBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                if (auto* inputEl = doc->GetElementById("load-dir-input"))
                    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                        input->SetValue(ToBackwardSlashes(folder));
            }
        }));
    }

    if (auto *browseTctBtn = window.document->GetElementById("browse-tct-btn")) {
        browseTctBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string file = BrowseTCTFile();
            if (!file.empty()) {
                if (auto* inputEl = doc->GetElementById("load-dir-input"))
                    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
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
            }
        }));
    }
    if (auto *loadProjectBtn = window.document->GetElementById("load-btn")) {
        loadProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (loadProject()) {
                doc->Hide();
                std::cout << saveData.projectName << std::endl;
            }
        }));
    }

    for (int i = 1; i <= 5; i++) {
        std::string id = "folder-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [doc = window.document, name = proj->GetInnerRML()] {
                                           SetSelectedProject(doc, name);
                                       }));
        }
        id = "tct-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [doc = window.document, name = proj->GetInnerRML()] {
                                           SetSelectedProject(doc, name);
                                       }));
        }
    }
    if (auto *el = window.document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            input->SetValue(ToBackwardSlashes(GetSaveFolderPath()));
        }
    }
}

void PopulateFolders(Rml::ElementDocument* doc, const std::string& path) {
    namespace fs = std::filesystem;
    Rml::Element* container = doc->GetElementById("tab-folder-list");
    if (!container) return;
    container->SetInnerRML("");

    for (auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            std::string fullPath = entry.path().string();

            Rml::ElementPtr folderDiv = doc->CreateElement("div");
            folderDiv->SetClassNames("sample-project");
            folderDiv->SetId("folder-" + folderName);
            folderDiv->SetInnerRML(folderName);

            folderDiv->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                 [doc, fullPath, folderName] {
                     if (auto* inputEl = doc->GetElementById("load-dir-input"))
                         if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                             input->SetValue(ToBackwardSlashes(fullPath));
                 }
             ));

            container->AppendChild(std::move(folderDiv));
        }
    }
}



