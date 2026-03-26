/*
 * Known Bugs:
 * Todo:
 *  - feedback
 *  - if clicking on new project or loadproject have the ability to go back
 *  - |Update Checking
 *  - resource name clipping
 *  - visuals
 *  - |multiselect
 *  - drag n drop resource selection
 *  - |request missing
 *  - cleanup
 *  - long scene names
 *  - split on other thread
 *  - ~fix svg rendering
*/

#include "UISetup.h"
#include "Shell.h"
#include "RmlUi/Debugger.h"
#include "RmlUi_Backend.h"
#include <future>

void resize_callback(GLFWwindow* window, int width, int height)
{
    getWindow().context->SetDimensions(Rml::Vector2i(width, height));
    getWindow().context->Update();
    UIManager::refreshProjectors();
}

int main() {

    std::filesystem::path path = Utilities::getRecentPath();
    if (path != "" && std::filesystem::exists(path)) {
        current_project_path = path;
        std::ifstream jFile(path / "saveData.json");
        nlohmann::json j;
        jFile >> j;

        from_json(j, save_data);
        LOG_INFO("main","save_data.path");

        if ((getWindow().document = getWindow().context->LoadDocument("assets/interface.rml"))) {
             LOG_INFO("main","Loading Interface...");
            getWindow().document->Show();
            auto future = std::async(std::launch::async, setInterfaceEventListeners);

        }

    } else {
        if ((getWindow().document = getWindow().context->LoadDocument("assets/startup.rml"))) {
             LOG_INFO("main","No recent path -> loading startup...");
            getWindow().document->Show();
            setStartupEventListeners();
        }
    }

    glfwSetWindowSizeCallback(Backend::GetWindow(), resize_callback);

    while (getWindow().running) {
        getWindow().update();
    }
    return 0;
}
