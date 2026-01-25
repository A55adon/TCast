#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <iostream>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "ResourceHandler.h"
#include "stb_image.h"

// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

struct SplitInfo {
    int resourceId = -1;
    float start = 0.f;
    float end = 1.f;
    bool isSplit = false;
};

class Projector
{
public:
    // Constructor now takes SplitInfo
    Projector(int monitor_index, const std::string& path, const SplitInfo& splitInfo);
    ~Projector();

    std::vector<std::string> getResources() const;
    SplitInfo getSplitInfo() const;
    bool isPlayingVideo() const;
    float getVideoPlaybackProgress() const;
    float getVideoCurrentTime() const;
    float getVideoDuration() const;

    void requestDie();
    bool isRunning();

    // Update split info dynamically
    void updateSplitInfo(const SplitInfo& newSplitInfo);

    std::thread th;

private:
    void run(int monitor_index, std::string path, SplitInfo splitInfo);

    // OpenGL
    void initShaders();
    void initGLObjects();
    void loadTexture(const std::string& path);
    void draw();

    // Split rendering
    void updateSplitUniforms();

    // Video
    bool initVideo(const std::string& path);
    void updateVideo();
    void cleanupVideo();
    void seekToTime(double seconds); // For video splits
    double getVideoFrameRate() const;

    std::atomic<bool> running{ true };
    std::atomic<bool> initialized{ false };

    GLFWwindow* window = nullptr;

    GLuint shaderProgram = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint texture = 0;

    // Split uniforms
    GLuint splitStartUniform = 0;
    GLuint splitEndUniform = 0;
    SplitInfo currentSplitInfo;

    int width = 0;
    int height = 0;

    // Video state
    bool isVideo = false;
    double videoDuration = 0.0;
    double videoSplitStartTime = 0.0;
    double videoSplitEndTime = 0.0;

    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRGBA = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* swsCtx = nullptr;

    int videoStreamIndex = -1;
    double timeBase = 0.0;

    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double, std::ratio<1, 1000000000>>>
    startTime;

    // Thread safety
    mutable std::mutex splitInfoMutex;
    mutable std::mutex videoStateMutex;
    mutable std::mutex glMutex;
};