#include "../include/Projector.h"

Projector::Projector(int monitor_index) {
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

    // Each projector gets its own context (can optionally share with main)
    window = glfwCreateWindow(width, height, "Projector", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window for projector\n";
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glewInit();
}


Projector::~Projector() {
    Rml::Shutdown();
    if (window) glfwDestroyWindow(window);
}

void Projector::update() {
    if (!window) return;
    glfwMakeContextCurrent(window);

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (context) {
        context->Update();
        context->Render();
    }

    glfwSwapBuffers(window);
}
