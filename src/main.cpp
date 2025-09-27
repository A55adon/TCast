
#include "Window.h"

int main() {
	Window window;

	// Add projectors on monitors 1 and 2 (if connected)
	window.addProjector(0); // first monitor
	window.addProjector(1); // second monitor

	while (!window.shouldClose()) {
		if( !window.running ) return -1;

		window.running = Backend::ProcessEvents(window.context, &Shell::ProcessKeyDownShortcuts, true);

		window.context->Update();

		Backend::BeginFrame();
		window.context->Render();
		Backend::PresentFrame();
		window.update();
	}
}