#pragma once

using json = nlohmann::json;

struct SaveData {
    std::string projectName;
    int projectorCount{};
    std::string description;
    std::string path;
};

struct SceneData {
    std::string sceneName;
    std::vector<std::string> sources;

    SceneData(){}

    SceneData(const std::string& name, std::vector<std::string> src)
        : sceneName(name), sources(std::move(src)) {}
};

struct SceneManager {
    std::vector<SceneData> scenes;
};

inline void to_json(json &j, const SaveData &d) {
    j = json{
            {"projectName", d.projectName},
            {"projectorCount", d.projectorCount},
            {"description", d.description},
            {"path", d.path}
    };
}

inline void to_json(json &j, const SceneData &s) {
    j = json{
            {"sceneName", s.sceneName},
            {"sources", s.sources}
    };
}

inline void to_json(json &j, const SceneManager &m) {
    j = json{
            {"scenes", m.scenes}
    };
}

inline void from_json(const json &j, SaveData &d) {
    j.at("projectName").get_to(d.projectName);
    j.at("projectorCount").get_to(d.projectorCount);
    j.at("description").get_to(d.description);
    j.at("path").get_to(d.path);
}

inline void from_json(const json &j, SceneData &s) {
    j.at("sceneName").get_to(s.sceneName);
    j.at("sources").get_to(s.sources);
}

inline void from_json(const json &j, SceneManager &m) {
    j.at("scenes").get_to(m.scenes);
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
    std::filesystem::path exeDir = exePath.parent_path(); // ...\cmake-build-debug (build directory)
    std::filesystem::path projectDir = exeDir.parent_path(); // ...\TCast
    std::filesystem::path savePath = projectDir / "saves" / "folderSaves";

    // make sure directory exists
    std::filesystem::create_directories(savePath);

    return savePath.string();
}

inline std::string ToBackwardSlashes(const std::string &path) {
    std::string fixed = path;
    std::replace(fixed.begin(), fixed.end(), '/', '\\');
    return fixed;
}

