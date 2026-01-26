#include <fstream>
#include <nlohmann/json.hpp>

#include "ResourceHandler.h"

#include "EventListener.h"
#include "Utilities.h"

#include "global.h"

using json = nlohmann::json;

ResourceManager ResourceHandler::rm;

bool ResourceHandler::initResources() {
    std::filesystem::create_directories(getRelativeImagePath());
    std::filesystem::create_directories(getRelativeVideoPath());
    std::filesystem::create_directories(getRelativeThumbnailPath());

    if (!std::filesystem::exists(save_data.path / save_data.name / "resourceData.json"))
        std::ofstream file(save_data.path / save_data.name / "resourceData.json");

    if (!loadResources()) {
        return false;
    }
    if (!verifyResources()) {
        return false;
    }

    return true;
}

bool ResourceHandler::loadResources() {
    const std::filesystem::path filePath = save_data.path / save_data.name / "resourceData.json";
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << filePath.string() << std::endl;
        Utilities::showError("Datei " + filePath.string() + " konnte nicht geöffnet werden!");
        return false;
    }

    if (std::filesystem::file_size(filePath) == 0) {
        rm.maxId = 0;
        rm.resources.clear();
        return true;
    }

    json j;
    file >> j;
    rm = j.get<ResourceManager>();
    return true;
}

bool ResourceHandler::saveResources() {
    const std::filesystem::path filePath = save_data.path / save_data.name / "resourceData.json";
    std::ofstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << filePath.string() << std::endl;
        Utilities::showError("Datei " + filePath.string() + " konnte nicht geöffnet werden!");
        return false;
    }

    json j = rm;
    file << j.dump(4);

    return true;
}

Resource& ResourceHandler::createResource(std::filesystem::path path, std::string name)
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Resource file does not exist: " + path.string());
    }

    Resource r;
    r.id = rm.maxId++;
    r.name = name;
    r.thumbnail_id = -1;

    const std::string ext = getFileExtension(path.string());

    try {
        if (ext == ".mp4") {
            r.isVideo = true;

            r.path = getRelativeVideoPath() / ("video_mp4_" + std::to_string(rm.maxId) + ".mp4");

            std::filesystem::create_directories(r.path.parent_path());
            std::filesystem::copy_file(
                path,
                r.path,
                std::filesystem::copy_options::overwrite_existing
            );

            const auto thumbPath = getRelativeThumbnailPath() / ("image_png_tumbnail" + std::to_string(rm.maxId) + ".png");
            std::filesystem::create_directories(thumbPath.parent_path());

            if (Utilities::extractMp4Thumbnail(path.string(), thumbPath.string())) {
                Resource thumb;
                thumb.id = rm.maxId++;
                thumb.name = name + "_thumbnail";
                thumb.path = thumbPath;
                thumb.isVideo = false;
                thumb.thumbnail_id = -1;

                rm.resources.push_back(thumb);
                r.thumbnail_id = thumb.id;
            }
            else {
                Utilities::showError("Thumbnail could not be generated for " + path.string());
            }
        }
        else {
            r.isVideo = false;

            r.path = getRelativeImagePath() / ("image_png_" + std::to_string(rm.maxId) + ".png");

            std::filesystem::create_directories(r.path.parent_path());

            if (!Utilities::convertToPng(path.string(), r.path.string())) {
                throw std::runtime_error("Image conversion failed: " + path.string());
            }
        }

        rm.resources.push_back(r);
        saveResources();
        return rm.resources.back();
    }
    catch (const std::exception& e) {
        std::cerr << "[ResourceHandler] " << e.what() << std::endl;
        throw;
    }
}

void ResourceHandler::deleteResource(int id)
{
    auto& resources = rm.resources;

    for (auto& res : resources) {
        if (res.id == id)
            if (res.isVideo)
                deleteResource(res.thumbnail_id);
    }
    resources.erase(
        std::remove_if(
            resources.begin(),
            resources.end(),
            [id](const Resource& r) {
                return r.id == id;
            }
        ),
        resources.end()
    );
    saveResources();
}

Resource& ResourceHandler::getResource(int id)
{
    for (auto& r : rm.resources) {
        if (r.id == id)
            return r;
    }
    throw std::runtime_error("Resource not found: id=" + std::to_string(id));
}

std::vector<Resource> ResourceHandler::getResources() {
    return rm.resources;
}

int ResourceHandler::createSplitResource(int sourceId, float start, float end, const std::string& nameSuffix) {
    Resource& src = getResource(sourceId);

    if (!std::filesystem::exists(src.path))
        throw std::runtime_error("Source resource missing: " + src.path.string());

    std::string ext = getFileExtension(src.path.string());
    if (ext != ".png") // Only handle images for cropping
        throw std::runtime_error("Splits only supported for images");

    // Build path for split image
    std::filesystem::path splitPath = getRelativeImagePath() / (src.name + "_" + nameSuffix + ".png");
    std::filesystem::create_directories(splitPath.parent_path());

    // Crop the image
    if (!Utilities::cropImagePart(start, end, src.path.string(), splitPath.string()))
        throw std::runtime_error("Failed to crop image: " + src.path.string());

    // Register as a resource
    Resource r;
    r.id = rm.maxId++;
    r.name = src.name + "_" + nameSuffix;
    r.path = splitPath;
    r.isVideo = false;
    r.thumbnail_id = -1;

    rm.resources.push_back(r);
    saveResources();

    return r.id;
}



int ResourceHandler::getResourceIdByPath(const std::string& path) {
    for (auto& r : rm.resources) {
        if (r.path == path)
            return r.id;
    }
    throw std::runtime_error("Resource not found for path: " + path);
}

bool ResourceHandler::verifyResources() {
    for(auto& resource : rm.resources) {
        if (!std::filesystem::exists(resource.path)) {
            //std::string path = requestMissingResource(resource.id);
            //if (path.empty()) { // no new path given
            deleteResource(resource.id);
            //} else { // new path given
            //    resource.path = path;
            //}
        }
    }
    return true;
}

std::filesystem::path ResourceHandler::getRelativeImagePath() {
    return save_data.path / save_data.name / "resources" / "images";
}

std::filesystem::path ResourceHandler::getRelativeVideoPath() {
    return save_data.path / save_data.name / "resources" / "videos";
}

std::filesystem::path ResourceHandler::getRelativeThumbnailPath() {
    return save_data.path / save_data.name / "resources" / "videos" / "thumbnails";
}

std::string ResourceHandler::getFileExtension(std::string in) {
    return std::filesystem::path(in).extension().string(); // return like ".png"
}
