#include "Projector.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstring>

// ================= SHADERS WITH SPLIT SUPPORT =================

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform float splitStart;
uniform float splitEnd;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    float texX = splitStart + (splitEnd - splitStart) * aTexCoord.x;
    TexCoord = vec2(texX, 1.0 - aTexCoord.y);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    FragColor = texture(texture1, TexCoord);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compilation error: " << log << std::endl;
        return 0;
    }
    return shader;
}

// ================= PUBLIC =================

Projector::Projector(int monitor_index, const std::string& path, const SplitInfo& splitInfo)
    : currentSplitInfo(splitInfo) {
    th = std::thread([=]() { run(monitor_index, path, splitInfo); });
}

Projector::~Projector() {
    running = false;
    if (th.joinable()) {
        th.join();
    }
}

void Projector::requestDie() {
    running = false;
}

bool Projector::isRunning() {
    return running && initialized;
}

std::vector<std::string> Projector::getResources() const {
    std::lock_guard<std::mutex> lock(splitInfoMutex);
    std::vector<std::string> resources;
    if (currentSplitInfo.resourceId != -1) {
        try {
            Resource resource = ResourceHandler::getResource(currentSplitInfo.resourceId);
            resources.push_back(resource.path.string());
        } catch (const std::exception& e) {
            std::cerr << "Error getting resource: " << e.what() << std::endl;
        }
    }
    return resources;
}

SplitInfo Projector::getSplitInfo() const {
    std::lock_guard<std::mutex> lock(splitInfoMutex);
    return currentSplitInfo;
}

bool Projector::isPlayingVideo() const {
    return isVideo;
}

float Projector::getVideoPlaybackProgress() const {
    std::lock_guard<std::mutex> lock(videoStateMutex);
    if (!isVideo || !formatCtx) return -1.0f;

    if (currentSplitInfo.isSplit) {
        double elapsed = std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        double splitDuration = videoSplitEndTime - videoSplitStartTime;

        if (splitDuration <= 0.0) return 0.0f;
        return static_cast<float>(elapsed / splitDuration);
    } else {
        double elapsed = std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        if (videoDuration <= 0.0) return 0.0f;
        return static_cast<float>(elapsed / videoDuration);
    }
}

float Projector::getVideoCurrentTime() const {
    std::lock_guard<std::mutex> lock(videoStateMutex);
    if (!isVideo || !formatCtx) return 0.0f;

    if (currentSplitInfo.isSplit) {
        double elapsed = std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        return static_cast<float>(videoSplitStartTime + elapsed);
    } else {
        double elapsed = std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        return static_cast<float>(elapsed);
    }
}

float Projector::getVideoDuration() const {
    std::lock_guard<std::mutex> lock(videoStateMutex);
    if (!isVideo || !formatCtx) return -1.0f;

    if (currentSplitInfo.isSplit) {
        return static_cast<float>(videoSplitEndTime - videoSplitStartTime);
    } else {
        return static_cast<float>(videoDuration);
    }
}

void Projector::updateSplitInfo(const SplitInfo& newSplitInfo) {
    {
        std::lock_guard<std::mutex> lock(splitInfoMutex);
        currentSplitInfo = newSplitInfo;
    }

    if (isVideo && formatCtx) {
        std::lock_guard<std::mutex> lock(videoStateMutex);
        videoSplitStartTime = videoDuration * currentSplitInfo.start;
        videoSplitEndTime = videoDuration * currentSplitInfo.end;

        if (currentSplitInfo.start > 0.0f) {
            seekToTime(videoSplitStartTime);
        }

        startTime = std::chrono::high_resolution_clock::now();
    }

    updateSplitUniforms();
}

// ================= CORE =================

void Projector::run(int monitor_index, std::string path, SplitInfo splitInfo) {
    {
        std::lock_guard<std::mutex> lock(splitInfoMutex);
        currentSplitInfo = splitInfo;
    }

    std::lock_guard<std::mutex> lock(glMutex);

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (!monitors || monitor_index >= count) {
        std::cerr << "Invalid monitor index: " << monitor_index << std::endl;
        return;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitor_index]);
    width = mode->width;
    height = mode->height;

    // Store window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);           // CRITICAL: Don't steal focus
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);     // CRITICAL: Don't focus on show
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);      // CRITICAL: Don't minimize
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);     // Don't center cursor

    // Create window
    window = glfwCreateWindow(width, height, "Projector", monitors[monitor_index], nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window for monitor " << monitor_index << std::endl;
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync for smooth video playback

    initShaders();
    if (shaderProgram == 0) {
        std::cerr << "Failed to initialize shaders" << std::endl;
        glfwDestroyWindow(window);
        return;
    }

    initGLObjects();

    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        loadTexture(path);
        isVideo = false;
    }
    else if (ext == ".mp4" || ext == ".avi" || ext == ".mov" || ext == ".mkv" || ext == ".flv") {
        isVideo = true;
        if (!initVideo(path)) {
            std::cerr << "Failed to initialize video: " << path << std::endl;
            glDeleteProgram(shaderProgram);
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteTextures(1, &texture);
            glfwDestroyWindow(window);
            return;
        }
        startTime = std::chrono::high_resolution_clock::now();

        if (currentSplitInfo.isSplit) {
            videoSplitStartTime = videoDuration * currentSplitInfo.start;
            videoSplitEndTime = videoDuration * currentSplitInfo.end;

            if (currentSplitInfo.start > 0.0f) {
                seekToTime(videoSplitStartTime);
            }
        }
    }
    else {
        std::cerr << "Unsupported format: " << ext << "\n";
        glDeleteProgram(shaderProgram);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &texture);
        glfwDestroyWindow(window);
        return;
    }

    // Set up shader uniforms
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    updateSplitUniforms();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, width, height);

    initialized = true;

    // Main render loop
    while (running && !glfwWindowShouldClose(window)) {
        glfwMakeContextCurrent(window);
        glClear(GL_COLOR_BUFFER_BIT);

        if (isVideo) {
            updateVideo();
        }

        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (isVideo)
        cleanupVideo();

    // Cleanup OpenGL resources
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);

    glfwDestroyWindow(window);
    initialized = false;
}

