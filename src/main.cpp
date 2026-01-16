/*
 * Info:
 * - Render interface is in RmlUi_Renderer_GL3.cpp and can do all stib_image file formats + SVGs via nanosvg
 * - getEl and some other utility functions were left out of the namespace because its awkward to write Utilities:: everytime
 *
 * Known Bugs:
 * - when creating a new project or loading a project from the dropdown of the main interface it does not switch to that project automatically
 * - images flip around when you rename them?
 * - renaming does not work at all
 * - creating a project with a existing name crashes
 * - resource renaming deleting doenst work
 * - deleting resources doesnt delete the file
 *

 * Todo:
 * IMPORTANT:
 *  - resource handler
 * REST:
 *  - |installer
 *  - settings
 *  - ->dont have a recent path as a setting
 *  - feedback for exporting importing saving loading etc
 *  - if clicking on new project or loadproject have the ability to go back
 *  - |Update Checking
 *  - |drag n drop upload
 *  - problem two resources with the same name override each other
 *  - split video
 *  - renaming or deleting a resource shows white images
 *  - trying to change source when resource is deleted it crashes
 *  - resource name clipping
 *  - visuals
 *  - |multiselect
 *  - |drag n drop resource selection
 *  - |request missing
 *  - cleanup
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
            std::cout << "Loading Startup..." << std::endl;
            setStartupEventListeners();
            getWindow().document->Show();
        }
    }

    while (getWindow().running) {
        getWindow().update();
    }
    return 0;
}
