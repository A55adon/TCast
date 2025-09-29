#include "Projector.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>

Projector::Projector(int monitor_index, const std::string& content_path, bool is_video) : content_path(content_path), is_video(is_video) {
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    if (monitor_index >= monitorCount) {
        std::cerr << "Monitor index out of range\n";
        exit(EXIT_FAILURE);
    }

    GLFWmonitor* monitor = monitors[monitor_index];
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    width = mode->width;
    height = mode->height;

    window = glfwCreateWindow(width, height, "Projector", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window for projector\n";
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture_id);
}

Projector::~Projector() {
    if (texture_id) glDeleteTextures(1, &texture_id);
    if (window) glfwDestroyWindow(window);
}


void Projector::update() {
    if (!window) return;
    glfwMakeContextCurrent(window);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!is_video && texture_id) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(1, 0); glVertex2f(1, -1);
        glTexCoord2f(1, 1); glVertex2f(1, 1);
        glTexCoord2f(0, 1); glVertex2f(-1, 1);
        glEnd();
    }

    glfwSwapBuffers(window);
}

bool Projector::shouldClose() {
    return glfwWindowShouldClose(window);
}