#pragma once

#include "global.h"

using json = nlohmann::json;
namespace fs = std::filesystem;
using RH = ResourceHandler;

// JSON serialization declarations
void to_json(json &j, const SaveData &d);
void to_json(json &j, const SceneData &s);
void to_json(json &j, const SceneManager &m);
void to_json(json &j, const Resource &r);
void to_json(json &j, const ResourceManager &rm);

void from_json(const json &j, SaveData &d);
void from_json(const json &j, SceneData &s);
void from_json(const json &j, SceneManager &m);
void from_json(const json &j, Resource &r);
void from_json(const json &j, ResourceManager &rm);

class Utilities {
public:
    // simple ResourceHandler access
    static int getResIDFromPath(const fs::path &path);
    static fs::path getResPathFromID(int id);
    static Resource* getResFromID(int id);

    // File IO
    static fs::path browseFolder();
    static fs::path browseTCTFile();
    static std::string browseImageOrMp4();

    // Image conversion
    static void convertMultipleToPng(const std::vector<std::pair<fs::path, fs::path>>& jobs);
    static bool convertToPng(const fs::path& src, const fs::path& dst);
    static bool downscaleAndCrop169(const fs::path& inputPath, const fs::path& outputPath);
    static bool cropImagePart(float start, float end, const fs::path& inputPath, const fs::path& outputPath);
    static bool extractMp4Thumbnail(const fs::path& videoPath, const fs::path& outPngPath);
    static bool saveRgbToPng(const unsigned char* rgbData, int width, int height, const fs::path& outPath);

    // Getters
    static fs::path getExecutablePath();
    static fs::path getSaveFolderPath();
    static fs::path toBackwardSlashes(const fs::path &path);
    static fs::path getRecentPath();

    // Other
    static bool validateString(const std::string &value);
    static void showPopup(const std::string& msg, bool isError = false);
    static void showError(const std::string & msg);
    static void showInfo(const std::string &msg);

    // Verification
    static bool isImageExt(const fs::path& p);
    static bool isMp4Ext(const fs::path& p);

    // HTML/CSS parser interface functions
    //static Rml::Element *getElement(const std::string& id);
    //static void setAttribute(const std::string &id, const std::string &attribute, const std::string &value);
    //static void setId(const Rml::ElementPtr obj, const std::string &new_id);
    //static void setContent(const std::string &id, const std::string &content);
    //static Rml::ElementPtr createElement(const std::string &element);
    //static void addChild(const std::string &parent_id, const std::string &child_id);

    //template <typename F>
    //static void addEventListener(std::string id, F&& ev);
    /*
    template <typename F>
    void UIHelpers::addEventListener(const std::string& id, F&& ev)
    {
        if (Rml::Element* el = GetElementById(id)) {
            el->AddEventListener(
                Rml::EventId::Click,
                new ButtonHandler(std::forward<F>(ev))
            );
        }
    }

     */
};

// TODO: remove
Rml::Element* getEl(const std::string &str);
