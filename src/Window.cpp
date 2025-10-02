#include "Window.h"

Window::Window(const int window_width, const int window_height): document(nullptr)
{
    if (!ShellRml::Initialize())
        exit(1);

    if (!Backend::Initialize("TCast", window_width, window_height, true)) {
        ShellRml::Shutdown();
        exit(1);
    }

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());
    Rml::Initialise();

    glfwInit();

    context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
    if (!context) {
        Rml::Shutdown();
        Backend::Shutdown();
        ShellRml::Shutdown();
        exit(1);
    }

    Rml::Debugger::Initialise(context);

    const std::vector<ShellRml::FontFace> font_faces = {
        {"LatoLatin-Regular.ttf", false},
        {"ComicSans-Regular.ttf", true}
    };

    ShellRml::LoadFonts(font_faces);

    running = true;
}

Window::~Window()
{
    Rml::Shutdown();
    Backend::Shutdown();
    ShellRml::Shutdown();
}

void Window::addProjector(int monitor_index, const std::string& content_path, bool is_video)
{
    projectors.emplace_back(std::make_unique<Projector>(monitor_index));
    glfwMakeContextCurrent(Backend::GetWindow()); // Give the context back to the main window after creating a new projector window
}

void Window::removeProjector(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= projectors.size()) {
        std::cerr << "Invalid projector index\n";
        return;
    }

    if (const auto& projector = projectors[index]; projector && projector->getWindow()) {
        glfwMakeContextCurrent(projector->getWindow());
    }

    projectors.erase(projectors.begin() + index);

    glfwMakeContextCurrent(Backend::GetWindow());
}

void Window::update()
{
    // Update main window
    glfwMakeContextCurrent(Backend::GetWindow());
    running = Backend::ProcessEvents(context, &ShellRml::ProcessKeyDownShortcuts, true);
    context->Update();
    Backend::BeginFrame();
    context->Render();
    Backend::PresentFrame();

    // Update projectors
    for (const auto& projector : projectors)
    {
        if (!projector) continue;
        projector->update();

        if (projector->shouldClose())
            running = false;
    }

    glfwMakeContextCurrent(Backend::GetWindow());
}
