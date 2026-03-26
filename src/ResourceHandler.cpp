#include "ResourceHandler.h"
#include "Utilities.h"

#include "global.h"

using json = nlohmann::json;
using U = Utilities;

// Static variable initialization
ResourceManager ResourceHandler::resourceManager;

// Initializes Resource Handler
bool ResourceHandler::initResources() {
    fs::create_directories(getRelativeImagePath());
    fs::create_directories(getRelativeVideoPath());
    fs::create_directories(getRelativeThumbnailPath());

    if (!fs::exists(getRelativeResourceData()))
        std::ofstream file(getRelativeResourceData());

    if (!loadResources()) {
        LOG_ERR("RH", "Couldn't load resource data in initResources()");
        return false;
    }
    if (!verifyResources()) {
        LOG_INFO("RH", "Couldn't verify all resources -> removing missing ones in initResources()");
        deleteMissingResources();
        return false;
    }

    return true;
}

// Loads from resourceData.json to resourceManager
bool ResourceHandler::loadResources() {
    std::ifstream file(getRelativeResourceData());

    if (!file.is_open()) {
        const std::string msg =  "Could not open file[" + getRelativeResourceData().string() + "] in loadResources()";
        LOG_INFO("RH", msg);
        U::showError("Datei " + getRelativeResourceData().string() + " konnte nicht geöffnet werden!");
        return false;
    }

    if (fs::file_size(getRelativeResourceData()) == 0) {
        resourceManager.max_id = 0;
        resourceManager.resources.clear();
        return true;
    }

    json j;
    file >> j;
    resourceManager = j.get<ResourceManager>();
    return true;
}

// Saves resourceManager to resourceData.json
bool ResourceHandler::saveResources() {;
    std::ofstream file(getRelativeResourceData());

    if (!file.is_open()) {
        const std::string msg = "Couldn't open file[" + getRelativeResourceData().string() + "] in saveResources()";
        LOG_INFO("RH", msg);
        U::showError("Datei " + getRelativeResourceData().string() + " konnte nicht geöffnet werden!");
        return false;
    }

    const json j = resourceManager;
    file << j.dump(4);

    return true;
}

Resource& ResourceHandler::createResource(const fs::path& path, const std::string &name)
{
    if (!fs::exists(path)) {
        LOG_ERR("RH", "File[" + path.string() + "] doesn't exist");
        U::showError("Datei " + path.string() + " existiert nicht!");
    }

    Resource r;
    r.id = resourceManager.max_id++;
    r.name = name;
    r.thumbnail_id = -1;

    try {
        if (getFileExtension(path.string()) == ".mp4") {
            r.is_video = true;

            r.path = getRelativeVideoPath() / ("video_mp4_" + std::to_string(resourceManager.max_id) + ".mp4");

            fs::create_directories(r.path.parent_path());
            fs::copy_file(
                path,
                r.path,
                fs::copy_options::overwrite_existing
            );

            const fs::path thumbPath = getRelativeThumbnailPath() / ("image_png_thumbnail" + std::to_string(resourceManager.max_id) + ".png");
            fs::create_directories(thumbPath.parent_path());

            if (U::extractMp4Thumbnail(path.string(), thumbPath.string())) {
                Resource thumb;
                thumb.id = resourceManager.max_id++;
                thumb.name = name + "_thumbnail";
                thumb.path = thumbPath;
                thumb.is_video = false;
                thumb.thumbnail_id = -1;

                resourceManager.resources.push_back(thumb);
                r.thumbnail_id = thumb.id;
            }
            else {
                LOG_ERR("RH", "Thumbnail couldn't be generated for " + path.string() + " in createResource()");
                U::showError("Thumbnail konnte nicht erstellt werden für " + path.string());
            }
        }
        else {
            r.is_video = false;

            r.path = getRelativeImagePath() / ("image_png_" + std::to_string(resourceManager.max_id) + ".png");

            fs::create_directories(r.path.parent_path());

            // For single image, create a vector with one job and call batch function
            std::vector<std::pair<fs::path, fs::path>> jobs = {
                {path, r.path}
            };

            U::convertMultipleToPng(jobs);

            // Verify the conversion worked
            if (!fs::exists(r.path)) {
                LOG_ERR("RH", "Couldn't convert " + path.string() + " to PNG in createResource()");
                U::showError("Konnte " + path.string() + " nicht in PNG umwandeln");
            }
        }

        resourceManager.resources.push_back(r);
        saveResources();
        return resourceManager.resources.back();
    }
    catch (const std::exception& e) {
        LOG_ERR("RH", std::string("Couldn't create Resource in createResource() - ") + e.what());
        static Resource discard;
        discard.id = -1;
        discard.name = "invalidResource";
        discard.path = "";
        discard.is_video = false;
        discard.thumbnail_id = -1;
        return discard;
    }
}

