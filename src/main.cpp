/*
 * Known Bugs:
 * - when creating a new project or loading a project from the dropdown of the main interface it does not switch to that project automatically
 * - creating a project with a existing name crashes
 * - image with spaces can lead to a crash
 * - loading a project without a path crashes
 * - window resize makes projector size 0
 * - video splitting restarts at like 60%
 * - video playback is laggy

 * Todo:
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
#include "Shell.h"
#include "RmlUi/Debugger.h"
#include "RmlUi_Backend.h"

void resize_callback() {
    UIManager::refreshProjectors();
}

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

    glfwSetWindowSizeCallback(Backend::GetWindow(), GLFWwindowsizefun(resize_callback));

    while (getWindow().running) {
        getWindow().update();
    }
    return 0;
}
