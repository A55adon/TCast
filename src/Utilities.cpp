#include "Utilities.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ZLIB_COMPRESS_LEVEL 1

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"
#include "EventListener.h"
#include <windows.h>
#include <vector>
#include <future>
#include <png.h>
#include <turbojpeg.h>


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// JSON serialization implementations
void to_json(json &j, const SaveData &d) {
    j = json{
            {"projectName", d.name},
            {"projectorCount", d.projector_amount},
            {"description", d.description},
            {"path", d.path},
            {"version", d.version}
    };
}

void to_json(json &j, const SceneData &s) {
    j = json{
        {"sceneName", s.name},
        {"sources", s.sources},
        {"splitSources", s.split_sources},
        {"connections", s.connections}
    };
}

void to_json(json &j, const SceneManager &m) {
    j = json{
            {"scenes", m.scenes}
    };
}

void to_json(json& j, const Resource& r)
{
    j = json{
            {"id", r.id},
            {"path", r.path},
            {"name", r.name},
            {"isVideo", r.is_video},
            {"thumbnail_id", r.thumbnail_id}
    };
}

void to_json(json &j, const ResourceManager &rm) {
    j = json{
        {"maxId", rm.max_id},
        {"resources", rm.resources}
    };
}

void from_json(const json &j, SaveData &d) {
    j.at("projectName").get_to(d.name);
    j.at("projectorCount").get_to(d.projector_amount);
    j.at("description").get_to(d.description);
    j.at("path").get_to(d.path);
    j.at("version").get_to(d.version);
}

void from_json(const json &j, SceneData &s) {
    j.at("sceneName").get_to(s.name);
    j.at("sources").get_to(s.sources);
    j.at("splitSources").get_to(s.split_sources);
    j.at("connections").get_to(s.connections);
}

void from_json(const json &j, SceneManager &m) {
    j.at("scenes").get_to(m.scenes);
}

void from_json(const json &j, Resource &r) {
    j.at("id").get_to(r.id);
    j.at("path").get_to(r.path);
    j.at("name").get_to(r.name);
    j.at("isVideo").get_to(r.is_video);
    j.at("thumbnail_id").get_to(r.thumbnail_id);
}

void from_json(const json &j, ResourceManager &rm) {
    j.at("maxId").get_to(rm.max_id);
    j.at("resources").get_to(rm.resources);
}

// Enter resource path and get the Resource id
int Utilities::getResIDFromPath(const fs::path &path) {
    return RH::getResourceIdByPath(path.string());
}
// Enter resource id and get the Resource file path
fs::path Utilities::getResPathFromID(const int id) {
    return RH::getResource(id).path;
}
// Enter resource id and get a pointer to that resource
Resource * Utilities::getResFromID(const int id) {
    return &RH::getResource(id);
}

// Opens an explorer window and returns path of selected resource
fs::path Utilities::browseFolder() {
    fs::path result;
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
                    // Convert wide string to UTF-8 string for fs::path
                    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0, nullptr, nullptr);
                    if (bufferSize > 0) {
                        std::string buffer(bufferSize, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &buffer[0], bufferSize, nullptr, nullptr);
                        // Remove null terminator from the string
                        buffer.resize(bufferSize - 1);
                        result = buffer;
                    }
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

// Opens explorer window to search for *.tct project files
fs::path Utilities::browseTCTFile() {
    fs::path result;
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
                    // Get the required buffer size for UTF-8 conversion
                    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0, nullptr, nullptr);
                    if (bufferSize > 0) {
                        std::string buffer(bufferSize, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &buffer[0], bufferSize, nullptr, nullptr);
                        // Remove null terminator from the string
                        buffer.resize(bufferSize - 1);
                        result = buffer;
                    }
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

// Returns path to executable
fs::path Utilities::getExecutablePath() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        // Return empty path instead of empty string
        return fs::path();
    }
    return fs::path(buffer);
}

// Returns path of the saveFolder relative to executable
fs::path Utilities::getSaveFolderPath() {
    const fs::path exePath = getExecutablePath();

    // Check if executable path is valid
    if (exePath.empty()) {
        return fs::path(); // Return empty path on error
    }

    const fs::path exeDir = exePath.parent_path(); // .../cmake-build-debug (build directory)
    const fs::path projectDir = exeDir.parent_path(); // .../TCast
    const fs::path savePath = projectDir / "saves" / "folderSaves";

    // make sure directory exists
    if (!fs::exists(savePath)) {
        fs::create_directories(savePath);
    }
    return savePath;
}

