#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct Resource {
    int id;
    std::filesystem::path path;
    std::string name;

    bool isVideo;
    int thumbnail_id;
};

struct ResourceManager {
    std::vector<Resource> resources;
    int maxId;
};

class ResourceHandler {
public:
    static bool initResources();

    static bool loadResources();
    static bool saveResources();
    static Resource& createResource(std::filesystem::path path, std::string name);
    //static std::string requestMissingResource(int id);
    static void deleteResource(int id);
    static Resource& getResource(int id);

    static std::vector<Resource> getResources();

    static int createSplitResource(int sourceId, float start, float end, const std::string& nameSuffix);

    static int getResourceIdByPath(const std::string &path);
    static std::string getFileExtension(std::string in);

private:
    static ResourceManager rm;

    static bool verifyResources();

    static std::filesystem::path getRelativeImagePath();
    static std::filesystem::path getRelativeVideoPath();
    static std::filesystem::path getRelativeThumbnailPath();
};

