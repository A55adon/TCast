#pragma once

#include "global.h"

using json = nlohmann::json;

// JSON serialization declarations
void to_json(json &j, const st_save_data &d);
void to_json(json &j, const st_scene_data &s);
void to_json(json &j, const st_scene_manager &m);
void to_json(json &j, const Resource &r);
void to_json(json &j, const ResourceManager &rm);

void from_json(const json &j, st_save_data &d);
void from_json(const json &j, st_scene_data &s);
void from_json(const json &j, st_scene_manager &m);
void from_json(const json &j, Resource &r);
void from_json(const json &j, ResourceManager &rm);

class Utilities {
public:
    static std::string browseFolder();
    static std::string browseTCTFile();
    static std::string browseImageOrMp4();
    static bool convertToPng(const std::string& src, const std::string& dst);
    static std::string getExecutablePath();
    static std::string getSaveFolderPath();
    static std::string toBackwardSlashes(const std::string &path);
    static std::filesystem::path getRecentPath();
    static bool downscaleAndCrop169(const std::string& inputPath, const std::string& outputPath);
    static bool cropImagePart(float start, float end, const std::string& inputPath, const std::string& outputPath);
    static bool extractMp4Thumbnail( const std::string& videoPath, const std::string& outPngPath);
    static bool saveRgbToPng(const unsigned char* rgbData, int width, int height, const std::string& outPath);

    static void showPopup(const std::string& msg, bool isError = false);
    static bool validateString(std::string &value);
    static void showError(const std::string & msg);
    static void showInfo(std::string msg);

    static bool isImageExt(const std::filesystem::path& p);
    static bool isMp4Ext(const std::filesystem::path& p);
};

Rml::Element* getEl(const std::string &str);
