/*
 * Info:
 * - Render interface is in RmlUi_Renderer_GL3.cpp and can do all stib_image file formats
 *
 * Known Bugs:
 * - if the amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the left
 * - when creating a new project or loading a project from the dropdown of the main interface it does not switch to that project automatically
 * - loading a project doesnt redirect to the interface screen

 * Todo:
 *  - installer
 *  - slider projector count
 *  - settings
 *  - dont have a recent path as a setting
 *  - delete projects
 *  - resources panel, upload, delete
 *  - callbacks for exporting importing saving loading etc
 *  - if clicking on new project or loadproject have the ability to go back
 *  - Update Checking
 *  - warn of invalid folder structure when loading a project and offer to fix it
 *
*/

#include "UISetup.h"

int main() {
    std::filesystem::path path = Utilities::getRecentPath();
    if (path != "") {
        projectPath = path;
        std::ifstream jFile(path / "saveData.json");
        nlohmann::json j;
        jFile >> j;

        from_json(j, saveData);
        std::cout << saveData.path << std::endl;

        if ((getWindow().document = getWindow().context->LoadDocument("assets/interface.rml"))) {
            setInterfaceEventListeners();
            getWindow().document->Show();
        }

    } else {
        if ((getWindow().document = getWindow().context->LoadDocument("assets/startup.rml"))) {
            setStartupEventListeners();
            getWindow().document->Show();
        }
    }
    std::cout << "Project at: " << projectPath << std::endl;

    using clock = std::chrono::high_resolution_clock;

    auto lastTime = clock::now();
    int frames = 0;

    while (getWindow().running) {
        getWindow().update();
        frames++;

        auto now = clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

        if (diff.count() >= 1) {
            std::cout << "FPS: " << frames << "\n";
            frames = 0;
            lastTime = now;
        }
    }
    return 0;
}
