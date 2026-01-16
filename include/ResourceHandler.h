#pragma once

#include <string>
#include <vector>

#include "global.h"

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
private:
    static ResourceManager rm;

    static bool verifyResources();

    static std::filesystem::path getRelativeImagePath();
    static std::filesystem::path getRelativeVideoPath();
    static std::filesystem::path getRelativeThumbnailPath();
    static std::string getFileExtension(std::string in);
};

