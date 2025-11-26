#pragma once

#include <atomic>
#include <glad/glad.h>
#include <RmlUi/Core.h>
#include <string>
#include <thread>

#include "GLFW/glfw3.h"
#include "stb_image.h"

class Projector
{
public:
    Projector(int monitor_index, std::string img);
    ~Projector();
    std::atomic<bool> running{true};
    void requestDie();   // just signal thread to stop
    bool isRunning();    // check if thread is still running
    void initShaders();


    std::thread th;
private:
    void run(int monitor_index, std::string img);

    void die();
    void initGLObjects();
    void loadTexture(const std::string& path);
    void draw();


    GLuint shaderProgram = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint texture = 0;

    GLFWwindow* window = nullptr;
    int width = 0;
    int height = 0;
};