// Deletes resource
void ResourceHandler::deleteResource(const int id)
{
    auto& resources = resourceManager.resources;

    for (const auto& res : resources) {
        if (res.id == id) {
            fs::path p1 = getRelativeImagePath() / res.path;
            fs::path p2 = getRelativeVideoPath() / res.path;
            fs::path p3 = getRelativeThumbnailPath() / res.path;

            fs::remove(p1);
            fs::remove(p2);
            fs::remove(p3);

            if (res.is_video) {
                deleteResource(res.thumbnail_id);
            }
        }
    }

    for (const auto& res : resources) {
        if (res.id == id)
            if (res.is_video)
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

// Returns reference of wanted resource
Resource& ResourceHandler::getResource(const int id)
{
    for (auto& r : resourceManager.resources) {
        if (r.id == id)
            return r;
    }
    LOG_ERR("RH", "Couldn't get Resource[" + std::to_string(id) + std::string("] in getResource()"));
    static Resource discard;
    discard.id = -1;
    discard.name = "invalidResource";
    discard.path = "";
    discard.is_video = false;
    discard.thumbnail_id = -1;
    return discard;
}

// Returns vector of all resource references
std::vector<Resource>& ResourceHandler::getResources() {
    return resourceManager.resources;
}
// Creates a split part from the source resource from start to end % on x-axis
int ResourceHandler::createSplitResource(int source_id, float start, float end, const std::string& name_suffix) {
    const Resource& src = getResource(source_id);

    if (!fs::exists(src.path)) {
        LOG_ERR("RH", "File[" + src.path.string() + "] does not exist in createSplitResource()");
        return -1;
    }

    if (getFileExtension(src.path.string()) != ".png") {
        LOG_ERR("RH", "File[" + src.path.string() + "] is not a .png in createSplitResource()");
        return -1;
    }

    const fs::path splitPath = getRelativeImagePath() / (src.name + "_" + name_suffix + ".png");
    fs::create_directories(splitPath.parent_path());

    if (!Utilities::cropImagePart(start, end, src.path.string(), splitPath.string())) {
        LOG_ERR("RH", "File[" + src.path.string() + "] couldn't be split in createSplitResource()");
        return -1;
    }

    Resource r;
    r.id = resourceManager.max_id++;
    r.name = src.name + "_" + name_suffix;
    r.path = splitPath;
    r.is_video = false;
    r.thumbnail_id = -1;

    resourceManager.resources.push_back(r);
    saveResources();

    return r.id;
}
// Returns resource path from id
int ResourceHandler::getResourceIdByPath(const fs::path& path) {
    for (auto& r : resourceManager.resources) {
        if (r.path == path)
            return r.id;
    }
    LOG_ERR("RH", "No resource found for " + path.string() + " in getResourceIdByPath()");
    return -1;
}

// Checks if all registered resources exist
bool ResourceHandler::verifyResources() {
    bool missing = false;
    for(auto& resource : resourceManager.resources) {
        if (!fs::exists(resource.path)) {
            missing = true;
            LOG_INFO("RH", resource.name);
        }
    }
    return missing;
}

// Deletes all resources that are registerd but not found
bool ResourceHandler::deleteMissingResources() {
    for(auto& resource : resourceManager.resources) {
        if (!fs::exists(resource.path)) {
            deleteResource(resource.id);
        }
    }
    return true;
}

fs::path ResourceHandler::getRelativeImagePath() {
    return save_data.path / save_data.name / "resources" / "images";
}

fs::path ResourceHandler::getRelativeVideoPath() {
    return save_data.path / save_data.name / "resources" / "videos";
}

fs::path ResourceHandler::getRelativeThumbnailPath() {
    return save_data.path / save_data.name / "resources" / "videos" / "thumbnails";
}

fs::path ResourceHandler::getRelativeResourceData() {
    return save_data.path / save_data.name / "resourceData.json";
}

std::string ResourceHandler::getFileExtension(const std::string in) {
    return fs::path(in).extension().string(); // e.g. .png
}
