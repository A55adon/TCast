#include "Utilities.h"

// JSON serialization implementations
void to_json(json &j, const SaveData &d) {
    j = json{
            {"projectName", d.projectName},
            {"projectorCount", d.projectorCount},
            {"description", d.description},
            {"path", d.path}
    };
}

void to_json(json &j, const SceneData &s) {
    j = json{
            {"sceneName", s.sceneName},
            {"sources", s.sources}
    };
}

void to_json(json &j, const SceneManager &m) {
    j = json{
            {"scenes", m.scenes}
    };
}

void from_json(const json &j, SaveData &d) {
    j.at("projectName").get_to(d.projectName);
    j.at("projectorCount").get_to(d.projectorCount);
    j.at("description").get_to(d.description);
    j.at("path").get_to(d.path);
}

void from_json(const json &j, SceneData &s) {
    j.at("sceneName").get_to(s.sceneName);
    j.at("sources").get_to(s.sources);
}

void from_json(const json &j, SceneManager &m) {
    j.at("scenes").get_to(m.scenes);
}

// Utilities class implementations
std::string Utilities::browseFolder() {
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

std::string Utilities::browseTCTFile() {
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

std::string Utilities::getExecutablePath() {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return "";
    }
    return std::string(buffer, len);
}

std::string Utilities::getSaveFolderPath() {
    std::filesystem::path exePath = getExecutablePath();
    std::filesystem::path exeDir = exePath.parent_path(); // ...\cmake-build-debug (build directory)
    std::filesystem::path projectDir = exeDir.parent_path(); // ...\TCast
    std::filesystem::path savePath = projectDir / "saves" / "folderSaves";

    // make sure directory exists
    std::filesystem::create_directories(savePath);

    return savePath.string();
}

std::string Utilities::toBackwardSlashes(const std::string &path) {
    std::string fixed = path;
    std::replace(fixed.begin(), fixed.end(), '/', '\\');
    return fixed;
}

std::filesystem::path Utilities::getRecentPath() {
    static const std::filesystem::path recentFilePath = "../saves/recent.path";

    // Check if recent.path exists
    if (!std::filesystem::exists(recentFilePath)) {
        return std::filesystem::path{};
    }

    // Read the path from recent.path
    std::ifstream file(recentFilePath);
    if (!file.is_open()) {
        std::cerr << "Unable to open recent file: " << recentFilePath << '\n';
        return std::filesystem::path{};
    }

    std::string line;
    std::getline(file, line);
    file.close();

    // Trim whitespace
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::filesystem::path{};
    }
    const auto last = line.find_last_not_of(" \t\r\n");
    std::string pathStr = line.substr(first, last - first + 1);

    std::filesystem::path result{ pathStr };
    if (!std::filesystem::exists(result)) {
        std::cout << "Path does not exist - going to startup: " << pathStr << '\n';
    }
    std::cout << "Found recent: " <<  pathStr << std::endl;
    return result;
}

Rml::Element* Utilities::getEl(const std::string& str) {
    //if (Rml::Element* temp = window.document->GetElementById(str))
    //    return temp;
    return nullptr;
}
