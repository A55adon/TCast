#pragma once
#include "Projector.h"
#include <RmlUi/Core.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

class Window {
public:
    Window(int window_width, int window_height);
    ~Window();

    void addProjector(int monitor_index, const std::string& content_path = "", bool is_video = false);
    void update();


    Rml::Context* context;
    bool running;
    Rml::ElementDocument* document;
private:
    GLFWwindow* window;
    std::vector<std::unique_ptr<Projector>> projectors;
    int width = 800;
    int height = 600;
};