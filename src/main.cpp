#include "gui.h"
#include "window.h"
#include "renderer.h"
#include "scene.h"

int main()
{
	Window window;
	GUI gui(window.getWindow());
	Renderer renderer;
	Scene scene;

	// Main loop
	while (!window.shouldClose())
	{
		scene.update();

		gui.beginFrame();
		renderer.beginFrame();
		scene.render();
		renderer.endFrame();
		gui.endFrame();

		window.swapBuffers();
		window.pollEvents();
	}

	return 0;
}

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	return main();
}
#endif