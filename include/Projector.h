#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <RmlUi/Core.h>
#include <string>

class Projector {
public:
    Projector(int monitor_index, const std::string& content_path = "", bool is_video = false);
    ~Projector();
    void update();
    bool shouldClose();
    GLFWwindow* getWindow() {
        return window;
    }
private:
    GLFWwindow* window;
    int width, height;
    Rml::Context* context;
    GLuint texture_id;
    bool is_video;
    std::string content_path;
};