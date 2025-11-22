#include "app.h"
#include "window.h"
#include "renderer.h"
#include "gui.h"
#include "sceneManager.h"
#include "shaderManager.h"
#include "scenes/triangleScene.h"
#include "scenes/squareScene.h"
#include "scenes/menuScene.h"
#include <glm/glm.hpp>
#include <memory>

App::App() {
	// Initialize core components
	window = std::make_unique<Window>(glm::ivec2(1280, 720), "Practice Engine");
	gui = std::make_unique<GUI>(window->getWindow());
	renderer = std::make_unique<Renderer>(window->getWindow());
	sceneManager = std::make_unique<SceneManager>();
	shaderManager = std::make_unique<ShaderManager>();

	// Register scenes
	sceneManager->registerScene("Triangle", std::make_unique<TriangleScene>(*sceneManager, *shaderManager));
	sceneManager->registerScene("Square", std::make_unique<SquareScene>(*sceneManager, *shaderManager));
	sceneManager->registerScene("Menu", std::make_unique<MenuScene>(*sceneManager));

	// Set initial scene
	sceneManager->setScene("Menu");
}

App::~App() {

}

void App::run() {
	Scene* currentScene = sceneManager->getCurrentScene();

	while (!window->shouldClose())
	{
		currentScene = sceneManager->getCurrentScene();

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