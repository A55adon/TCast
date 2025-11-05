/*
 * Info:
 * - Render interface is in RmlUi_Renderer_GL3.cpp and can do all stib_image file formats
 *
 * Known Bugs:
 * - if the amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the left
 * - when creating a new project or loading a project from the dropdown of the main interface it does not switch to that project automatically
 * - loading a project doesnt redirect to the interface screen

 * Todo:
 *  - slider for beamercount
 *  - settings
 *  - dont have a recentpath as a setting
 *  - delete projects
 *  - callbacks for exporting importing saving loading etc
 *  - if clicking on new project or loadproject have the ability to go back
 *  - Update Checking
*/

#include "../include/helper.h"

int main() {
    std::filesystem::path path = Utilities::getRecentPath();
    if (path != "") {
        projectPath = path;
        std::ifstream jFile(path / "saveData.json");
        nlohmann::json j;
        jFile >> j;

        from_json(j, saveData);
        std::cout << saveData.path << std::endl;

        if ((window.document = window.context->LoadDocument("assets/interface.rml"))) {
            setInterfaceEventListeners();
            window.document->Show();
        }

    } else {
        if ((window.document = window.context->LoadDocument("assets/startup.rml"))) {
            setStartupEventListeners();
            window.document->Show();
        }
    }
    std::cout << "Project at: " << projectPath << std::endl;

    while (window.running) {
        window.update();
    }
    return 0;
}