// Returns input path whilst replacing all / with backslash
fs::path Utilities::toBackwardSlashes(const fs::path& path) {
    std::string fixed = path.string();
    std::replace(fixed.begin(), fixed.end(), '/', '\\');
    return fs::path(fixed);
}

// Returns the content of recent.path from /saves if it exists, else returns ""
fs::path Utilities::getRecentPath() {
    static const fs::path recentFilePath = "../saves/recent.path";

    // Check if recent.path exists
    if (!fs::exists(recentFilePath)) {
        return fs::path{};
    }

    // Read the path from recent.path
    std::ifstream file(recentFilePath);
    if (!file.is_open()) {
        LOG_ERR("Utilities","Unable to open recent file");
        return fs::path{};
    }

    std::string line;
    std::getline(file, line);
    file.close();

    // Trim whitespace
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return fs::path{};
    }
    const auto last = line.find_last_not_of(" \t\r\n");
    const std::string pathStr = line.substr(first, last - first + 1);

    fs::path result{ pathStr };
    if (!fs::exists(result)) {
        std::string msg = "Path [" + pathStr + "] does not exist - going to startup";
        LOG_ERR("Utilities", msg);
        return fs::path{};
    }
    std::string msg = "Fount recent path [" + pathStr + "]";
    LOG_INFO("Utilities", msg);
    return result;
}

