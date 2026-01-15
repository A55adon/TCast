#include "Projector.h"

#include <iostream>
#include <filesystem>
#include <algorithm>

// ================= SHADERS =================

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
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
        std::cerr << log << std::endl;
    }
    return shader;
}

// ================= PUBLIC =================

Projector::Projector(int monitor_index, const std::string& path) {
    th = std::thread([=]() { run(monitor_index, path); });
}

Projector::~Projector() {
    running = false;
    if (th.joinable())
        th.join();
}

void Projector::requestDie() {
    running = false;
}

bool Projector::isRunning() {
    return running;
}

// ================= CORE =================

void Projector::run(int monitor_index, std::string path) {
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (!monitors || monitor_index >= count)
        return;

    const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitor_index]);
    width = mode->width;
    height = mode->height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Projector", monitors[monitor_index], nullptr);
    if (!window)
        return;

    glfwMakeContextCurrent(window);

    initShaders();
    initGLObjects();

    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".png") {
        loadTexture(path);
    }
    else if (ext == ".mp4") {
        isVideo = true;
        initVideo(path);
        startTime = std::chrono::high_resolution_clock::now();
    }
    else {
        std::cerr << "Unsupported format\n";
        return;
    }

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    while (running && !glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        if (isVideo)
            updateVideo();

        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (isVideo)
        cleanupVideo();

    glfwDestroyWindow(window);
}

// ================= OPENGL =================

void Projector::initShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Projector::initGLObjects() {
    float verts[] = {
        -1, -1, 0, 0,
         1, -1, 1, 0,
         1,  1, 1, 1,
        -1,  1, 0, 1
    };
    unsigned idx[] = { 0,1,2, 2,3,0 };

    GLuint ebo;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &ebo);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Projector::loadTexture(const std::string& path) {
    int w, h, c;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
    if (!data) return;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

void Projector::draw() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ================= VIDEO =================

void Projector::initVideo(const std::string& path) {
    avformat_open_input(&formatCtx, path.c_str(), nullptr, nullptr);
    avformat_find_stream_info(formatCtx, nullptr);

    for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    const AVCodec* codec = avcodec_find_decoder( formatCtx->streams[videoStreamIndex]->codecpar->codec_id);


    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx,
        formatCtx->streams[videoStreamIndex]->codecpar);
    avcodec_open2(codecCtx, codec, nullptr);

    frame = av_frame_alloc();
    frameRGBA = av_frame_alloc();
    packet = av_packet_alloc();

    int size = av_image_get_buffer_size(
        AV_PIX_FMT_RGBA, codecCtx->width, codecCtx->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(size);

    av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize,
        buffer, AV_PIX_FMT_RGBA,
        codecCtx->width, codecCtx->height, 1);

    swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        codecCtx->width, codecCtx->height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    timeBase = av_q2d(formatCtx->streams[videoStreamIndex]->time_base);
}

void Projector::updateVideo() {
    double elapsed = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - startTime).count();

    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }

        avcodec_send_packet(codecCtx, packet);
        av_packet_unref(packet);

        if (avcodec_receive_frame(codecCtx, frame) == 0) {
            double pts = frame->pts * timeBase;
            if (pts > elapsed)
                return;

            sws_scale(swsCtx,
                frame->data, frame->linesize,
                0, codecCtx->height,
                frameRGBA->data, frameRGBA->linesize);

            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                codecCtx->width, codecCtx->height,
                GL_RGBA, GL_UNSIGNED_BYTE,
                frameRGBA->data[0]);
            return;
        }
    }
}

void Projector::cleanupVideo() {
    if (swsCtx) sws_freeContext(swsCtx);
    if (frame) av_frame_free(&frame);
    if (frameRGBA) av_frame_free(&frameRGBA);
    if (packet) av_packet_free(&packet);
    if (codecCtx) avcodec_free_context(&codecCtx);
    if (formatCtx) avformat_close_input(&formatCtx);
}
