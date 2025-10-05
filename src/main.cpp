#include "Helper.h"


/*
 * Known Bugs:
 * - if the amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the left
 * *
 * Todo:
 * - message loaded/saved/created successfully
 *  - Update Checking
*/


void switchToStartup() {
    if ((window.document = window.context->LoadDocument("assets/startup.rml")))
        setStartupInterfaceEventListeners();
    window.document->Show();
}

void setInterfaceEventListeners() {
    if (startupEventlistenersInitialized)
        return;
    else
        startupEventlistenersInitialized = true;

    if (auto* dropdownNewproject = window.document->GetElementById("file-dropdown-newproject")) {
        dropdownNewproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdownNewproject] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new")->SetClass("active", true);
            window.document->GetElementById("tab-load")->SetClass("active", false);
            window.document->GetElementById("tab-load")->SetAttribute("style", "display:none");
        }));
    }
    if (auto* dropdownLoadproject = window.document->GetElementById("file-dropdown-loadproject")) {
        dropdownLoadproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdownLoadproject] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-load")->SetClass("active", true);
            window.document->GetElementById("tab-new")->SetClass("active", false);
            window.document->GetElementById("tab-new")->SetAttribute("style", "display:none");
        }));
    }
}


int main() {
    if (std::filesystem::exists("../saves/recent.path")) {

        std::fstream pFile("../saves/recent.path");
        std::stringstream path;
        path << pFile.rdbuf();
        pFile.close();
        std::ifstream jFile(path.str());
        nlohmann::json j;
        jFile >> j;

        from_json(j, saveData);
        std::cout << saveData.path << std::endl;


        if ((window.document = window.context->LoadDocument("assets/interface.rml")))
            window.document->Show();

        setInterfaceEventListeners();
    }
    else {

        if ((window.document = window.context->LoadDocument("assets/startup.rml")))
            setStartupInterfaceEventListeners();
            window.document->Show();
    }

    while (window.running) {
        window.update();
    }
    return 0;
}
