#pragma once

#include <RmlUi/Core.h>
#include <functional>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/ElementDocument.h>
#include <string>
#include "global.h"
#include "Utilities.h"

class ButtonHandler final : public Rml::EventListener {
    std::function<void()> callback;
    bool stopPropagation;
public:
    explicit ButtonHandler(std::function<void()> callback, bool stopPropagation = false)
        : callback(std::move(callback)), stopPropagation(stopPropagation) {}

    void ProcessEvent(Rml::Event& event) override {
        if (stopPropagation) {
            event.StopPropagation();
        }
        callback();
    }
};

class KeyEventHandler : public Rml::EventListener {
    std::function<void(Rml::Event&)> callback;
public:
    explicit KeyEventHandler(std::function<void(Rml::Event&)> callback)
        : callback(std::move(callback)) {}

    void ProcessEvent(Rml::Event& event) override {
        callback(event);
    }
};


class SceneContextMenuHandler : public Rml::EventListener {
    Rml::ElementDocument* document;
    std::string action;
public:
    SceneContextMenuHandler(Rml::ElementDocument* doc, const std::string& action_)
        : document(doc), action(action_) {}

    void ProcessEvent(Rml::Event& event) override {
        if (auto* contextMenu = document->GetElementById("sceneContextMenu")) {
            auto* attr = contextMenu->GetAttribute("data-target-scene");
            int index = 0;
            if (attr) index = std::stoi(attr->Get<Rml::String>().c_str());

            if (action == "rename") {
                showSceneRenameDialog(index);
            } else if (action == "delete") {
                deleteScene(index);
            } else if (action == "duplicate") {
                duplicateScene(index);
            }

            contextMenu->SetProperty("display", "none");
        }
    }
};

struct MouseEventHandler : Rml::EventListener {
    std::function<void(Rml::Event&)> callback;
    explicit MouseEventHandler(std::function<void(Rml::Event&)> cb) : callback(std::move(cb)) {}
    void ProcessEvent(Rml::Event& event) override { callback(event); }
};

class SceneItemHandler : public Rml::EventListener {
    Rml::ElementDocument* document;
    int sceneIndex;
public:
    SceneItemHandler(Rml::ElementDocument* doc, int index)
        : document(doc), sceneIndex(index) {}

    void ProcessEvent(Rml::Event& event) override {
        int button = event.GetParameter<int>("button", 0);
        if (button == 1) { // Right click
            event.StopPropagation();
            //std::cout << "Right click on scene: " << sceneIndex << std::endl;

            if (auto* contextMenu = document->GetElementById("sceneContextMenu")) {
                contextMenu->SetAttribute("data-target-scene", std::to_string(sceneIndex));
                float mouse_x = event.GetParameter("mouse_x", 0.0f);
                float mouse_y = event.GetParameter("mouse_y", 0.0f);
                contextMenu->SetProperty("left", Rml::ToString(mouse_x) + "px");
                contextMenu->SetProperty("top", Rml::ToString(mouse_y) + "px");
                contextMenu->SetProperty("display", "block");

                //std::cout << "Context menu shown at: " << mouse_x << ", " << mouse_y << std::endl;
            } else {
                LOG_ERR("EventListener", "Context menu not found!");
            }
        } else if (button == 0) { // Left click
            //std::cout << "Left click on scene: " << sceneIndex << std::endl;
            selectScene(sceneIndex);
        }
    }
};


class ResourceContextMenuHandler : public Rml::EventListener {
    Rml::ElementDocument* document;
    std::string action;
public:
    ResourceContextMenuHandler(Rml::ElementDocument* doc, const std::string& action_)
        : document(doc), action(action_) {}

    void ProcessEvent(Rml::Event& event) override {
        if (auto* contextMenu = document->GetElementById("resourceContextMenu")) {
            auto* attr = contextMenu->GetAttribute("data-target-resource");
            int id = 0;
            if (attr) id = std::stoi(attr->Get<Rml::String>().c_str());

            if (action == "rename") {
                showResourceRenameDialog(id);
            } else if (action == "delete") {
                //TODO: sure?
                deleteResource(id);
            }

            contextMenu->SetProperty("display", "none");
        }
    }
};
class ResourceItemHandler : public Rml::EventListener {
    Rml::ElementDocument* document;
    int resourceIndex;
public:
    ResourceItemHandler(Rml::ElementDocument* doc, int index)
        : document(doc), resourceIndex(index) {}

