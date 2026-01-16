#include <fstream>
#include <nlohmann/json.hpp>

#include "../include/ResourceHandler.h"

#include "EventListener.h"
#include "Utilities.h"

using json = nlohmann::json;

ResourceManager ResourceHandler::rm;

bool ResourceHandler::initResources() {
    std::filesystem::create_directories(getRelativeImagePath());
    std::filesystem::create_directories(getRelativeVideoPath());
    std::filesystem::create_directories(getRelativeThumbnailPath());

    if (!std::filesystem::exists(saveData.path / saveData.projectName / "resourceData.json"))
        std::ofstream file(saveData.path / saveData.projectName / "resourceData.json");

    if (!loadResources()) {
        return false;
    }
    if (!verifyResources()) {
        return false;
    }

    return true;
}

bool ResourceHandler::loadResources() {
    const std::filesystem::path filePath = saveData.path / saveData.projectName / "resourceData.json";
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
    const std::filesystem::path filePath = saveData.path / saveData.projectName / "resourceData.json";
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

            r.path = getRelativeVideoPath() / (name + ".mp4");

            std::filesystem::create_directories(r.path.parent_path());
            std::filesystem::copy_file(
                path,
                r.path,
                std::filesystem::copy_options::overwrite_existing
            );

            const auto thumbPath = getRelativeThumbnailPath() / (name + ".png");
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

            r.path = getRelativeImagePath() / (name + ".png");

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


//std::string ResourceHandler::requestMissingResource(int id) { // "" -> remove; "path" -> new path;
//    std::string text = "'" + rm.resources[id].name + "' konnte nicht gefunden werden!";
//
//    auto resource_request_dialog = getEl("resource-request-dialog");
//
//    resource_request_dialog->SetClass("show", true);
//
//    resource_request_dialog = getEl("resource-edit-path");
//    resource_request_dialog->SetAttribute("value", rm.resources[id].path.string());
//
//    resource_request_dialog = getEl("resource-edit-name");
//    resource_request_dialog->SetInnerRML(rm.resources[id].name);
//
//    auto resource_browse = getEl("resource-request-browse");
//
//    std::string newPath;
//
//    resource_browse->AddEventListener(Rml::EventId::Click, new ButtonHandler ([newPath]{
//        newPath = Utilities::browseImageOrMp4();
//    }));
//
//    auto resource_apply = getEl("resource-request-apply");
//    resource_apply->AddEventListener(Rml::EventId::Click, new ButtonHandler ([newPath, id, resource_request_dialog] {
//        if (!newPath.empty()) {
//            resource_request_dialog->SetClass("show", false);
//            return newPath;
//        }
//    }));
//
//    auto resource_decline = getEl("resource-request-apply");
//    resource_apply->AddEventListener(Rml::EventId::Click, new ButtonHandler ([newPath, id, resource_request_dialog] {
//        resource_request_dialog->SetClass("show", false);
//        return "";
//    }));
//
//    return "";
//}
//

void ResourceHandler::deleteResource(int id)
{
    auto& resources = rm.resources;

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
    return saveData.path / saveData.projectName / "resources" / "images";
}

std::filesystem::path ResourceHandler::getRelativeVideoPath() {
    return saveData.path / saveData.projectName / "resources" / "videos";
}

std::filesystem::path ResourceHandler::getRelativeThumbnailPath() {
    return saveData.path / saveData.projectName / "resources" / "videos" / "thumbnails";
}

std::string ResourceHandler::getFileExtension(std::string in) {
    return std::filesystem::path(in).extension().string(); // return like ".png"
}
