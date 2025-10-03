#include "Helper.h"
#include "ButtonListener.h"
#include <fstream>
#include <iostream>

/*
 * - check if project name already exists
 * - load into new scene
 *
*/

Window window = Window(1920, 1080);

bool saveNewProject() {
    SaveData saveData;
    std::string path;

    if (auto *el = window.document->GetElementById("project-name-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            std::string value = input->GetValue();

            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto *element_error = window.document->GetElementById("error-text")) {
                    element_error->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                }
                return false;
            }

            saveData.projectName = value;
        }
    }
    if (auto *el = window.document->GetElementById("projector-count-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl*>(el)) {
            Rml::String value = input->GetValue();
            int count = 0;
            if (!validateBeamerCount(value, count)) {
                if (auto *err = window.document->GetElementById("error-text")) {
                    err->SetInnerRML("Nur Zahlen 1-9 erlaubt!");
                }
                return false;
            }
            saveData.beamerCount = count;
        }
    }
    if (auto *el = window.document->GetElementById("project-desc-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            Rml::String value = input->GetValue();

            static const std::regex pattern("^[A-Za-z0-9äöüÄÖÜ _.\\-;,]+$");
            if (!std::regex_match(value, pattern)) {
                if (auto *element_error = window.document->GetElementById("error-text")) {
                    element_error->SetInnerRML("Ungültige Zeichen! erlaubt ist: ^[A-Za-z0-9äöüÄÖÜ _.-;,]+$");
                }
                return false;
            }

            saveData.description = value;
        }
    }

    if (auto *el = window.document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            Rml::String value = input->GetValue();

            saveData.path = value;
            if (value == ToBackwardSlashes(GetSaveFolderPath())){
                std::string forwardSlashpath = value + "/" + saveData.projectName + "/" + saveData.projectName + ".json";
                path = ToBackwardSlashes(forwardSlashpath);
            }
            else
                path = value;

            std::cout << path << std::endl;
        }
    }
    std::filesystem::path filePath = path;
    auto folderPath = filePath.parent_path();
    std::error_code ec;

    // Check if folder exists
    if (std::filesystem::exists(folderPath)) {
        if (!std::filesystem::is_directory(folderPath)) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML("Pfad existiert, ist aber keine Ordnerstruktur!");
            }
            return false;
        }
    } else {
        // Try to create missing directories
        if (!std::filesystem::create_directories(folderPath, ec) || ec) {
            if (auto *el = window.document->GetElementById("error-text")) {
                el->SetInnerRML(("Konnte Ordner nicht erstellen: " + ec.message()).c_str());
            }
            return false;
        }
    }

    // Prevent overwriting existing project file
    if (std::filesystem::exists(filePath)) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Projekt existiert bereits – bitte anderen Namen wählen!");
        }
        return false;
    }

    // Open file for writing
    json j = saveData;
    std::ofstream file(filePath);
    if (!file.is_open()) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("Konnte Datei nicht erstellen – ungültiger Pfad oder Rechteproblem");
        }
        return false;
    }

    // Write JSON
    file << j.dump(4);
    file.close();

    // Verify file exists
    if (!std::filesystem::exists(filePath)) {
        if (auto *el = window.document->GetElementById("error-text")) {
            el->SetInnerRML("JSON-Datei wurde nicht erstellt!");
        }
        return false;
    }

    return true;

}

void setStartupInterfaceEventListeners()
{
    auto *tabLoad = window.document->GetElementById("tab-load");
    auto *tabNew = window.document->GetElementById("tab-new");

    if (tabLoad) {
        tabLoad->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-load-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-new-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-load")->SetClass("active", true);
            doc->GetElementById("tab-new")->SetClass("active", false);
        }));
    }

    if (tabNew) {
        tabNew->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-load-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-new-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-new")->SetClass("active", true);
            doc->GetElementById("tab-load")->SetClass("active", false);
        }));
    }

    if (auto *tabFolder = window.document->GetElementById("tab-folder")) {
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-folder-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-tct-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-folder")->SetClass("active", true);
            doc->GetElementById("tab-tct")->SetClass("active", false);
        }));
    }

    if (auto *tabTct = window.document->GetElementById("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            doc->GetElementById("tab-folder-div")->SetAttribute("style", "display:none");
            doc->GetElementById("tab-tct-div")->SetAttribute("style", "display:flex");
            doc->GetElementById("tab-tct")->SetClass("active", true);
            doc->GetElementById("tab-folder")->SetClass("active", false);
        }));
    }

    if (auto *browseFolderBtn = window.document->GetElementById("browse-folder-btn")) {
        browseFolderBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                SetSelectedProject(doc, folder);
            }
        }));
    }

    if (auto *browseTctBtn = window.document->GetElementById("browse-tct-btn")) {
        browseTctBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string file = BrowseTCTFile();
            if (!file.empty()) {
                SetSelectedProject(doc, file);
            }
        }));
    }
    if (auto *saveNewProjectBtn = window.document->GetElementById("save-btn")) {
        saveNewProjectBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (saveNewProject()) {
                doc->Hide();
            }
        }));
    }

    for (int i = 1; i <= 5; i++) {
        std::string id = "folder-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [doc = window.document, name = proj->GetInnerRML()] {
                                           SetSelectedProject(doc, name);
                                       }));
        }
        id = "tct-proj-" + std::to_string(i);
        if (auto *proj = window.document->GetElementById(id)) {
            proj->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                                       [doc = window.document, name = proj->GetInnerRML()] {
                                           SetSelectedProject(doc, name);
                                       }));
        }
    }
    if (auto *el = window.document->GetElementById("project-dir-input")) {
        if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
            input->SetValue(ToBackwardSlashes(GetSaveFolderPath()));
        }
    }
}

int main() {

    if ((window.document = window.context->LoadDocument("assets/interface.rml")))
        window.document->Show();

    setStartupInterfaceEventListeners();

    while (window.running) {
        window.update();
    }
    return 0;
}

