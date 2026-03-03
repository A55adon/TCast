#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

struct Resource {
    int id;
    fs::path path;
    std::string name;

    bool is_video;
    int thumbnail_id;
};

struct ResourceManager {
    std::vector<Resource> resources;
    int max_id;
};

class ResourceHandler {
public:
    static bool initResources();
    static bool loadResources();
    static bool saveResources();

    static Resource& createResource(const fs::path& path, const std::string &name);
    static void deleteResource(int id);

    static Resource& getResource(int id);
    static std::vector<Resource>& getResources();
    static int getResourceIdByPath(const fs::path &path);
    static std::string getFileExtension(std::string in);

    static int createSplitResource(int source_id, float start, float end, const std::string& name_suffix);

private:
    static ResourceManager resourceManager;

    static bool verifyResources();
    static bool deleteMissingResources();

    static fs::path getRelativeImagePath();
    static fs::path getRelativeVideoPath();
    static fs::path getRelativeThumbnailPath();
    static fs::path getRelativeResourceData();
};