// Copies an input image, turns it to 16:9 format, saves it in the output Path
bool Utilities::downscaleAndCrop169(const fs::path& inputPath,
                                    const fs::path& outputPath)
{
    const auto t0 = std::chrono::high_resolution_clock::now();

    int w, h, channels;

    unsigned char* img = stbi_load(
        inputPath.string().c_str(),
        &w, &h, &channels,
        0
    );

    if (!img)
        return false;

    constexpr int maxW = 1920;
    constexpr int maxH = 1080;
    constexpr float aspect = 16.0f / 9.0f;

    const float imgAspect = static_cast<float>(w) / h;

    int cropW = w;
    int cropH = h;

    if (imgAspect > aspect)
        cropW = static_cast<int>(h * aspect);
    else
        cropH = static_cast<int>(w / aspect);

    const int offX = (w - cropW) / 2;
    const int offY = (h - cropH) / 2;

    const bool needResize = cropW > maxW || cropH > maxH;

    int finalW = cropW;
    int finalH = cropH;

    if (needResize)
    {
        if ((float)cropW / cropH > aspect)
        {
            finalW = maxW;
            finalH = static_cast<int>(finalW / aspect);
        }
        else
        {
            finalH = maxH;
            finalW = static_cast<int>(finalH * aspect);
        }
    }

    std::vector<unsigned char> output(finalW * finalH * channels);

    const unsigned char* croppedStart =
        img + (offY * w + offX) * channels;

    if (!needResize)
    {
        for (int y = 0; y < cropH; ++y)
        {
            memcpy(
                output.data() + y * cropW * channels,
                croppedStart + y * w * channels,
                cropW * channels
            );
        }
    }
    else
    {
        stbir_resize(
            croppedStart,
            cropW, cropH, w * channels,
            output.data(),
            finalW, finalH, finalW * channels,
            channels == 4 ? STBIR_RGBA : STBIR_RGB,
            STBIR_TYPE_UINT8,
            STBIR_EDGE_CLAMP,
            STBIR_FILTER_BOX
        );
    }

    stbi_image_free(img);

    stbi_write_png_compression_level = 0;

    const int ok = stbi_write_png(
        outputPath.string().c_str(),
        finalW,
        finalH,
        channels,
        output.data(),
        finalW * channels
    );

    const auto t1 = std::chrono::high_resolution_clock::now();
    const double ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "downscaleAndCrop169 finished in "
              << ms
              << " ms\n";

    return ok != 0;
}
// Copies an input image, cuts it of so it starts on x-axis start percentage to end percentage and saves it in the output path
bool Utilities::cropImagePart(float start, float end,
                              const fs::path& inputPath,
                              const fs::path& outputPath)
{
    if (start < 0.f || end > 1.f || start >= end)
        return false;

    int srcW, srcH, channels;

    unsigned char* src = stbi_load(
        inputPath.string().c_str(),
        &srcW, &srcH, &channels,
        0
    );

    if (!src)
        return false;

    const int cropX0 = int(srcW * start);
    const int cropX1 = int(srcW * end);
    const int cropW  = cropX1 - cropX0;

    if (cropW <= 0) {
        stbi_image_free(src);
        return false;
    }

    const int outW = cropW;
    const int outH = int(outW * 9.f / 16.f);

    // Only use needed source height
    const int cropH = std::min(srcH, int(cropW * 9.f / 16.f));
    const int offY  = (srcH - cropH) / 2;

    std::vector<unsigned char> out(outW * outH * channels);

    stbir_resize(
        src + (offY * srcW + cropX0) * channels,
        cropW,
        cropH,
        srcW * channels,
        out.data(),
        outW,
        outH,
        outW * channels,
        channels == 4 ? STBIR_RGBA : STBIR_RGB,
        STBIR_TYPE_UINT8,
        STBIR_EDGE_CLAMP,
        STBIR_FILTER_BOX
    );

    stbi_image_free(src);

    stbi_write_png_compression_level = 0;

    return stbi_write_png(
        outputPath.string().c_str(),
        outW,
        outH,
        channels,
        out.data(),
        outW * channels
    ) != 0;
}
// Takes the first frame from the video path and saves it as png in output path
bool Utilities::extractMp4Thumbnail(const fs::path& videoPath,
                                     const fs::path& outPngPath)
{
    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, videoPath.string().c_str(), nullptr, nullptr) < 0)
        return false;

    if (avformat_find_stream_info(fmt, nullptr) < 0)
        return false;

    int videoStream = -1;
    for (unsigned i = 0; i < fmt->nb_streams; ++i)
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }

    if (videoStream == -1)
        return false;

    AVCodecParameters* params = fmt->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (!codec)
        return false;

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx, params);
    avcodec_open2(ctx, codec, nullptr);

    av_seek_frame(fmt, videoStream, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(ctx);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgb   = av_frame_alloc();

    SwsContext* sws = sws_getContext(
        ctx->width, ctx->height, ctx->pix_fmt,
        ctx->width, ctx->height, AV_PIX_FMT_RGB24,
        SWS_FAST_BILINEAR,
        nullptr, nullptr, nullptr
    );

    const int bufSize = av_image_get_buffer_size(
        AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1
    );

    uint8_t* buffer = (uint8_t*)av_malloc(bufSize);
    av_image_fill_arrays(
        rgb->data, rgb->linesize,
        buffer,
        AV_PIX_FMT_RGB24,
        ctx->width, ctx->height,
        1
    );

    bool gotFrame = false;

    while (av_read_frame(fmt, packet) >= 0 && !gotFrame)
    {
        if (packet->stream_index == videoStream)
        {
            if (avcodec_send_packet(ctx, packet) == 0)
            {
                while (avcodec_receive_frame(ctx, frame) == 0)
                {
                    sws_scale(
                        sws,
                        frame->data,
                        frame->linesize,
                        0,
                        ctx->height,
                        rgb->data,
                        rgb->linesize
                    );

                    stbi_write_png_compression_level = 0;

                    stbi_write_png(
                        outPngPath.string().c_str(),
                        ctx->width,
                        ctx->height,
                        3,
                        rgb->data[0],
                        rgb->linesize[0]
                    );

                    gotFrame = true;
                    break;
                }
            }
        }
        av_packet_unref(packet);
    }

    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&rgb);
    av_packet_free(&packet);
    sws_freeContext(sws);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);

    return gotFrame;
}

// Turns rgb input data into a image
bool Utilities::saveRgbToPng(const unsigned char *rgbData, const int width, const int height, const fs::path &outPath) {
    if (!rgbData || width <= 0 || height <= 0) {
        LOG_ERR("Utilities", "invalid input in saveRgbToPng()");
        return false;
    }

    try {
        std::filesystem::create_directories(
            std::filesystem::path(outPath).parent_path()
        );
    } catch (...) {
        LOG_ERR("Utilities", "failed to create directories in saveRgbToPng()");
        return false;
    }

    // stride = width * RGB
    const int stride = width * 3;

    int result = stbi_write_png(
        outPath.string().c_str(),
        width,
        height,
        3,              // RGB
        rgbData,
        stride
    );

    if (!result) {
        LOG_ERR("Utilities", "invalid image result in saveRgbToPng()");
        return false;
    }

    return true;
}

