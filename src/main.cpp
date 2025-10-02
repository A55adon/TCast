#include <fstream>
#include <iostream>
#include <regex>
#include <shlobj.h>
#include <set>
#include <algorithm>
#include <windows.h>
#include "ButtonListener.h"
#include "Window.h"
#include "Shell.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

std::string BrowseFolder() {
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

std::string BrowseTCTFile() {
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

std::string GetExecutablePath() {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return "";
    }
    return std::string(buffer, len);
}

std::string GetSaveFolderPath() {
    std::filesystem::path exePath = GetExecutablePath();
    std::filesystem::path exeDir  = exePath.parent_path();      // ...\cmake-build-debug
    std::filesystem::path projectDir = exeDir.parent_path();    // ...\TCast
    std::filesystem::path savePath = projectDir / "saves" / "folderSaves";

    // make sure directory exists
    std::filesystem::create_directories(savePath);

    return savePath.string();
}


std::string ToBackwardSlashes(const std::string& path) {
    std::string fixed = path;
    std::replace(fixed.begin(), fixed.end(), '/', '\\');
    return fixed;
}
bool validateBeamerCount(const Rml::String& value, int& outCount) {
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

struct SaveData {
    std::string projectName;
    Rml::String beamerCount{};
    std::string description;
    std::string path;
};

void to_json(json &j, const SaveData &d) {
    j = json{
        {"projectName", d.projectName},
        {"beamerCount", d.beamerCount},
        {"description", d.description},
        {"path", d.path}
    };
}

void from_json(const json &j, SaveData &d) {
    j.at("projectName").get_to(d.projectName);
    j.at("beamerCount").get_to(d.beamerCount);
    j.at("description").get_to(d.description);
    j.at("path").get_to(d.path);
}


void SetSelectedProject(Rml::ElementDocument *doc, const std::string &name) {
    if (auto *label = doc->GetElementById("selected-project-label")) {
        label->SetInnerRML(name.c_str());
    }
}

void setStartupInterfaceEventListeners();

bool saveNewProject();


Window window = Window(1920, 1080);

int main() {

    if ((window.document = window.context->LoadDocument("assets/interface.rml")))
        window.document->Show();

    setStartupInterfaceEventListeners();

    while (window.running) {
        window.update();
    }
    return 0;
}

void setStartupInterfaceEventListeners() {
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
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-folder-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-tct-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-folder")->SetClass("active", true);
            doc->GetElementById("tab-tct")->SetClass("active", false);
        }));
    }

    if (auto *tabTct = window.document->GetElementById("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-folder-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-tct-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-tct")->SetClass("active", true);
            doc->GetElementById("tab-folder")->SetClass("active", false);
        }));
    }

    if (auto *browseFolderBtn = window.document->GetElementById("browse-folder-btn")) {
        browseFolderBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                SetSelectedProject(doc, folder);
            }
        }));
    }

    if (auto *browseTctBtn = window.document->GetElementById("browse-tct-btn")) {
        browseTctBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string file = BrowseTCTFile();
            if (!file.empty()) {
                SetSelectedProject(doc, file);
            }
        }));
    }
    if (auto *saveNewProjectBtn = window.document->GetElementById("save-btn")) {
        saveNewProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (saveNewProject()) {
                window.document->Hide();
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

bool saveNewProject() {
    SaveData saveData;
    std::string path;

    if (auto *el = window.document->GetElementById("project-name-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();

            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto *el = window.document->GetElementById("error-text")) {
                    el->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
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
                if (auto *el = window.document->GetElementById("error-text")) {
                    el->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
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
            if (value == ToBackwardSlashes(GetSaveFolderPath())){
                std::string forwardSlashpath = value + "/" + saveData.projectName + "/" + saveData.projectName + ".json";
                path = ToBackwardSlashes(forwardSlashpath);
            }
            else
                path = value;

            std::cout << path << std::endl;
        }
    }
    std::filesystem::path filePath = path;
    std::error_code ec;

    // Create parent directories
    std::filesystem::create_directories(filePath.parent_path(), ec);
    if (ec) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML(("Ungültiger Pfad: " + ec.message()).c_str());
        }
        return false;
    }

    // write JSON to file
    json j = saveData;
    std::ofstream file(path);
    if (!file.is_open()) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte Datei nicht erstellen – ungültiger Pfad oder Rechteproblem");
        }
        return false;
    }
    file << j.dump(4);
    file.close();

    // Check if the file actually exists
    if (!std::filesystem::exists(filePath)) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
        }
        return false;
    }

    // Saved succesfully
    return true;

}
