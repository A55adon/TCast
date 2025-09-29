#include <iostream>

#include "ButtonListener.h"
#include "Window.h"
#include "RmlUi/Debugger.h"
#include "RmlUi_Backend.h"
#include "Shell.h"
int main() {
	Window window = Window(1000,1000);
	if (window.document = window.context->LoadDocument("assets/interface.rml"))
		window.document->Show();

	if (Rml::Element* button = window.document->GetElementById("button1")) {
		button->AddEventListener(Rml::EventId::Click, new ButtonHandler([&window] {
			window.addProjector(0);
		}));
	};

	while (window.running)
	{
		window.update();
	}

	return 0;
}