// Checks a input string for "^[A-Za-z0-9äöüÄÖÜ ()_.\\-;,\\[\\]&]+$"
bool Utilities::validateString(const std::string &value) {
    if (value.empty()) {
        return true; // allow empty strings
    }

    static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ ()_.\\-;,\\[\\]&]+$");
    if (std::regex_match(value, pattern)) {
        return true;  // input is valid
    }
    return false;     // invalid input
}

// Creates a toast container popup
void Utilities::showPopup(const std::string& msg, const bool isError) {

    std::cout << msg << std::endl;

    Rml::ElementDocument* doc = getWindow().document;
    if (!doc) {
        LOG_ERR("Utilities", "failed to get window document in showPopup()");
        return;
    }

    // Create or get container
    Rml::Element* container = getEl("toast-container");
    if (!container) {
        Rml::ElementPtr new_container = doc->CreateElement("div");
        new_container->SetId("toast-container");
        new_container->SetAttribute("class", "toast-container");
        container = new_container.get();
        auto el = getEl("body");
        if (!el)
            return;
        el->AppendChild(std::move(new_container));

    }

    // Create new toast
    Rml::ElementPtr toast = doc->CreateElement("div");
    toast->SetAttribute("class", isError ? "toast toast-error" : "toast");
    toast->SetInnerRML(msg.c_str());

    Rml::Element* toast_raw = toast.get(); // raw pointer for callbacks
    container->AppendChild(std::move(toast)); // transfer ownership

    // Click to remove
    toast_raw->AddEventListener(Rml::EventId::Click, new ButtonHandler([toast_raw] {
        if (Rml::Element* parent = toast_raw->GetParentNode())
            parent->RemoveChild(toast_raw);
    }));

    // Auto-remove after 10s
    std::thread([toast_raw] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (Rml::Element* parent = toast_raw->GetParentNode())
            parent->RemoveChild(toast_raw);
    }).detach();
}

// Creates a Popup error
void Utilities::showError(const std::string & msg) {
    LOG_ERR("Popup", msg);
    showPopup(msg, true);
}

// Creates a Popup info
void Utilities::showInfo(const std::string &msg) {
    LOG_INFO("Popup", msg);
    showPopup(msg);
}

// Checks if entered path has a image extension
bool Utilities::isImageExt(const fs::path &p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".webp";
}

