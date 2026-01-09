#pragma once

#include "global.h"

using json = nlohmann::json;

// JSON serialization declarations
void to_json(json &j, const SaveData &d);
void to_json(json &j, const SceneData &s);
void to_json(json &j, const SceneManager &m);
void from_json(const json &j, SaveData &d);
void from_json(const json &j, SceneData &s);
void from_json(const json &j, SceneManager &m);

class Utilities {
public:
    static std::string browseFolder();
    static std::string browseTCTFile();
    static std::string browseImage();
    static bool convertToPng(const std::string& src, const std::string& dst);
    static std::string getExecutablePath();
    static std::string getSaveFolderPath();
    static std::string toBackwardSlashes(const std::string &path);
    static std::filesystem::path getRecentPath();
    static bool downscaleAndCrop169(const std::string& inputPath, const std::string& outputPath);
    static bool cropImagePart(float start, float end, const std::string& inputPath, const std::string& outputPath);

    static void showPopup(const std::string& msg, bool isError = false);
    static bool validateString(std::string &value);
    static void showError(const std::string & msg);
    static void showInfo(std::string msg);
};

Rml::Element* getEl(const std::string &str);
