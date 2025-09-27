#include "RmlUi.h"

RmlUi::RmlUi() {
    if (init() != 0) {
        exit(EXIT_FAILURE);
    }
}

RmlUi::~RmlUi() {
    Rml::Shutdown();
    Backend::Shutdown();
    Shell::Shutdown();
}

int RmlUi::init() {

    if (!Shell::Initialize())
        return -1;

    if (!Backend::Initialize("TCast", window_width, window_height, true))
    {
        Shell::Shutdown();
        return -1;
    }

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());

    Rml::Initialise();

    // creates environment for the UI
    context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
    if (!context)
    {
        Rml::Shutdown();
        Backend::Shutdown();
        Shell::Shutdown();
        return -1;
    }
    std::vector<Shell::FontFace> font_faces = {
        {"LatoLatin-Regular.ttf", false},
        {"ComicSans-Regular.ttf", true}
    };

    Shell::LoadFonts(font_faces);

    Rml::Debugger::Initialise(context);

    running = true;

    return 0;
}

void RmlUi::run() {

}
