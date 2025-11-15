#include "app.h"
#include "window.h"
#include "renderer.h"
#include "gui.h"
#include "scene.h"
#include <memory>

App::App() {
	window = std::make_unique<Window>();
	gui = std::make_unique<GUI>(window->getWindow());
	renderer = std::make_unique<Renderer>();
	scene = std::make_unique<Scene>();
}

App::~App() {

}

void App::run() {
	while (!window->shouldClose())
	{
		scene->update();

		gui->beginFrame();
		renderer->beginFrame();
		scene->render();
		renderer->endFrame();
		gui->endFrame();

		window->swapBuffers();
		window->pollEvents();
	}
}