// ================= OPENGL =================

void Projector::initShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vs == 0) return;

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (fs == 0) {
        glDeleteShader(vs);
        return;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    GLint linkOk;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkOk);
    if (!linkOk) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "Shader program link error: " << log << std::endl;
        shaderProgram = 0;
    } else {
        splitStartUniform = glGetUniformLocation(shaderProgram, "splitStart");
        splitEndUniform = glGetUniformLocation(shaderProgram, "splitEnd");
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Projector::initGLObjects() {
    float verts[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };
    unsigned int idx[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Projector::loadTexture(const std::string& path) {
    int w, h, c;
    //stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);
    std::cout << "Loaded texture: " << path << " (" << w << "x" << h << ")" << std::endl;
}

void Projector::updateSplitUniforms() {
    std::lock_guard<std::mutex> lock(splitInfoMutex);
    glUseProgram(shaderProgram);

    if (currentSplitInfo.isSplit) {
        glUniform1f(splitStartUniform, currentSplitInfo.start);
        glUniform1f(splitEndUniform, currentSplitInfo.end);
    } else {
        glUniform1f(splitStartUniform, 0.0f);
        glUniform1f(splitEndUniform, 1.0f);
    }
}

void Projector::draw() {
    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ================= VIDEO =================

bool Projector::initVideo(const std::string& path) {
    // Initialize FFmpeg
    avformat_network_init();

    if (avformat_open_input(&formatCtx, path.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Failed to open video: " << path << std::endl;
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Failed to find stream info" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    // Find the first video stream
    videoStreamIndex = -1;
    for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "No video stream found" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecParameters* codecParams = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        std::cerr << "Failed to allocate codec context" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    if (avcodec_parameters_to_context(codecCtx, codecParams) < 0) {
        std::cerr << "Failed to copy codec parameters" << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec" << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    // Allocate frames
    frame = av_frame_alloc();
    frameRGBA = av_frame_alloc();
    if (!frame || !frameRGBA) {
        std::cerr << "Failed to allocate frames" << std::endl;
        cleanupVideo();
        return false;
    }

    packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Failed to allocate packet" << std::endl;
        cleanupVideo();
        return false;
    }

    // Calculate video duration
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        videoDuration = formatCtx->duration / (double)AV_TIME_BASE;
    } else {
        videoDuration = formatCtx->streams[videoStreamIndex]->duration *
                       av_q2d(formatCtx->streams[videoStreamIndex]->time_base);
    }

    // Prepare for converting to RGB
    swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        std::cerr << "Failed to create sws context" << std::endl;
        cleanupVideo();
        return false;
    }

    // Allocate buffer for RGBA frame
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, codecCtx->width, codecCtx->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
    if (!buffer) {
        std::cerr << "Failed to allocate buffer" << std::endl;
        cleanupVideo();
        return false;
    }

    av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize,
        buffer, AV_PIX_FMT_RGBA,
        codecCtx->width, codecCtx->height, 1);

    // Create OpenGL texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        codecCtx->width, codecCtx->height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    timeBase = av_q2d(formatCtx->streams[videoStreamIndex]->time_base);

    std::cout << "Loaded video: " << path << " (" << codecCtx->width << "x" << codecCtx->height
              << "), Duration: " << videoDuration << "s" << std::endl;
    return true;
}

void Projector::seekToTime(double seconds) {
    if (!formatCtx || videoStreamIndex < 0) return;

    int64_t timestamp = (int64_t)(seconds / av_q2d(formatCtx->streams[videoStreamIndex]->time_base));

    if (av_seek_frame(formatCtx, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Failed to seek in video" << std::endl;
        return;
    }

    avcodec_flush_buffers(codecCtx);
}
void Projector::updateVideo() {
    std::lock_guard<std::mutex> lock(videoStateMutex);

    if (!formatCtx || !codecCtx) {
        return;
    }

    // Get the video's frame rate
    AVStream* stream = formatCtx->streams[videoStreamIndex];
    double frameRate = av_q2d(stream->avg_frame_rate);
    if (frameRate <= 0.0) {
        frameRate = av_q2d(stream->r_frame_rate);
        if (frameRate <= 0.0) {
            frameRate = 30.0; // fallback to 30 FPS
        }
    }

    // Calculate how much time has passed since we started
    auto now = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime).count();

    // Calculate target playback position
    double targetTime;
    if (currentSplitInfo.isSplit) {
        targetTime = videoSplitStartTime + elapsed;

        // Check if we've reached the end of the split
        if (targetTime >= videoSplitEndTime) {
            // Calculate how far we overshot the end
            double overshoot = targetTime - videoSplitEndTime;

            // Seek to start of split
            seekToTime(videoSplitStartTime);

            // Adjust start time to account for overshoot
            startTime = now - std::chrono::duration<double>(overshoot);
            targetTime = videoSplitStartTime + overshoot;
        }
    } else {
        targetTime = elapsed;

        // Check if we've reached the end of the video
        if (targetTime >= videoDuration) {
            // Calculate how far we overshot the end
            double overshoot = targetTime - videoDuration;

            // Seek to start
            seekToTime(0);

            // Adjust start time to account for overshoot
            startTime = now - std::chrono::duration<double>(overshoot);
            targetTime = overshoot;
        }
    }

    // Calculate how many frames should have been displayed by now
    double targetFrameNumber = targetTime * frameRate;

    // Track the current frame number we're displaying
    static double currentFrameNumber = -1.0;

    // Only decode a new frame if we're behind by at least 0.5 frames
    if (currentFrameNumber < 0 || (targetFrameNumber - currentFrameNumber) >= 0.5) {

        bool frameFound = false;
        while (!frameFound) {
            int readResult = av_read_frame(formatCtx, packet);

            if (readResult < 0) {
                // End of file - seek back to appropriate position
                if (currentSplitInfo.isSplit) {
                    seekToTime(videoSplitStartTime);
                } else {
                    seekToTime(0);
                }
                // Reset frame tracking
                currentFrameNumber = -1.0;
                continue;
            }

            if (packet->stream_index != videoStreamIndex) {
                av_packet_unref(packet);
                continue;
            }

            int ret = avcodec_send_packet(codecCtx, packet);
            av_packet_unref(packet);

            if (ret < 0) {
                continue;
            }

            ret = avcodec_receive_frame(codecCtx, frame);

            if (ret == AVERROR(EAGAIN)) {
                continue;
            } else if (ret == AVERROR_EOF) {
                // End of stream - loop
                if (currentSplitInfo.isSplit) {
                    seekToTime(videoSplitStartTime);
                } else {
                    seekToTime(0);
                }
                currentFrameNumber = -1.0;
                continue;
            } else if (ret < 0) {
                continue;
            }

            // Calculate frame time and frame number
            double frameTime = frame->pts * timeBase;

            // Check split boundaries
            if (currentSplitInfo.isSplit) {
                if (frameTime < videoSplitStartTime) {
                    av_frame_unref(frame);
                    continue;
                }
                if (frameTime > videoSplitEndTime) {
                    av_frame_unref(frame);
                    // Loop back to start of split
                    seekToTime(videoSplitStartTime);
                    currentFrameNumber = -1.0;
                    continue;
                }
            }

            // Calculate which frame number this is
            currentFrameNumber = frameTime * frameRate;

            // Display this frame
            frameFound = true;

            // Convert frame to RGBA
            sws_scale(swsCtx,
                      frame->data, frame->linesize,
                      0, codecCtx->height,
                      frameRGBA->data, frameRGBA->linesize);

            // Update OpenGL texture
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            codecCtx->width, codecCtx->height,
                            GL_RGBA, GL_UNSIGNED_BYTE,
                            frameRGBA->data[0]);

            av_frame_unref(frame);
        }
    }
    // else: keep displaying the current frame
}

void Projector::cleanupVideo() {
    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    if (frameRGBA) {
        if (frameRGBA->data[0]) {
            av_free(frameRGBA->data[0]);
        }
        av_frame_free(&frameRGBA);
        frameRGBA = nullptr;
    }
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }

    avformat_network_deinit();
}

double Projector::getVideoFrameRate() const {
    if (!formatCtx || videoStreamIndex < 0) {
        return 30.0; // Default fallback
    }

    AVStream* stream = formatCtx->streams[videoStreamIndex];
    double frameRate = av_q2d(stream->avg_frame_rate);

    if (frameRate <= 0.0) {
        frameRate = av_q2d(stream->r_frame_rate);
    }

    if (frameRate <= 0.0) {
        frameRate = 30.0; // Default fallback
    }

    return frameRate;
}