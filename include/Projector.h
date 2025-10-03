#pragma once

#include <glad/glad.h>
#include <RmlUi/Core.h>
#include <string>
#include "GLFW/glfw3.h"

class Projector
{
public:
    Projector(int monitor_index);
    ~Projector();

    void update() const;
    bool shouldClose() const;

    GLFWwindow* getWindow() const { return window; }

private:
    GLuint shaderProgram = 0;
    GLuint VAO, VBO = 0;

    GLFWwindow* window;
    int width, height;

    Rml::Context* context;
    //GLuint texture_id;
    //bool is_video;
    std::string content_path;
    //bool texture_loaded = false;
};