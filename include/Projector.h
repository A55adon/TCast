#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <iostream>
#include <string>
#include <memory>

class Projector {
public:
    GLFWwindow* window = nullptr;
    Rml::Context* context = nullptr;
    int width = 0, height = 0;

    Projector(int monitor_index);

    ~Projector();

    void update();

    bool shouldClose() {
        return glfwWindowShouldClose(window);
    }
};
