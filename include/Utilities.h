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
    static std::string getExecutablePath();
    static std::string getSaveFolderPath();
    static std::string toBackwardSlashes(const std::string &path);
    static std::filesystem::path getRecentPath();

    static Rml::Element* getEl(const std::string &str);
};

