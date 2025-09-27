#pragma once

#include <GL/glew.h>          // MUST come first
#include <GLFW/glfw3.h>       // After GLEW

#include <memory>
#include <vector>

#include "Projector.h"
#include "RmlUi_Backend.h"
#include <RmlUi/Core.h>
#include <Shell.h>


class Window {
public:
    GLFWwindow* window = nullptr;
    int width = 800, height = 600;

    std::vector<std::unique_ptr<Projector>> projectors;

    Window();

    ~Window();

    void addProjector(int monitor_index);

    void update();

    bool shouldClose();

    bool running;
    Rml::ElementDocument* document;
    Rml::Context* context;
};
