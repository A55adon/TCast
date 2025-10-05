#include "Helper.h"


/*
 * Known Bugs:
 * - if the amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the left
 * *
 * Todo:
 * - load into new scene
 * - message loaded/saved/created successfully
 *  - Update Checking
*/


int main() {
    if (std::filesystem::exists("../saves/recent.path")) {
        if ((window.document = window.context->LoadDocument("assets/interface.rml")))
            window.document->Show();
    } else {
        if ((window.document = window.context->LoadDocument("assets/startup.rml")))
            setStartupInterfaceEventListeners();
            window.document->Show();
    }

    while (window.running) {
        window.update();
    }
    return 0;
}