// Checks if extension is Mp4
bool Utilities::isMp4Ext(const fs::path &p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp4";
}
/*
// Gets a Element from its id string
Rml::Element *Utilities::getElement(const std::string& id) {
    if (getWindow().document->GetElementById(id)) return getWindow().document->GetElementById(id);
    LOG_INFO("Utilities", "Couldn't get Element with id[" + id + "]");
    return nullptr;
}
// Sets attributes like classes
void Utilities::setAttribute(const std::string &id, const std::string &attribute, const std::string &value) {
    getElement(id)->SetAttribute(attribute, value);
}
// Sets the id of a obj
void Utilities::setId(const std::string &id, const std::string &new_id) {
    getElement(id)->SetId(new_id);
}
// Sets inner html content
void Utilities::setContent(const std::string &id, const std::string &content) {
    getElement(id)->SetInnerRML(content);
}
// Creates new element
Rml::Element* Utilities::createElement(const std::string& element) {
    return getWindow().document->CreateElement(element).get();
}
// Adds a child to a given parent
void Utilities::addChild(const std::string& parent_id, const std::string &child_id) {
    getElement(parent_id)->AppendChild(Rml::ElementPtr(getElement(child_id)));
}

template<typename F>
void Utilities::addEventListener(std::string id, F &&ev) {
}
*/
// Opens browser and allows selection of 1 or multiple Images or MP4 files
std::string Utilities::browseImageOrMp4() // Also more than 1 image
{
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return "";

    IFileOpenDialog* dialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&dialog));
    if (FAILED(hr)) {
        CoUninitialize();
        return "";
    }

    // Allow multiple selection
    DWORD options;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM);

    // Set filter for images
    COMDLG_FILTERSPEC filters[] = {
        { L"Image Files", L"*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.tif;*.tiff;*.webp;*.mp4" },
        { L"All Files", L"*.*" }
    };
    dialog->SetFileTypes(_countof(filters), filters);
    dialog->SetFileTypeIndex(1);

    // Show dialog
    hr = dialog->Show(nullptr);
    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return "";
    }

    // Get result items
    IShellItemArray* items = nullptr;
    hr = dialog->GetResults(&items);
    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return "";
    }

    DWORD count = 0;
    items->GetCount(&count);

    std::vector<std::string> paths;
    paths.reserve(count);

    for (DWORD i = 0; i < count; ++i) {
        IShellItem* item = nullptr;
        items->GetItemAt(i, &item);

        PWSTR wpath = nullptr;
        item->GetDisplayName(SIGDN_FILESYSPATH, &wpath);

        if (wpath) {
            char pathUtf8[MAX_PATH * 4];

            // Convert UTF-16 → UTF-8
            int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1,
                                          pathUtf8, sizeof(pathUtf8),
                                          nullptr, nullptr);
            if (len > 0) {
                paths.emplace_back(pathUtf8);
            }

            CoTaskMemFree(wpath);
        }

        item->Release();
    }

    items->Release();
    dialog->Release();
    CoUninitialize();

    if (paths.empty())
        return "";

    // Join with commas
    std::string joined;
    for (size_t i = 0; i < paths.size(); i++) {
        if (i > 0) joined += ",";
        joined += paths[i];
    }
    return joined;
}

static thread_local std::vector<unsigned char> tls_fileBuffer;

bool Utilities::convertToPng(const fs::path& src, const fs::path& dst)
{
    // --- Use CreateFile for direct Windows API access (fastest) ---
    HANDLE hFile = CreateFileW(
        src.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) return false;

    // Get file size
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);

    if (fileSize.QuadPart == 0) {
        CloseHandle(hFile);
        return false;
    }

    // Reuse thread-local buffer to avoid allocations
    tls_fileBuffer.resize(fileSize.QuadPart);

    DWORD bytesRead;
    BOOL readResult = ReadFile(hFile, tls_fileBuffer.data(), fileSize.QuadPart, &bytesRead, NULL);
    CloseHandle(hFile);

    if (!readResult || bytesRead != fileSize.QuadPart) return false;

    // --- Decode image ---
    int w, h, channels;
    stbi_uc* data = stbi_load_from_memory(
        tls_fileBuffer.data(),
        fileSize.QuadPart,
        &w, &h, &channels,
        0
    );

    if (!data) return false;

    // --- Fastest PNG write ---
    stbi_write_png_compression_level = 0; // No compression

    // Use direct write with no compression
    int success = stbi_write_png(
        dst.string().c_str(),
        w, h, channels,
        data,
        w * channels
    );

    stbi_image_free(data);
    return success != 0;
}

// Batch processing with controlled parallelism
void Utilities::convertMultipleToPng(const std::vector<std::pair<fs::path, fs::path>>& jobs)
{
    // Get optimal number of threads
    const size_t numThreads = std::thread::hardware_concurrency();

    // Split jobs among threads
    std::vector<std::future<void>> futures;
    futures.reserve(numThreads);

    size_t jobsPerThread = (jobs.size() + numThreads - 1) / numThreads;

    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * jobsPerThread;
        size_t end = std::min(start + jobsPerThread, jobs.size());

        if (start >= jobs.size()) break;

        futures.push_back(std::async(std::launch::async, [&jobs, start, end]() {
            for (size_t i = start; i < end; ++i) {
                convertToPng(jobs[i].first, jobs[i].second);
            }
        }));
    }

    // Wait for all threads
    for (auto& future : futures) {
        future.wait();
    }
}

// Gets a Rml::Element from ID
Rml::Element* getEl(const std::string& str) {
    if (Rml::Element* temp = getWindow().document->GetElementById(str))
        return temp;

    LOG_ERR("Utilities", "Could not find element: " + str);
    return nullptr;
}
