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
using json = nlohmann::json;

struct SaveData {
    std::string projectName;
    Rml::String beamerCount{};
    std::string description;
    std::string path;
};

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
