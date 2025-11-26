/*
 * Info:
 * - Render interface is in RmlUi_Renderer_GL3.cpp and can do all stib_image file formats
 * - getEl and some other utilitie functions were left out of the namespace because i was to lazy to write Utilities:: everytime.
 *   I know you cant do namespace so you dont have to write Utilities:: everytime but I like to write it when using funcions from like UIManager
 *   or UISetup so i know where they are from
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
 *  - popups for showInfo and showError
 *  - upload videos
 *  - upload multiple resources at once
 *  - upload other file formats
 *  - drag n drop upload
 *  - two resources with the same name override each other
 *  - renaming or deleting a resource shows white images
 *  - trying to change source when resource is deleted it crashes
 *
*/

#include "UISetup.h"

int main() {
    std::filesystem::path path = Utilities::getRecentPath();
    if (path != "" && std::filesystem::exists(path)) {
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
    }
    return 0;
}
