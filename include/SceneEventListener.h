#pragma once
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/ElementDocument.h>
#include <string>
#include "SceneEditActions.h"
#include "global.h"

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
                showRenameDialog(index);
            } else if (action == "delete") {
                deleteScene(index);
            } else if (action == "duplicate") {
                duplicateScene(index);
            }

            contextMenu->SetProperty("display", "none");
        }
    }
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
            std::cout << "Right click on scene: " << sceneIndex << std::endl;

            if (auto* contextMenu = document->GetElementById("sceneContextMenu")) {
                contextMenu->SetAttribute("data-target-scene", std::to_string(sceneIndex));
                float mouse_x = event.GetParameter("mouse_x", 0.0f);
                float mouse_y = event.GetParameter("mouse_y", 0.0f);
                contextMenu->SetProperty("left", Rml::ToString(mouse_x) + "px");
                contextMenu->SetProperty("top", Rml::ToString(mouse_y) + "px");
                contextMenu->SetProperty("display", "block");

                std::cout << "Context menu shown at: " << mouse_x << ", " << mouse_y << std::endl;
            } else {
                std::cout << "Context menu not found!" << std::endl;
            }
        } else if (button == 0) { // Left click
            std::cout << "Left click on scene: " << sceneIndex << std::endl;
            selectScene(sceneIndex);
        }
    }
};
