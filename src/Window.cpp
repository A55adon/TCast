#include "Window.h"
#include "Shell.h"
#include "RmlUi/Debugger.h"
#include "RmlUi_Backend.h"

#include <iostream>

Window::Window(const int window_width, const int window_height): document(nullptr)
{
    std::cout << "[Window] Initializing" << std::endl;
    if (!ShellRml::Initialize())
        exit(1);

    std::cout << "[Window] shellInit" << std::endl;

    if (!Backend::Initialize("TCast", window_width, window_height, true)) {
        std::cout << "[Window] backendShutdown" << std::endl;
        ShellRml::Shutdown();
        exit(1);
    }
    std::cout << "[Window] backendInit" << std::endl;

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());
    Rml::Initialise();

    std::cout << "[Window] renderinterfaceInit" << std::endl;

    glfwInit();
    std::cout << "[Window] glfwInit" << std::endl;

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
    std::cout << "[Window] loadFonts" << std::endl;


    glfwMaximizeWindow(Backend::GetWindow());

    std::cout << "[Window] maximize" << std::endl;


    running = true;
}

Window::~Window()
{
    Rml::Shutdown();
    Backend::Shutdown();
    ShellRml::Shutdown();
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

    glfwMakeContextCurrent(Backend::GetWindow());
}
