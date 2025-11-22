#pragma once

#include <memory>

class Window;
class Renderer;
class GUI;
class SceneManager;
class ShaderManager;
class Scene;

class App {
public:
	App();
	~App();
	void run();

private:
	std::unique_ptr<Window> window;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<GUI> gui;
	std::unique_ptr<SceneManager> sceneManager;
	std::unique_ptr<ShaderManager> shaderManager;
};