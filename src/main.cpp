#include "Window.h"

int main()
{
	Window window = Window(1000,1000);

	window.addProjector(0);

	while (window.running)
	{
		window.update();
	}

	return 0;
}
