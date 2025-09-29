#include "Window.h"
#include "RmlUi/Debugger.h"
#include "RmlUi_Backend.h"
#include "Shell.h"
#include <iostream>

Window::Window(int window_width, int window_height) {
    if (!Shell::Initialize())
        exit(1);

    if (!Backend::Initialize("TCast", window_width, window_height, true))
    {
        Shell::Shutdown();
        exit(1);
    }

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());

    Rml::Initialise();

    context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
    if (!context)
    {
        Rml::Shutdown();
        Backend::Shutdown();
        Shell::Shutdown();
        exit(1);
    }

    Rml::Debugger::Initialise(context);

    std::vector<Shell::FontFace> font_faces = {
        {"LatoLatin-Regular.ttf", false},
        {"ComicSans-Regular.ttf", true}
    };
    Shell::LoadFonts(font_faces);

    // Load and show the tutorial document.
    if (document = context->LoadDocument("assets/interface.rml"))
        document->Show();

    running = true;
}

Window::~Window() {
    Rml::Shutdown();
    Backend::Shutdown();
    Shell::Shutdown();
}

void Window::addProjector(int monitor_index, const std::string& content_path, bool is_video) {
    projectors.emplace_back(std::make_unique<Projector>(monitor_index, content_path, is_video));
}

void Window::update() {
    glfwMakeContextCurrent(Backend::GetWindow());
    running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

    context->Update();

    Backend::BeginFrame();
    context->Render();
    Backend::PresentFrame();

    for (auto& projector : projectors) {
        glfwMakeContextCurrent(projector->getWindow());
        projector->update();
        if (projector->shouldClose()) {
            running = false;
        }
    }
    glfwMakeContextCurrent(Backend::GetWindow());

}
