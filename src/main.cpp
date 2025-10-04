#include "Helper.h"
#include "ButtonListener.h"
#include <fstream>
#include <iostream>

/*
 *Known Bugs:
 * - if amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the lest
 *
 * TODO:
 * - load into new scene
 * - message loaded/saved/created successfully
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
            std::string forwardSlashpath = value + "/" + saveData.projectName + "/" + saveData.projectName + ".json";
            path = ToBackwardSlashes(forwardSlashpath);
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

    if (auto* tabFolder = window.document->GetElementById("tab-folder")) {
        tabFolder->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabFolder] {
            if (auto* fDiv = window.document->GetElementById("tab-folder-div"))
                fDiv->SetProperty("display", "flex");
            if (auto* tDiv = window.document->GetElementById("tab-tct-div"))
                tDiv->SetProperty("display", "none");

            if (auto* bFolder = window.document->GetElementById("browse-folder-btn"))
                bFolder->SetProperty("display", "block");
            if (auto* bTct = window.document->GetElementById("browse-tct-btn"))
                bTct->SetProperty("display", "none");

            tabFolder->SetClassNames("tab-button active");
            if (auto* tabTct = window.document->GetElementById("tab-tct"))
                tabTct->SetClassNames("tab-button");
        }));
    }

    if (auto* tabTct = window.document->GetElementById("tab-tct")) {
        tabTct->AddEventListener(Rml::EventId::Click, new ButtonHandler([tabTct] {
            if (auto* fDiv = window.document->GetElementById("tab-folder-div"))
                fDiv->SetProperty("display", "none");
            if (auto* tDiv = window.document->GetElementById("tab-tct-div"))
                tDiv->SetProperty("display", "flex");

            if (auto* bFolder = window.document->GetElementById("browse-folder-btn"))
                bFolder->SetProperty("display", "none");
            if (auto* bTct = window.document->GetElementById("browse-tct-btn"))
                bTct->SetProperty("display", "block");

            tabTct->SetClassNames("tab-button active");
            if (auto* tabFolder = window.document->GetElementById("tab-folder"))
                tabFolder->SetClassNames("tab-button");
        }));
    }

    if (auto *browseFolderBtn = window.document->GetElementById("browse-folder-btn")) {
        browseFolderBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                if (auto* inputEl = doc->GetElementById("load-dir-input"))
                    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                        input->SetValue(ToBackwardSlashes(folder));
                        std::cout << "clicked" << std::endl;
            }
        }));
    }
    if (auto *browseLoadBtn = window.document->GetElementById("browse-load-btn")) {
        browseLoadBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string folder = BrowseFolder();
            if (!folder.empty()) {
                if (auto* inputEl = doc->GetElementById("load-dir-input"))
                    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                        input->SetValue(ToBackwardSlashes(folder));
            }
        }));
    }

    if (auto *browseTctBtn = window.document->GetElementById("browse-tct-btn")) {
        browseTctBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            std::string file = BrowseTCTFile();
            if (!file.empty()) {
                if (auto* inputEl = doc->GetElementById("load-dir-input"))
                    if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                        input->SetValue(ToBackwardSlashes(file));

            }
        }));
    }

    if (auto *browseDirBtn = window.document->GetElementById("browse-btn")) {
        browseDirBtn->AddEventListener(Rml::EventId::Click, new ButtonHandler([doc = window.document] {
            if (auto *el = doc->GetElementById("project-dir-input")) {
                if (auto *input = dynamic_cast<Rml::ElementFormControl *>(el)) {
                    input->SetValue(ToBackwardSlashes(BrowseFolder()));
                }
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

void loadProjectsTCT(){}
void loadProjectsFolder(){}
void PopulateFolders(Rml::ElementDocument* doc, const std::string& path) {
    namespace fs = std::filesystem;
    Rml::Element* container = doc->GetElementById("tab-folder-list");
    if (!container) return;
    container->SetInnerRML("");

    for (auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            std::string fullPath = entry.path().string();

            Rml::ElementPtr folderDiv = doc->CreateElement("div");
            folderDiv->SetClassNames("sample-project");
            folderDiv->SetId("folder-" + folderName);
            folderDiv->SetInnerRML(folderName);

            folderDiv->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                 [doc, fullPath, folderName] {
                     if (auto* inputEl = doc->GetElementById("load-dir-input"))
                         if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                             input->SetValue(ToBackwardSlashes(fullPath));
                 }
             ));

            container->AppendChild(std::move(folderDiv));
        }
    }
}

void PopulateTCTFiles(Rml::ElementDocument* doc, const std::string& path) {
    namespace fs = std::filesystem;
    Rml::Element* container = doc->GetElementById("tab-tct-list");
    if (!container) return;
    container->SetInnerRML("");

    for (auto& entry : fs::directory_iterator(path)) {
        if (entry.path().extension() == ".tct") {
            std::string fileName = entry.path().filename().string();
            std::string fullPath = entry.path().string();

            Rml::ElementPtr fileDiv = doc->CreateElement("div");
            fileDiv->SetClassNames("sample-project");
            fileDiv->SetId("tct-" + fileName);
            fileDiv->SetInnerRML(fileName);

            fileDiv->AddEventListener(Rml::EventId::Click, new ButtonHandler(
                [doc, fullPath, fileName] {
                    if (auto* inputEl = doc->GetElementById("load-dir-input"))
                        if (auto* input = dynamic_cast<Rml::ElementFormControl*>(inputEl))
                            input->SetValue(ToBackwardSlashes(fullPath));
                }
            ));
            container->AppendChild(std::move(fileDiv));
        }
    }
}

SaveData openProject() {
    SaveData sd;
    return sd;
}

int main() {

    if ((window.document = window.context->LoadDocument("assets/startup.rml")))
        window.document->Show();

    PopulateFolders(window.document, "../saves/folderSaves/");
    PopulateTCTFiles(window.document, "../saves/tctSaves/");

    setStartupInterfaceEventListeners();

    while (window.running) {
        window.update();
    }
    return 0;
}