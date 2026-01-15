#include "Utilities.h"

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "EventListener.h"
#include "stb_image_resize2.h"
#define STBIW_ZLIB_COMPRESS_LEVEL 1

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// JSON serialization implementations
void to_json(json &j, const SaveData &d) {
    j = json{
            {"projectName", d.projectName},
            {"projectorCount", d.projectorCount},
            {"description", d.description},
            {"path", d.path},
            {"version", d.version}
    };
}

void to_json(json &j, const SceneData &s) {
    j = json{
        {"sceneName", s.sceneName},
        {"sources", s.sources},
        {"splitSources", s.splitSources},
        {"connections", s.connection}
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
    j.at("version").get_to(d.version);
}

void from_json(const json &j, SceneData &s) {
    j.at("sceneName").get_to(s.sceneName);
    j.at("sources").get_to(s.sources);
    j.at("splitSources").get_to(s.splitSources);
    j.at("connections").get_to(s.connection);
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
bool Utilities::cropImagePart(
    float start,
    float end,
    const std::string& inputPath,
    const std::string& outputPath
) {
    if (start < 0.f || end > 1.f || start >= end)
        return false;

    int srcW, srcH, _;
    unsigned char* src = stbi_load(inputPath.c_str(), &srcW, &srcH, &_, 4);
    if (!src)
        return false;

    constexpr int channels = 4;

    const int cropX0 = static_cast<int>(srcW * start);
    const int cropX1 = static_cast<int>(srcW * end);
    const int cropW  = cropX1 - cropX0;

    if (cropW <= 0) {
        stbi_image_free(src);
        return false;
    }

    const int outW = cropW;
    const int outH = static_cast<int>(cropW * 9.f / 16.f);

    std::vector<unsigned char> out(outW * outH * channels);

    for (int y = 0; y < outH; ++y) {
        const int srcY = std::min(
            static_cast<int>((float)y * srcH / outH),
            srcH - 1
        );

        const unsigned char* srcRow =
            src + (srcY * srcW + cropX0) * channels;

        unsigned char* dstRow =
            out.data() + y * outW * channels;

        memcpy(dstRow, srcRow, outW * channels);
    }

    stbi_image_free(src);

    // FAST PNG write
    stbi_write_png_compression_level = 1;

    const bool ok = stbi_write_png(
        outputPath.c_str(),
        outW,
        outH,
        channels,
        out.data(),
        outW * channels
    );

    return ok;
}

bool Utilities::extractMp4Thumbnail(const std::string &videoPath, const std::string &outPngPath) {

    AVFormatContext* fmt = nullptr;

    if (avformat_open_input(&fmt, videoPath.c_str(), nullptr, nullptr) < 0)
        return false;

    if (avformat_find_stream_info(fmt, nullptr) < 0)
        return false;

    int videoStream = -1;
    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
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

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgb = av_frame_alloc();

    SwsContext* sws = sws_getContext(
        ctx->width, ctx->height, ctx->pix_fmt,
        ctx->width, ctx->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    int bufSize = av_image_get_buffer_size(
        AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1
    );
    uint8_t* buffer = (uint8_t*)av_malloc(bufSize);
    av_image_fill_arrays(rgb->data, rgb->linesize, buffer,
                          AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1);

    bool gotFrame = false;

    while (av_read_frame(fmt, packet) >= 0 && !gotFrame) {
        if (packet->stream_index == videoStream) {
            if (avcodec_send_packet(ctx, packet) == 0) {
                if (avcodec_receive_frame(ctx, frame) == 0) {
                    sws_scale(
                        sws,
                        frame->data, frame->linesize,
                        0, ctx->height,
                        rgb->data, rgb->linesize
                    );

                    // save RGB → PNG
                    saveRgbToPng(
                        rgb->data[0],
                        ctx->width,
                        ctx->height,
                        outPngPath
                    );

                    gotFrame = true;
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

bool Utilities::saveRgbToPng(const unsigned char *rgbData, int width, int height, const std::string &outPath) {
    if (!rgbData || width <= 0 || height <= 0) {
        std::cerr << "saveRgbToPng: invalid input\n";
        return false;
    }

    try {
        std::filesystem::create_directories(
            std::filesystem::path(outPath).parent_path()
        );
    } catch (...) {
        std::cerr << "saveRgbToPng: failed to create directories\n";
        return false;
    }

    // stride = width * RGB
    const int stride = width * 3;

    int result = stbi_write_png(
        outPath.c_str(),
        width,
        height,
        3,              // RGB
        rgbData,
        stride
    );

    if (!result) {
        std::cerr << "saveRgbToPng: failed to write " << outPath << "\n";
        return false;
    }

    return true;
}


bool Utilities::validateString(std::string &value) {
    if (value.empty()) {
        return true; // allow empty strings
    }

    static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ ()_.\\-;,]+$");
    if (std::regex_match(value, pattern)) {
        return true;  // input is valid
    }
    return false;     // invalid input
}

void Utilities::showPopup(const std::string& msg, bool isError) {
    using namespace Rml;

    ElementDocument* doc = getWindow().document;
    if (!doc) return;

    // Create or get container
    Element* container = doc->GetElementById("toast-container");
    if (!container) {
        ElementPtr new_container = doc->CreateElement("div");
        new_container->SetId("toast-container");
        new_container->SetAttribute("class", "toast-container");
        container = new_container.get();
        doc->GetElementById("body")->AppendChild(std::move(new_container));
    }

    // Create new toast
    ElementPtr toast = doc->CreateElement("div");
    toast->SetAttribute("class", isError ? "toast toast-error" : "toast");
    toast->SetInnerRML(msg.c_str());

    Element* toast_raw = toast.get(); // raw pointer for callbacks
    container->AppendChild(std::move(toast)); // transfer ownership

    // Click to remove
    toast_raw->AddEventListener(EventId::Click, new ButtonHandler([toast_raw] {
        if (Element* parent = toast_raw->GetParentNode())
            parent->RemoveChild(toast_raw);
    }));

    // Auto-remove after 10s
    std::thread([toast_raw] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (Element* parent = toast_raw->GetParentNode())
            parent->RemoveChild(toast_raw);
    }).detach();
}


void Utilities::showError(const std::string & msg) {
    std::cerr << msg << std::endl;
    showPopup(msg, true);
}

void Utilities::showInfo(std::string msg) {
    std::clog << msg << std::endl;
    showPopup(msg);
}

bool Utilities::isImageExt(const std::filesystem::path &p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".webp";
}

bool Utilities::isMp4Ext(const std::filesystem::path &p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp4";
}

std::string Utilities::browseImageOrMp4() // also more than 1 image
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
