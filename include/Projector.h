#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class Projector
{
public:
    Projector(int monitor_index, const std::string& path);
    ~Projector();

    void requestDie();
    bool isRunning();

    std::thread th;
private:
    void run(int monitor_index, std::string path);

    // OpenGL
    void initShaders();
    void initGLObjects();
    void loadTexture(const std::string& path);
    void draw();

    // Video
    void initVideo(const std::string& path);
    void updateVideo();
    void cleanupVideo();

    std::atomic<bool> running{ true };

    GLFWwindow* window = nullptr;

    GLuint shaderProgram = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint texture = 0;

    int width = 0;
    int height = 0;

    // Video state
    bool isVideo = false;

    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRGBA = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* swsCtx = nullptr;

    int videoStreamIndex = -1;
    double timeBase = 0.0;

    std::chrono::high_resolution_clock::time_point startTime;
};