    void ProcessEvent(Rml::Event& event) {
        int button = event.GetParameter<int>("button", 0);

        // Check if this resource is currently being renamed
        std::string itemId = "resource-item-" + std::to_string(resourceIndex);
        if (auto* itemEl = getEl(itemId)) {
            if (itemEl->IsClassSet("renaming")) {
                return; // ignore click while renaming
            }
        }

        if (button == 1) { // Right click
            event.StopPropagation();
            //std::cout << "Right click on resource: " << resourceIndex << std::endl;

            if (auto* contextMenu = document->GetElementById("resourceContextMenu")) {
                contextMenu->SetAttribute("data-target-resource", std::to_string(resourceIndex));
                float mouse_x = event.GetParameter("mouse_x", 0.0f);
                float mouse_y = event.GetParameter("mouse_y", 0.0f);
                contextMenu->SetProperty("left", Rml::ToString(mouse_x) + "px");
                contextMenu->SetProperty("top", Rml::ToString(mouse_y) + "px");
                contextMenu->SetProperty("display", "block");
            }
        } else if (button == 0) { // Left click
            selectResource(resourceIndex);
        }
    }

};


class ProjectorContextMenuHandler : public Rml::EventListener {
    Rml::ElementDocument* document;
    std::string action;
public:
    ProjectorContextMenuHandler(Rml::ElementDocument* doc, const std::string& action_)
        : document(doc), action(action_) {}

    void ProcessEvent(Rml::Event& event) override {
        if (auto* contextMenu = document->GetElementById("projectorContextMenu")) {
            auto* attr = contextMenu->GetAttribute("data-target-projector");
            int index = 0;
            if (attr) index = std::stoi(attr->Get<Rml::String>().c_str());

            if (action == "selectResource") {
                showProjectorResourceSelection(index);
            }

            contextMenu->SetProperty("display", "none");
        }
    }
};
class ProjectorHandler : public Rml::EventListener {
    Rml::ElementDocument* document;
    int projectorIndex;
public:
    ProjectorHandler(Rml::ElementDocument* doc, int index)
        : document(doc), projectorIndex(index) {}

    void ProcessEvent(Rml::Event& event) {
        int button = event.GetParameter<int>("button", 0);

        // Check if this resource is currently being renamed
        std::string itemId = "projector-" + std::to_string(projectorIndex);
        if (auto* itemEl = getEl(itemId)) {
            if (itemEl->IsClassSet("setResource")) {
                auto* contextMenu = getEl("projectorContextMenu");
                contextMenu->SetProperty("display", "none");
            }
        }

        if (button == 1) { // Right click
            event.StopPropagation();
            //std::cout << "Right click on projector: " << projectorIndex << std::endl;

            //if (auto* contextMenu = document->GetElementById("projectorContextMenu")) {
            //    contextMenu->SetAttribute("data-target-projector", std::to_string(projectorIndex));
            //    float mouse_x = event.GetParameter("mouse_x", 0.0f);
            //    float mouse_y = event.GetParameter("mouse_y", 0.0f);
            //    contextMenu->SetProperty("left", Rml::ToString(mouse_x) + "px");
            //    contextMenu->SetProperty("top", Rml::ToString(mouse_y) + "px");
            //    contextMenu->SetProperty("display", "block");
            //}
                showProjectorResourceSelection(projectorIndex);
        } else if (button == 0) { // Left click
            //Todo: Drag to move or connect
        }
    }
};


class ConnectHandler : public Rml::EventListener {
public:
    int index;

    ConnectHandler(int i) : index(i) {}

    void ProcessEvent(Rml::Event& event) override {
        if (scene_manager.scenes[active_scene_index].connections[index] == 1) {
            disconnectProjectors(index);
            scene_manager.scenes[active_scene_index].connections[index] = 0;
        } else if (!scene_manager.scenes[active_scene_index].sources[index].empty()){
            connectProjectors(index);
            scene_manager.scenes[active_scene_index].connections[index] = 1;
        }
    }
};