#include "Utilities.h"

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"


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

bool Utilities::downscaleAndCrop169(const std::string &inputPath, const std::string &outputPath) {
    int w, h, channels;
    unsigned char* img = stbi_load(inputPath.c_str(), &w, &h, &channels, 4);
    if (!img) return false;

    int in_channels = 4;
    int sw = w / 2;
    int sh = h / 2;
    if (sw <= 0 || sh <= 0) {
        stbi_image_free(img);
        return false;
    }

    std::vector<unsigned char> scaled(sw * sh * in_channels);

    int input_stride = w * in_channels;
    int output_stride = sw * in_channels;

    void *ok = stbir_resize(
        img, w, h, input_stride,
        scaled.data(), sw, sh, output_stride,
        STBIR_RGBA,
        STBIR_TYPE_UINT8,
        STBIR_EDGE_CLAMP,
        STBIR_FILTER_DEFAULT
    );

    stbi_image_free(img);

    if (!ok) return false;

    const float targetAspect = 16.0f / 9.0f;
    int cropW = sw;
    int cropH = static_cast<int>(std::round(sw / targetAspect));

    if (cropH > sh) {
        cropH = sh;
        cropW = static_cast<int>(std::round(sh * targetAspect));
    }

    int offX = std::max(0, (sw - cropW) / 2);
    int offY = std::max(0, (sh - cropH) / 2);

    std::vector<unsigned char> cropped(cropW * cropH * in_channels);
    for (int y = 0; y < cropH; ++y) {
        unsigned char* dst_row = cropped.data() + y * cropW * in_channels;
        unsigned char* src_row = scaled.data() + ((offY + y) * sw + offX) * in_channels;
        memcpy(dst_row, src_row, static_cast<size_t>(cropW * in_channels));
    }

    int write_ok = stbi_write_png(outputPath.c_str(), cropW, cropH, in_channels, cropped.data(), cropW * in_channels);
    return write_ok != 0;
}

bool Utilities::validateString(std::string &value) {
    static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
    if (std::regex_match(value, pattern)) {
        return true;  // return pointer to the original string
    }
    return false;     // invalid input
}

void Utilities::showError(const std::string & msg) {
    std::cerr << msg << std::endl;
}

void Utilities::showInfo(std::string msg) {
}


std::string Utilities::browsePng() {
    OPENFILENAME ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "PNG Files\0*.png\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }

    return ""; // user cancelled
}

Rml::Element* getEl(const std::string& str) {
    if (Rml::Element* temp = getWindow().document->GetElementById(str))
        return temp;
    return nullptr;
}
