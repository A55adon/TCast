#include "Window.h"

Window::Window() {
    if (!Shell::Initialize())
        exit(EXIT_FAILURE);

    if (!Backend::Initialize("Main Control Window", width, height, true)) {
        Shell::Shutdown();
        exit(EXIT_FAILURE);
    }

    window = Backend::GetWindow();

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());
    Rml::Initialise();

    context = Rml::CreateContext("main", Rml::Vector2i(width, height));
    if (!context) {
        Rml::Shutdown();
        Backend::Shutdown();
        Shell::Shutdown();
        exit(EXIT_FAILURE);
    }

    std::vector<Shell::FontFace> font_faces = {
        {"LatoLatin-Regular.ttf", false},
        {"ComicSans-Regular.ttf", true}
    };
    Shell::LoadFonts(font_faces);

    Rml::Debugger::Initialise(context);
    running = true;
}


Window::~Window() {
    projectors.clear();
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::addProjector(int monitor_index) {
    projectors.emplace_back(std::make_unique<Projector>(monitor_index));
}

void Window::update() {
    // Render main window (RmlUi)
    glfwMakeContextCurrent(window);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Update RmlUi here
    if (context) {
        context->Update();
        context->Render();
    }

    glfwSwapBuffers(window);

    // Render projectors (no RmlUi)
    for (auto& proj : projectors) {
        proj->update();
    }

    glfwPollEvents();
}

bool Window::shouldClose() {
    if (glfwWindowShouldClose(window)) return true;
    for (auto& proj : projectors) {
        if (proj->shouldClose()) return true;
    }
    return false;
}
