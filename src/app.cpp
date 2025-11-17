#include "app.h"
#include "window.h"
#include "renderer.h"
#include "gui.h"
#include "sceneManager.h"
#include "scenes/triangleScene.h"
#include "scenes/squareScene.h"
#include <glm/glm.hpp>
#include <memory>

App::App() {
	// Initialize core components
	window = std::make_unique<Window>(glm::ivec2(1280, 720), "Practice Engine");
	gui = std::make_unique<GUI>(window->getWindow());
	renderer = std::make_unique<Renderer>();
	sceneManager = std::make_unique<SceneManager>();

	// Register scenes
	sceneManager->registerScene("TriangleScene", std::make_unique<TriangleScene>());
	sceneManager->registerScene("SquareScene", std::make_unique<SquareScene>());

	// Set initial scene
	sceneManager->setScene("SquareScene");
}

App::~App() {

}

void App::run() {
	Scene* currentScene = sceneManager->getCurrentScene();

	while (!window->shouldClose())
	{
		if (currentScene) {
			currentScene->update();
		}

		gui->beginFrame();

		if (currentScene) {
			currentScene->gui();
		}

		renderer->beginFrame();

		if (currentScene) {
			currentScene->render();
		}

		renderer->endFrame();
		gui->endFrame();

		window->swapBuffers();
		window->pollEvents();
	}
}