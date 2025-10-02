#pragma once

#include <vector>
#include <memory>
#include <RmlUi/Core.h>

#include "Projector.h"
// Don't delete these are actually used
#include "Shell.h"
#include "RmlUi/Debugger.h"
#include "RmlUi_Backend.h"

class Window
{
public:
    Window(int window_width, int window_height);
    ~Window();

    // The added Projectors can't have RmlUi because of the GL handle
    void addProjector(int monitor_index, const std::string& content_path = "", bool is_video = false);
    void removeProjector(int index);
    void update();

    Rml::ElementDocument* document;
    Rml::Context* context;
    bool running;

private:
    std::vector<std::unique_ptr<Projector>> projectors;
    int width = 800;
    int height = 600;
};