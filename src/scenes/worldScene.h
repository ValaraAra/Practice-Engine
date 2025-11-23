#pragma once

#include "scene.h"
#include "window.h"
#include "input.h"
#include "sceneManager.h"
#include "shaderManager.h"
#include "shader.h"
#include "renderer.h"
#include <vector>
#include <memory>

class World;

class WorldScene : public Scene {
public:
	WorldScene(SceneManager& sceneManager, ShaderManager& shaderManager, InputManager& inputManager, Window& window);
	~WorldScene();

	void enter() override;
	void update(float deltaTime) override;
	void render(Renderer& renderer) override;
	void gui() override;
	void exit() override;

private:
	std::unique_ptr<World> world;

	Window& window;
	Shader& shader;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
	InputManager& inputManager;

	// Input callback handles
	std::vector<CallbackHandle> inputCallbackHandles;

	glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 0.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	float cameraYaw = -90.0f;
	float cameraPitch = 0.0f;

	glm::vec2 lastMousePosition = glm::vec2(0.0f, 0.0f);
	bool firstMousePosition = true;

	bool cameraMovementDisabled = false;
	bool exitSceneRequested = false;

	struct {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
		bool sprint = false;
	} movement;

	void updateCamera(float deltaTime);
};