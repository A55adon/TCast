#include "Helper.h"


/*
 *Known Bugs:
 * - if amount of folderProjects is higher than 8 or fills up the height of the list they are unclickable and are squished to the lest
 *
 * Ideas:
 *  - Update Checking
 *
 * Todo:
 * - load into new scene
 * - message loaded/saved/created successfully
*/



int main() {

    if ((window.document = window.context->LoadDocument("assets/startup.rml")))
        window.document->Show();

    PopulateFolders(window.document, "../saves/folderSaves/");

    setStartupInterfaceEventListeners();

    while (window.running) {
        window.update();
    }
    return 0;
}