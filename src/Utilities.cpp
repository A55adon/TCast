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

    int targetW = w;
    int targetH = h;

    const int maxW = 1920;
    const int maxH = 1080;

    // Determine if downscaling is needed
    float aspect = 16.0f / 9.0f;
    float imgAspect = static_cast<float>(w) / h;

    if (w > maxW || h > maxH) {
        if (imgAspect >= aspect) {
            targetW = maxW;
            targetH = static_cast<int>(std::round(targetW / aspect));
        } else {
            targetH = maxH;
            targetW = static_cast<int>(std::round(targetH * aspect));
        }
    } else {
        // For images smaller than 1920x1080, just adjust crop to 16:9
        if (imgAspect >= aspect) {
            targetW = static_cast<int>(std::round(h * aspect));
            targetH = h;
        } else {
            targetH = static_cast<int>(std::round(w / aspect));
            targetW = w;
        }
    }

    // Center crop coordinates
    int offX = std::max(0, (w - targetW) / 2);
    int offY = std::max(0, (h - targetH) / 2);

    std::vector<unsigned char> cropped(targetW * targetH * in_channels);

    for (int y = 0; y < targetH; ++y) {
        unsigned char* dst_row = cropped.data() + y * targetW * in_channels;
        unsigned char* src_row = img + ((offY + y) * w + offX) * in_channels;
        memcpy(dst_row, src_row, static_cast<size_t>(targetW * in_channels));
    }

    stbi_image_free(img);

    // If the cropped size exceeds 1920x1080, downscale to fit
    if (targetW > maxW || targetH > maxH) {
        int finalW = std::min(targetW, maxW);
        int finalH = std::min(targetH, maxH);
        std::vector<unsigned char> finalImg(finalW * finalH * in_channels);

        stbir_resize(
            cropped.data(), targetW, targetH, targetW * in_channels,
            finalImg.data(), finalW, finalH, finalW * in_channels,
            STBIR_RGBA,
            STBIR_TYPE_UINT8,
            STBIR_EDGE_CLAMP,
            STBIR_FILTER_DEFAULT
        );

        targetW = finalW;
        targetH = finalH;
        cropped.swap(finalImg);
    }

    int write_ok = stbi_write_png(outputPath.c_str(), targetW, targetH, in_channels, cropped.data(), targetW * in_channels);
    return write_ok != 0;
}


bool Utilities::validateString(std::string &value) {
    static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ ()_.\\-;,]+$");
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


std::string Utilities::browseImage() {
    OPENFILENAME ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    // Accept many image file types
    ofn.lpstrFilter =
        "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.tif;*.tiff;*.webp\0"
        "All Files\0*.*\0";

    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }

    return "";
}

bool Utilities::convertToPng(const std::string& src, const std::string& dst) {
    int w, h, channels;
    stbi_uc* data = stbi_load(src.c_str(), &w, &h, &channels, 4);  // always RGBA

    if (!data) {
        return false;
    }

    int success = stbi_write_png(dst.c_str(), w, h, 4, data, w * 4);
    stbi_image_free(data);

    return success != 0;
}

Rml::Element* getEl(const std::string& str) {
    if (Rml::Element* temp = getWindow().document->GetElementById(str))
        return temp;
    return nullptr;
}
