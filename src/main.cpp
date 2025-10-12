#include "Helper.h"
#include "RmlUi_Backend.h"


/*
 * Known Bugs:
 * - if the amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the left
 * - when creating a new project or loading a project from the dropdown of the main interface it does not switch to that project automatically
 * - loading a project doesnt redirect to the interface screen

 * Todo:
 *  - dont have a recentpath
 *  - delete projects
 *  - callbacks for exporting importing saving loading etc
 *  - if clicking on new project or loadproject have the ability to go back
 *  - Update Checking
*/


void switchToStartup() {
    if ((window.document = window.context->LoadDocument("assets/startup.rml"))) {
        setStartupInterfaceEventListeners();
        window.document->Show();
    }
}

void setInterfaceEventListeners() {
#pragma region filedropdown
    if (auto *dropdownNewproject = window.document->GetElementById("file-dropdown-newproject")) {
        dropdownNewproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdownNewproject] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new")->SetClass("active", true);
            window.document->GetElementById("tab-load")->SetClass("active", false);
            window.document->GetElementById("tab-load")->SetAttribute("style", "display:none");
        }));
    }

    if (auto *dropdownLoadproject = window.document->GetElementById("file-dropdown-loadproject")) {
        dropdownLoadproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdownLoadproject] {
            switchToStartup();
            window.document->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            window.document->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            window.document->GetElementById("tab-load")->SetClass("active", true);
            window.document->GetElementById("tab-new")->SetClass("active", false);
            window.document->GetElementById("tab-new")->SetAttribute("style", "display:none");
        }));
    }

    if (auto *filedropdownexportproject = window.document->GetElementById("file-dropdown-exportproject")) {
        filedropdownexportproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([filedropdownexportproject] {
            try {
                std::string fullPath = saveData.path + "\\" + saveData.projectName;

                if (std::filesystem::exists(fullPath + ".tct")) {
                    std::filesystem::remove_all(fullPath + ".tct");
                }

                std::string command = "powershell Compress-Archive -Path \"" + fullPath +
                                      "\\*\" -DestinationPath \"" + fullPath + ".zip\" -Force";
                int result = std::system(command.c_str());

                if (result == 0)
                    std::cout << "Folder zipped successfully.\n";
                else
                    std::cerr << "Failed to zip folder. Exit code: " << result << '\n';

                std::filesystem::rename(fullPath + ".zip", fullPath + ".tct");

                std::cout << "File " + saveData.projectName + ".tct exported successfully to: " + saveData.path <<
                        std::endl;
            } catch (const std::filesystem::filesystem_error &e) {
                std::cerr << "Filesystem error: " << e.what() << '\n';
            }
            //TODO: feedback
        }));
    }

    if (auto *filedropdownimportproject = window.document->GetElementById("file-dropdown-importproject")) {
        filedropdownimportproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([filedropdownimportproject] {
            //TODO: import projects

            //TODO: feedback
        }));
    }

    if (auto *dropdowncloseproject = window.document->GetElementById("file-dropdown-closeproject")) {
        dropdowncloseproject->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdowncloseproject] {
            switchToStartup();
        }));
    }

    if (auto *dropdowncloseprogramm = window.document->GetElementById("file-dropdown-closeprogramm")) {
        dropdowncloseprogramm->AddEventListener(Rml::EventId::Click, new ButtonHandler([dropdowncloseprogramm] {
            exit(EXIT_SUCCESS);
        }));
    }

#pragma endregion
#pragma region settingsdropdown
#pragma endregion
#pragma region viewdropdown
#pragma endregion
#pragma region helpdropdown
#pragma endregion

    if (auto* projectorgrid = window.document->GetElementById("projectorGrid")) {
            std::cout << "[Info] Projectorcount: " << saveData.projectorCount << std::endl;
        for (int i = 0; i < saveData.projectorCount; ++i) {
            Rml::ElementPtr child = window.document->CreateElement("div");
            child->SetClass("projector", true);

            Rml::ElementPtr span = window.document->CreateElement("span");
            span->SetInnerRML("Beamer " + std::to_string(i + 1));
            child->AppendChild(std::move(span));


            projectorgrid->AppendChild(std::move(child));
        }
    }


    if (auto *projectname = window.document->GetElementById("project-name")) {
        projectname->SetInnerRML(saveData.projectName);
    }
}


int main() {
    if (std::filesystem::exists("../saves/recent.path")) {
        std::fstream pFile("../saves/recent.path");
        std::stringstream path;
        path << pFile.rdbuf();
        pFile.close();
        std::cout << "Found recent path" << path.str() << '\n';
        if (!std::filesystem::exists(path.str())) {
            std::cout << "Path was not found - going to startup" << path.str() << '\n';
            if ((window.document = window.context->LoadDocument("assets/startup.rml"))) {
                setStartupInterfaceEventListeners();
                window.document->Show();
            }
        }
        std::ifstream jFile(path.str());
        nlohmann::json j;
        jFile >> j;

        from_json(j, saveData);
        std::cout << saveData.path << std::endl;


        if ((window.document = window.context->LoadDocument("assets/interface.rml")))
            window.document->Show();

        setInterfaceEventListeners();
    } else {
        if ((window.document = window.context->LoadDocument("assets/startup.rml"))) {
            setStartupInterfaceEventListeners();
            window.document->Show();
        }
    }


    while (window.running) {
        window.update();
    }
    return 0;
}
